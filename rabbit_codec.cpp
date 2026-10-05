#include "rabbit_codec.hpp"
#include <cstring>
#include <omp.h>

inline uint32_t RabbitCodec::pack_frame_to_mask(const WireState* bits, uint8_t length) {
    uint32_t mask = 0;
    for (uint8_t i = 0; i < length; ++i) {
        if (bits[i] == WireState::LOGIC_1) mask |= (1 << i);
    }
    return mask;
}

void RabbitCodec::initialize_binary_lut() {
    for (int l1 = 0; l1 < 16; ++l1) {
        for (int l2 = 0; l2 < 16; ++l2) {
            OptimizedFrame& frame = BINARY_LUT[l1][l2];
            int idx = 0;
            if (l1 == 0 && l2 == 0) {}
            else if (l1 == l2) {
                for (int k = 0; k < l1; ++k) frame.bits[idx++] = WireState::LOGIC_0;
                frame.bits[idx++] = WireState::LOGIC_1;
                frame.bits[idx++] = WireState::LOGIC_0;
            }
            else if (l1 > 0 && l2 == 0) {
                for (int k = 0; k < l1; ++k) frame.bits[idx++] = WireState::LOGIC_0;
            }
            else if (l1 == 0 && l2 > 0) {
                for (int k = 0; k < l2; ++k) frame.bits[idx++] = WireState::LOGIC_1;
            }
            else if (l1 > l2) {
                for (int k = 0; k < l2; ++k) frame.bits[idx++] = WireState::LOGIC_0;
                for (int k = 0; k < (l1 - l2); ++k) frame.bits[idx++] = WireState::LOGIC_1;
            }
            else if (l2 > l1) {
                for (int k = 0; k < l1; ++k) frame.bits[idx++] = WireState::LOGIC_1;
                for (int k = 0; k < (l2 - l1); ++k) frame.bits[idx++] = WireState::LOGIC_0;
            }
            frame.length = static_cast<uint8_t>(idx);
        }
    }
}

void RabbitCodec::initialize_inverse_lut() {
    for (int l1 = 0; l1 < 16; ++l1) {
        for (int l2 = 0; l2 < 16; ++l2) {
            const OptimizedFrame& frame = BINARY_LUT[l1][l2];
            if (frame.length > 0 && frame.length < 18) {
                uint32_t mask = pack_frame_to_mask(frame.bits.data(), frame.length);
                INVERSE_BINARY_LUT[frame.length][mask] = static_cast<uint8_t>((l1 << 4) | (l2 & 0x0F));
            }
        }
    }
}

void RabbitCodec::initialize_transmitter_wire_lut() {
    for (int b = 0; b < 256; ++b) {
        uint8_t l1 = (b >> 4) & 0x0F;
        uint8_t l2 = b & 0x0F;
        const OptimizedFrame& src_frame = BINARY_LUT[l1][l2];
        OptimizedFrame& dst_frame = TRANSMITTER_WIRE_LUT[b];
        dst_frame.length = src_frame.length;
        if (src_frame.length > 0) {
            std::memcpy(dst_frame.bits.data(), src_frame.bits.data(), src_frame.length * sizeof(WireState));
        }
        // Embed the HIGH_Z delimiter frame marker directly into the wire LUT
        dst_frame.bits[dst_frame.length] = WireState::HIGH_Z;
        dst_frame.length += 1;
    }
}

void RabbitCodec::initialize_transmitter_compress_lut() {
    for (int b = 0; b < 256; ++b) {
        uint8_t byte = static_cast<uint8_t>(b);
        uint8_t nibbles[2] = { static_cast<uint8_t>((byte >> 4) & 0x0F), static_cast<uint8_t>(byte & 0x0F) };
        uint64_t pattern = 0;
        uint8_t len = 0;
        for (int n = 0; n < 2; ++n) {
            uint8_t val = nibbles[n];
            for (int k = 0; k <= val; ++k) {
                pattern = (pattern << 1) | 1; len++;
            }
            pattern = (pattern << 1) | 0; len++;
        }
        // Shift pattern to the left side (MSB-aligned) to provide fast bit-streaming packing
        TRANSMITTER_COMPRESS_LUT[byte].bit_pattern = pattern << (64 - len);
        TRANSMITTER_COMPRESS_LUT[byte].bit_length = len;
    }
}

RabbitCodec::RabbitCodec() {
    // Allocate heap memory for the large index-jump tables
    INVERSE_BINARY_LUT.assign(18, std::vector<uint8_t>(131072, 0));
    TRANSMITTER_COMPRESS_LUT.resize(256);

    initialize_binary_lut();
    initialize_inverse_lut();
    initialize_transmitter_wire_lut();
    initialize_transmitter_compress_lut();
}

