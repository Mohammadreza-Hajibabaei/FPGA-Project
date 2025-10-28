Great — I reviewed your **FPGA Project Draft** file. Based on the document, here’s a clear and professional **README.md** (in English) that explains **Phase 1** and **Phase 2** of your project, suitable for your GitHub repository.

---

# FPGA Signal Processing Project (Zynq-based)

## Overview

This project is part of the *ASIC/FPGA System Design* course at Sharif University of Technology.
The goal is to implement **a simple digital signal processing system** on a **Zynq platform**, involving waveform generation, data transmission between PS (Processing System) and PL (Programmable Logic), signal scrambling/descrambling, and FFT spectrum visualization via HDMI.

---

## Phase 1 – Signal Generation and FFT Visualization

### Objectives

In Phase 1, the task was to:

1. Generate **four types of waveforms** (sine, square, triangular, and sawtooth) in the **Processing System (PS)**.
2. Send the generated signals to the **Programmable Logic (PL)** through a **DMA (AXI DMA)** interface.
3. Perform an **FFT (Fast Fourier Transform)** on the received signal in PL.
4. Display both the **input signal** and its **FFT spectrum** on an **HDMI display**.

### Implementation Details

* Each waveform consists of **1024 samples** (8-bit data).
* The signal is transferred from PS to PL using **AXI Memory-Mapped to AXI Stream DMA**.
* A **key press (PS_KEY1)** is used to switch between different waveform types.
* The FFT block in PL processes the incoming data stream continuously.
* The output of the FFT is a **complex signal (Real + Imaginary parts)**.
* The amplitude is approximated using:
  [
  |X| ≈ |Real| + |Imag|
  ]
* The display shows:

  * The **upper part** of the screen: the input waveform.
  * The **lower part**: the FFT spectrum of the signal.

### Tools and Components

* **Zynq SoC** (PS + PL architecture)
* **AXI DMA Interface**
* **FFT IP Core**
* **HDMI Display Controller**
* **VHDL/Verilog for PL design**
* **C/C++ code** on PS for signal generation and DMA control

---

## Phase 2 – Scrambler and Descrambler Implementation

### Objectives

In Phase 2, the project was extended by adding **data scrambling and descrambling** between PS and PL to simulate a communication link.

### Implementation Details

* A **Scrambler** was implemented in the **transmitter (PL)** side.
* A **Descrambler** was implemented in the **receiver (PS)** side.
* A **frame structure** with a **header** was introduced for synchronization and frame detection.
* Each transmitted frame consists of:

  * A **header** (used to identify the start of a frame)
  * **512 data samples** of scrambled signal
* The header type (e.g., CCS/DS or custom ASM) can be chosen freely.
* The receiver waits for the header to identify the start of a frame and then descrambles the received data using the same polynomial as the scrambler.
* The **DVB-S2 standard** scrambler algorithm was suggested as a reference.

### Tools and Components

* **ASM/State Machine Design** for Scrambler/Descrambler
* **AXI Stream Interfaces** for data transfer
* **C/C++ control software** for frame management
* Optional use of **internet references** or DVB-S2 documentation for scrambler design

---

## Summary

| Phase       | Focus                                   | Key Components                                                 |
| ----------- | --------------------------------------- | -------------------------------------------------------------- |
| **Phase 1** | Signal generation and FFT visualization | PS waveform generation, DMA transfer, FFT IP, HDMI output      |
| **Phase 2** | Data scrambling and descrambling        | Scrambler/Descrambler design, frame structure, synchronization |

---

## Future Work (Phase 3 – Optional)

A third phase (optional) involves transmitting the signal via **UART** to the Zynq and processing it in **MATLAB** for additional analysis.

---

## Contributors

This project was developed by students of the **Electrical Engineering Department, Sharif University of Technology**, under the supervision of **Dr. Mehdi Shabani**.

