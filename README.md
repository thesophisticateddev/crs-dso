# CRS-DSO

CRS-DSO is a dual-channel desktop digital oscilloscope application built with C++17, Qt Quick, and Qt Charts. It supports both a built-in test signal generator and a USB hardware mode.

## Documentation

- [User Guide](docs/USER_GUIDE.md)
- [Documentation Index](docs/README.md)
- [Linux Build Instructions](docs/LINUX_BUILD.md)
- [Windows Build Instructions](docs/WINDOWS_BUILD.md)
- [macOS Build Instructions](docs/MACOS_BUILD.md)

## Features

- Dual-channel waveform display
- Test Mode for UI preview and demos
- Device Mode for USB hardware acquisition
- Trigger source, edge, and level controls
- Timebase and per-channel volts/div controls
- Horizontal and vertical positioning controls
- Basic cursor-based voltage measurement

## Quick Start

### Run In Test Mode

```bash
cmake --preset default
cmake --build --preset default
./build/src/crs-dso
```

### Force Device-Mode Demo Path

```bash
./build/src/crs-dso --demoMode
```

## Build Requirements

- Qt 6.5+ with Quick, Charts, and Concurrent
- CMake 3.16+
- C++17 compiler
- `libusb-1.0` for USB hardware support

If `libusb-1.0` is not available during configuration, CRS-DSO builds in mock-only mode.

## Linux Dependencies

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev qt6-charts-dev libusb-1.0-0-dev
```

For platform-specific packaging and build details, use the docs linked above.
