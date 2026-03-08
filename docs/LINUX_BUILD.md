# Linux Build Instructions

## Prerequisites

- **Qt 6.5+** with the following modules:
  - Qt Quick
  - Qt Charts
- **CMake 3.16+**
- **C++17 compatible compiler** (GCC 9+ or Clang 10+)
- **Make** or **Ninja** build system

### Installing Dependencies (Debian/Ubuntu)

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev qt6-charts-dev
```

### Installing Dependencies (Fedora)

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtcharts-devel
```

### Installing Dependencies (Arch Linux)

```bash
sudo pacman -S base-devel cmake qt6-base qt6-declarative qt6-charts
```

## Building

### Debug Build

```bash
cd /path/to/crs-dso
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

The executable will be at `build/src/crs-dso`.

### Release Build

```bash
cd /path/to/crs-dso
mkdir -p build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

The executable will be at `build-release/src/crs-dso`.

### Using CMake Presets (requires Ninja)

```bash
# Debug build
cmake --preset default
cmake --build --preset default

# Release build
cmake --preset release
cmake --build --preset release
```

## Running

```bash
./build-release/src/crs-dso
```

## Creating a Distribution Package

### Basic Tarball

```bash
mkdir -p dist/crs-dso-linux-x86_64
cp build-release/src/crs-dso dist/crs-dso-linux-x86_64/
cp LICENSE README dist/crs-dso-linux-x86_64/
cd dist && tar -czvf crs-dso-linux-x86_64.tar.gz crs-dso-linux-x86_64/
```

### Creating an AppImage (Portable)

For a fully portable build that bundles all Qt dependencies:

1. Download the deployment tools:
   ```bash
   wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
   wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
   chmod +x linuxdeploy*.AppImage
   ```

2. Create a `.desktop` file (e.g., `crs-dso.desktop`):
   ```ini
   [Desktop Entry]
   Type=Application
   Name=CRS-DSO
   Exec=crs-dso
   Icon=crs-dso
   Categories=Utility;
   ```

3. Build the AppImage:
   ```bash
   ./linuxdeploy-x86_64.AppImage \
       --appdir AppDir \
       --plugin qt \
       --executable build-release/src/crs-dso \
       --desktop-file crs-dso.desktop \
       --output appimage
   ```

## Troubleshooting

### CMake cannot find Qt6

Ensure Qt6 is installed and its path is in `CMAKE_PREFIX_PATH`:

```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64 ..
```

### QML module not found at runtime

Set the `QML2_IMPORT_PATH` environment variable:

```bash
export QML2_IMPORT_PATH=/usr/lib/x86_64-linux-gnu/qt6/qml
./crs-dso
```

### Missing OpenGL libraries

Install OpenGL development packages:

```bash
# Debian/Ubuntu
sudo apt install libgl1-mesa-dev

# Fedora
sudo dnf install mesa-libGL-devel
```
