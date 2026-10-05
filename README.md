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

### License
This project is licensed under the MIT License - see the LICENSE file for details.
