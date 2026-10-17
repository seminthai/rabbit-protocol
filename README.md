# Rabbit Protocol Simulator (v3.0 Concept)

An experimental simulator and Proof-of-Concept (PoC) for a custom physical and transport layer data encoding architecture.

### The "Rabbit" Concept
The Rabbit framework was born from an engineering metaphor: placing a lightweight Layer 2 ("the rabbit") directly onto a "free-ride" transport Layer 1 infrastructure. 

In this architecture, the physical transmission waveform is represented as a structured combination of a *Value* and a *Delta* (change state).

Key features of the implementation:
- **Unary-Modulated Signaling:** Data nibbles are packed into dynamic unary bitstreams, separated on the physical line by a High-Impedance state (`WireState::HIGH_Z`).
- **XOR-Resolving:** Employs bitwise XOR operations to gracefully resolve and match upper/lower bounding indexes within a frame without using heavy conditional branching on the receiver side.
- **Bi-Directional LUT Acceleration:** High-performance simulation speed (achieving 4.0+ MB/sec on large files) powered by pre-computed direct and inverse look-up tables for waveforms.
- **Parallel Bus Parsing (OpenMP):** The physical structure of frames bound by `HIGH_Z` delimiters allows the bitstream to be sliced cleanly across multiple CPU cores, enabling asynchronous multi-threaded decoding.

### Current Project Status
This repository serves as a **Research Proof-of-Concept**. At this stage, the algorithm demonstrates 100% Bit-Perfect convergence within an ideal software-simulated environment. It is designed strictly as a mathematical and algorithmic simulation and does not contain advanced error-correction mechanisms (such as CRC or ARQ) required for noisy, real-world physical networks.

### How to Run
1. Clone the repository and build the project using CMake in **Release** configuration.
2. Place any sufficiently large binary file (recommended size: > 10 MB to allow OpenMP multi-threading to scale effectively) named `test.bin` into the target executable directory.
3. Execute the `rabbit_simulator` binary to view the performance and integrity report.

### License

This project is dual-licensed to balance open scientific collaboration with proprietary commercial protection:
1. Open Source & Academic Use: The source code is published under the standard GNU GPLv3 license (see the LICENSE file). It is fully open for academic research, independent auditing, and non-commercial development. Anyone using or modifying this code under GPLv3 must keep their derived works open-source under the same license terms.
2. Commercial Use: Integrating, copying, or implementing the core logical principles, hardware architectures, waveform encoding schemes, or source code of this technology into closed-source commercial or proprietary products (including custom physical layer transceivers, network interface controllers, or hardware communication intellectual property) is strictly prohibited under the GPLv3 terms (as it legally requires disclosing your proprietary source code).
Commercial deployment or proprietary implementation of the Rabbit Protocol technology is permissible solely upon executing a Private Commercial Agreement with the copyright holder.
