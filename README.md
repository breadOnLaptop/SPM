# Smart Process Manager (SPM)

**Smart Process Manager (SPM)** is a powerful, lightweight, and cross-platform system management tool written in C++11. It provides a modern graphical interface for monitoring processes, managing system resources, and analyzing hardware performance in real-time.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C++-11-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)

## Features

### Modern Dashboard
- **Interactive Process Table**: Real-time list of all running processes with PID, Name, Status, Priority, Memory Usage, and Owner.
- **Search & Filter**: Instantly find processes by name or owner using the integrated search bar.
- **Vibrant UI**: Built with Dear ImGui, featuring a "Task Manager" style dark theme with rounded corners and high readability.

### Process Control & Safety
- **Tiered Termination**: Uses a "Graceful-then-Hard" strategy (WM_CLOSE/SIGTERM followed by a fallback kill) to prevent data loss.
- **Priority Management**: Change process priority levels (Idle to High) on the fly.
- **System Protection**: Automatically protects critical system/root processes by disabling management actions for them.

### Performance Monitoring
- **Real-time Graphs**: Dedicated Performance tab with 2D scrolling graphs for global CPU and RAM usage.
- **Asynchronous Updates**: Background thread-safe snapshotting ensures the UI remains responsive regardless of system load.

### Detailed Logging
- **Implementation Logs**: Tracks application events and errors.
- **Process Snapshots**: Logs formatted process tables to dedicated files for historical analysis.

## Build Instructions

### Prerequisites
- **CMake** (3.10 or newer)
- **Compiler**: 
  - Windows: MinGW-w64 (64-bit) or MSVC
  - Linux: GCC/G++ (Requires `build-essential`, `libglfw3-dev`, `libgl1-mesa-dev`, `libx11-dev`, etc.)

### Building from Source

#### Windows (MinGW)
1. **Clone the repository**:
   ```bash
   git clone https://github.com/breadOnLaptop/SPM.git
   cd SPM
   ```
2. **Generate build files**:
   ```bash
   mkdir build
   cd build
   cmake -G "MinGW Makefiles" ..
   ```
3. **Compile**:
   ```bash
   mingw32-make
   ```

#### Linux
1. **Install dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt update
   sudo apt install build-essential cmake libglfw3-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
   ```
2. **Build**:
   ```bash
   cd SPM
   mkdir build
   cd build
   cmake ..
   make
   ```
3. **Run**:
   ```bash
   ./spm
   ```

## Repository Structure
```
SPM/
├── include/       # Core backend & UI headers
├── src/           # Implementation logic
├── third_party/   # Vendored Dear ImGui & GLFW
├── CMakeLists.txt # Build configuration
└── README.md
```

## License
This project is licensed under the [MIT License](LICENSE).
