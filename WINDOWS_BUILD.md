# Windows Build Instructions

## Prerequisites

- **Qt 6.5+** with the following modules:
  - Qt Quick
  - Qt Charts
- **CMake 3.16+**
- **C++17 compatible compiler**:
  - Microsoft Visual Studio 2019 or later (MSVC), or
  - MinGW-w64 11+

## Installing Dependencies

### Option 1: Qt Online Installer (Recommended)

1. Download the Qt Online Installer from [qt.io](https://www.qt.io/download-qt-installer)
2. Run the installer and select:
   - Qt 6.5+ for your compiler (MSVC or MinGW)
   - Qt Quick (included by default)
   - Qt Charts (under Additional Libraries)
3. Note the installation path (e.g., `C:\Qt\6.5.0\msvc2019_64`)

### Option 2: vcpkg

```powershell
vcpkg install qt6-base qt6-declarative qt6-charts --triplet=x64-windows
```

### Option 3: MSYS2 (MinGW)

```bash
pacman -S mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-declarative mingw-w64-x86_64-qt6-charts mingw-w64-x86_64-cmake
```

## Building

### Using Visual Studio (MSVC)

1. Open **Developer Command Prompt for VS** or **x64 Native Tools Command Prompt**

2. Configure and build:
   ```cmd
   cd C:\path\to\crs-dso
   mkdir build-release
   cd build-release
   cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64 -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . --config Release
   ```

3. The executable will be at `build-release\src\Release\crs-dso.exe`

### Using MinGW

1. Open a terminal with MinGW in PATH (e.g., MSYS2 MinGW64 shell or Qt MinGW shell)

2. Configure and build:
   ```bash
   cd /c/path/to/crs-dso
   mkdir -p build-release && cd build-release
   cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/Qt/6.5.0/mingw_64 -DCMAKE_BUILD_TYPE=Release ..
   mingw32-make -j%NUMBER_OF_PROCESSORS%
   ```

3. The executable will be at `build-release/src/crs-dso.exe`

### Using Ninja (Faster Builds)

```cmd
cd C:\path\to\crs-dso
mkdir build-release
cd build-release
cmake -G Ninja -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64 -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### Using CMake Presets

Edit `CMakePresets.json` to add your Qt path, then:

```cmd
cmake --preset release
cmake --build --preset release
```

## Running

```cmd
build-release\src\Release\crs-dso.exe
```

If you get missing DLL errors, see the Deployment section below.

## Creating a Distribution Package

### Using windeployqt

Qt provides `windeployqt` to automatically copy required DLLs and plugins.

1. Create a distribution folder:
   ```cmd
   mkdir dist\crs-dso-windows-x64
   copy build-release\src\Release\crs-dso.exe dist\crs-dso-windows-x64\
   ```

2. Run windeployqt:
   ```cmd
   C:\Qt\6.5.0\msvc2019_64\bin\windeployqt.exe --release --qmldir src\qml dist\crs-dso-windows-x64\crs-dso.exe
   ```

3. Copy additional files:
   ```cmd
   copy LICENSE dist\crs-dso-windows-x64\
   copy README dist\crs-dso-windows-x64\
   ```

4. The `dist\crs-dso-windows-x64` folder now contains everything needed to run the application.

### Creating a ZIP Archive

```powershell
Compress-Archive -Path dist\crs-dso-windows-x64 -DestinationPath dist\crs-dso-windows-x64.zip
```

### Creating an Installer (Optional)

For creating a professional installer, consider using:

- **Inno Setup**: Free, script-based installer creator
- **NSIS**: Nullsoft Scriptable Install System
- **WiX Toolset**: Windows Installer XML

Example Inno Setup script (`setup.iss`):

```ini
[Setup]
AppName=CRS-DSO
AppVersion=1.0.0
DefaultDirName={autopf}\CRS-DSO
DefaultGroupName=CRS-DSO
OutputBaseFilename=crs-dso-setup
Compression=lzma2
SolidCompression=yes

[Files]
Source: "dist\crs-dso-windows-x64\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\CRS-DSO"; Filename: "{app}\crs-dso.exe"
Name: "{commondesktop}\CRS-DSO"; Filename: "{app}\crs-dso.exe"
```

Compile with: `iscc setup.iss`

## Troubleshooting

### CMake cannot find Qt6

Ensure `CMAKE_PREFIX_PATH` points to your Qt installation:

```cmd
cmake -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64 ..
```

Or set the environment variable:

```cmd
set CMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64
cmake ..
```

### Missing DLL errors at runtime

Run `windeployqt` to copy all required DLLs, or manually add the Qt `bin` directory to your PATH:

```cmd
set PATH=C:\Qt\6.5.0\msvc2019_64\bin;%PATH%
crs-dso.exe
```

### QML module not found

Ensure `windeployqt` is run with the `--qmldir` flag pointing to your QML sources:

```cmd
windeployqt --qmldir src\qml crs-dso.exe
```

### MSVC Runtime errors

If users get MSVC runtime errors, they need to install the Visual C++ Redistributable:
- Download from [Microsoft](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)
- Or include `vc_redist.x64.exe` with your installer

### MinGW builds require MinGW runtime DLLs

When distributing MinGW builds, include these DLLs from your MinGW `bin` folder:
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`
