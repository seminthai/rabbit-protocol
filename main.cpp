#include "rabbit_codec.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <memory>
#include <iomanip>

int main() {
    std::cout << "========================================================\n";
    std::cout << "=== PROTOCOL COMPONENT: RABBIT ENGINE v3.0 ===========\n";
    std::cout << "========================================================\n\n";

    // Allocate the codec instance dynamically inside the heap segment
    auto rabbit = std::make_unique<RabbitCodec>();

    const std::string input_filename = "test.bin";
    std::ifstream file_in(input_filename, std::ios::binary | std::ios::ate);
    if (!file_in.is_open()) {
        std::cerr << "[ERROR] Could not open '" << input_filename << "'! Please put 'test.bin' in target folder.\n";
        return 1;
    }

    std::streamsize file_size = file_in.tellg();
    file_in.seekg(0, std::ios::beg);

    std::vector<uint8_t> source_bytes(file_size);
    file_in.read(reinterpret_cast<char*>(source_bytes.data()), file_size);
    file_in.close();

    std::cout << "[File] Source file loaded successfully.\n";
    std::cout << "[File] Size : " << source_bytes.size() << " bytes.\n\n";

    std::vector<WireState> physical_wire;
    std::vector<uint8_t> restored_bytes;

    std::cout << "[Run] Transmitter Parallel Execution..." << std::endl;
    auto t_tx_start = std::chrono::high_resolution_clock::now();
    rabbit->Encode(source_bytes, physical_wire);
    auto t_tx_end = std::chrono::high_resolution_clock::now();

    std::cout << "[Run] Receiver Multi-Threaded OpenMP Execution..." << std::endl;
    auto t_rx_start = std::chrono::high_resolution_clock::now();
    rabbit->Decode(physical_wire, restored_bytes);
    auto t_rx_end = std::chrono::high_resolution_clock::now();

    double tx_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_tx_end - t_tx_start).count() / 1000.0;
    double rx_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_rx_end - t_rx_start).count() / 1000.0;
    double total_sec = (tx_ms + rx_ms) / 1000.0;

    double mbytes_per_sec = total_sec > 0 ? ((double)source_bytes.size() / (1024.0 * 1024.0)) / total_sec : 0.0;
    bool integrity = (source_bytes == restored_bytes);

    std::cout << "\n=== INTEGRITY & PERFORMANCE REPORT ===\n";
    std::cout << "[STATS] Wire tacts spent : " << physical_wire.size() << " tacts.\n";
    std::cout << "[STATS] Density ratio    : " << std::fixed << std::setprecision(3)
        << (physical_wire.empty() ? 0.0 : (double)(source_bytes.size() * 8) / physical_wire.size()) << " bits/tact.\n";
    std::cout << "[TIME] Transmitter time  : " << tx_ms << " ms.\n";
    std::cout << "[TIME] Receiver time     : " << rx_ms << " ms.\n";
    std::cout << "[PERF] Net Performance   : " << std::fixed << std::setprecision(2) << mbytes_per_sec << " MB/sec.\n";

    if (integrity) {
        std::cout << "[OK] 100% BIT-PERFECT TURBO CONVERGENCE!\n";
    }
    else {
        std::cout << "[FAIL] Data corruption detected!\n";
    }
    std::cout << "========================================================\n";

    return 0;
}
