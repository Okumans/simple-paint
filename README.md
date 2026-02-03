# Simple Paint

Simple Paint is a high-performance, infinite canvas painting application built with C++ and OpenGL 4.5. It features a stroke-based rendering engine, a custom UI system with spatial optimization, and a robust input handling architecture.

## Features

*   **Infinite Canvas:** Pan and zoom freely without boundaries.
*   **Stroke Rendering:** Smooth, antialiased strokes with variable thickness.
*   **Input Handling:** Hybrid input model combining polling for continuous actions (panning, drawing) and callbacks for discrete events (shortcuts, UI clicks).
*   **Custom UI:** Efficient UI manager using sweep-and-prune spatial hashing for O(log n) hit testing. Supports both solid color and textured elements.
*   **Undo/Redo:** Full history support for strokes.
*   **Performance:** Uses OpenGL 4.5 Direct State Access (DSA) and optimized batch rendering.

## Controls

| Action | Control |
| :--- | :--- |
| **Draw** | Left Mouse Button |
| **Pan** | Middle Mouse Button (Hold) OR Space + Left Mouse Button OR Left + Right Mouse Button |
| **Zoom** | Scroll Wheel OR Ctrl + Left Mouse Button (Drag) |
| **Undo** | Ctrl + Z |
| **Redo** | Ctrl + R |
| **Reset View** | '0' Key |
| **Increase Brush Size** | Ctrl + '+' (Equal) |
| **Decrease Brush Size** | Ctrl + '-' (Minus) |
| **Select Color** | Click on UI Color Swatches |

## Building the Project

This project uses **CMake** for build configuration and **CPM** for dependency management.

### Prerequisites

*   CMake 3.14+
*   C++17 compliant compiler
*   OpenGL 4.5 capable graphics driver

### Instructions

1.  Clone the repository.
2.  Create a build directory:
    ```bash
    mkdir build && cd build
    ```
3.  Configure the project:
    ```bash
    cmake ..
    ```
4.  Build:
    ```bash
    make
    ```
5.  Run the application:
    ```bash
    ./bin/simple-paint
    ```

## Architecture Highlights

*   **Hybrid Input Model:**
    *   **Polling (`process_input`):** Handles continuous states like panning and drawing to ensure smooth frame-rate independent updates.
    *   **Callbacks (`glfw_key_callback`):** Handles discrete events like Undo/Redo to prevent "rapid-fire" accidental triggers.
*   **UI System:**
    *   `UIManager` maintains `UIHitbox` keys in a `std::map`.
    *   Hit testing uses `std::map::upper_bound` and a backward iteration sweep for efficient spatial queries.
    *   Render pipeline uses a simple quad batcher with screen-space orthographic projection.
