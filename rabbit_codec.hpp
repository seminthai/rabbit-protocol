#pragma once

#include <vector>
#include <cstdint>
#include <array>

// Physical states of the custom transmission line
enum class WireState : uint8_t {
    LOGIC_0 = 0,
    LOGIC_1 = 1,
    HIGH_Z = 2
};

class RabbitCodec {
private:
    static constexpr size_t MAX_FRAME_BITS = 64;

    // Optimized frame structure containing bits sequence
    struct OptimizedFrame {
        uint8_t length;
        std::array<WireState, MAX_FRAME_BITS> bits;
    };

    // Pre-packed bits representation for faster unary compression
    struct PrePackedByte {
        uint64_t bit_pattern;
        uint8_t bit_length;
    };

    // Internal Look-Up Tables (LUT)
    std::array<std::array<OptimizedFrame, 16>, 16> BINARY_LUT;
    std::array<OptimizedFrame, 256> TRANSMITTER_WIRE_LUT;
    std::vector<std::vector<uint8_t>> INVERSE_BINARY_LUT;
    std::vector<PrePackedByte> TRANSMITTER_COMPRESS_LUT;

    // Helper functions for lookup tables initialization
    inline uint32_t pack_frame_to_mask(const WireState* bits, uint8_t length);
    void initialize_binary_lut();
    void initialize_inverse_lut();
    void initialize_transmitter_wire_lut();
    void initialize_transmitter_compress_lut();

public:
    RabbitCodec();
    ~RabbitCodec() = default;

    // Delete copy operations to prevent expensive table memory allocations
    RabbitCodec(const RabbitCodec&) = delete;
    RabbitCodec& operator=(const RabbitCodec&) = delete;

    // Transmitter: Encode input byte stream into physical wire states (L2 -> L1)
    void Encode(const std::vector<uint8_t>& src, std::vector<WireState>& physical_wire);

    // Receiver: Multi-threaded decoding of physical wire states into original bytes (L1 -> L2)
    void Decode(const std::vector<WireState>& physical_wire, std::vector<uint8_t>& dest);
};