void RabbitCodec::Encode(const std::vector<uint8_t>& src, std::vector<WireState>& physical_wire) {
    size_t src_size = src.size();
    if (src_size == 0) return;

    std::vector<uint8_t> packed_bytes(src_size * 4 + 8);
    uint8_t* p_packed = packed_bytes.data();
    size_t packed_idx = 0;

    uint64_t bit_accumulator = 0;
    int accumulator_bits = 0;
    const uint8_t* p_src = src.data();

    // Phase 1: Fast streaming packing using MSB-aligned compression LUT
    for (size_t i = 0; i < src_size; ++i) {
        const PrePackedByte& lut = TRANSMITTER_COMPRESS_LUT[p_src[i]];
        bit_accumulator |= (lut.bit_pattern >> accumulator_bits);
        accumulator_bits += lut.bit_length;

        while (accumulator_bits >= 8) {
            uint8_t out_byte = static_cast<uint8_t>(bit_accumulator >> 56);
            p_packed[packed_idx++] = ~out_byte;
            bit_accumulator <<= 8;
            accumulator_bits -= 8;
        }
    }

    if (accumulator_bits > 0) {
        uint8_t out_byte = static_cast<uint8_t>(bit_accumulator >> 56);
        p_packed[packed_idx++] = ~out_byte;
    }

    physical_wire.resize(packed_idx * 20);
    WireState* p_wire = physical_wire.data();
    size_t wire_idx = 0;

    // Phase 2: Macro block-copying of ready physical frames to the wire
    for (size_t i = 0; i < packed_idx; ++i) {
        const OptimizedFrame& frame = TRANSMITTER_WIRE_LUT[p_packed[i]];
        std::memcpy(p_wire + wire_idx, frame.bits.data(), frame.length * sizeof(WireState));
        wire_idx += frame.length;
    }
    physical_wire.resize(wire_idx);
}

void RabbitCodec::Decode(const std::vector<WireState>& physical_wire, std::vector<uint8_t>& dest) {
    dest.clear();
    if (physical_wire.empty()) return;

    size_t wire_size = physical_wire.size();
    const WireState* p_wire = physical_wire.data();

    std::vector<uint8_t> rx_packed_bytes(wire_size / 2 + 1);
    uint8_t* p_rx_packed = rx_packed_bytes.data();
    size_t rx_packed_idx = 0;

    int max_threads = omp_get_max_threads();
    std::vector<std::vector<uint8_t>> thread_buffers(max_threads);

    // Phase 1: Multi-threaded wire parsing via OpenMP
#pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int total_threads = omp_get_num_threads();

        size_t chunk_size = wire_size / total_threads;
        size_t start_pos = thread_id * chunk_size;
        size_t end_pos = (thread_id == total_threads - 1) ? wire_size : (start_pos + chunk_size);

        // Align thread processing chunk to the closest HIGH_Z frame delimiter boundary
        if (thread_id > 0) {
            while (start_pos < wire_size && p_wire[start_pos - 1] != WireState::HIGH_Z) {
                start_pos++;
            }
        }

        size_t i = start_pos;
        std::vector<uint8_t>& local_rx_packed = thread_buffers[thread_id];
        local_rx_packed.reserve((end_pos - start_pos) / 4);

        while (i < end_pos && i < wire_size) {
            uint32_t mask = 0;
            uint8_t local_len = 0;

            // Collect bits inside the current frame window between HIGH_Z separators
            while (i < wire_size && p_wire[i] != WireState::HIGH_Z) {
                if (p_wire[i] == WireState::LOGIC_1) mask |= (1 << local_len);
                local_len++;
                i++;
            }
            i++; // Skip the HIGH_Z delimiter

            if (local_len > 0 && local_len < 18) {
                // Instantly extract original byte using O(1) jump table index
                local_rx_packed.push_back(INVERSE_BINARY_LUT[local_len][mask]);
            }
            else {
                local_rx_packed.push_back(0);
            }
        }
    }

    // Merge localized execution thread buffers sequentially to maintain the correct stream order
    for (int t = 0; t < max_threads; ++t) {
        if (!thread_buffers[t].empty()) {
            std::memcpy(p_rx_packed + rx_packed_idx, thread_buffers[t].data(), thread_buffers[t].size());
            rx_packed_idx += thread_buffers[t].size();
        }
    }

    // Phase 2: Decouple the compressed unary code array stream into nibbles
    std::vector<uint8_t> restored_nibbles(rx_packed_idx * 4);
    uint8_t* p_nibbles = restored_nibbles.data();
    size_t nibble_idx = 0;

    uint8_t current_unary_count = 0;
    bool stop_decoding = false;

    for (size_t idx = 0; idx < rx_packed_idx; ++idx) {
        if (stop_decoding) break;
        uint8_t original_packed = ~p_rx_packed[idx];
        for (int bit = 7; bit >= 0; --bit) {
            if ((original_packed >> bit) & 1) {
                current_unary_count++;
            }
            else {
                if (current_unary_count > 0) {
                    p_nibbles[nibble_idx++] = current_unary_count - 1;
                    current_unary_count = 0;
                }
                else {
                    stop_decoding = true;
                    break;
                }
            }
        }
    }

    // Reconstruct bytes from pair-grouped nibbles
    dest.resize(nibble_idx / 2);
    uint8_t* p_dest = dest.data();
    size_t dest_idx = 0;
    for (size_t idx = 0; idx + 1 < nibble_idx; idx += 2) {
        p_dest[dest_idx++] = (p_nibbles[idx] << 4) | (p_nibbles[idx + 1] & 0x0F);
    }
}
