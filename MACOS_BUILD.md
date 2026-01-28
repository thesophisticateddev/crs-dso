# macOS Build Instructions

## Prerequisites

- **macOS 11 (Big Sur) or later**
- **Xcode Command Line Tools**
- **Qt 6.5+** with the following modules:
  - Qt Quick
  - Qt Charts
- **CMake 3.16+**

## Installing Dependencies

### Install Xcode Command Line Tools

```bash
xcode-select --install
```

### Option 1: Homebrew (Recommended)

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Qt6 and CMake
brew install qt@6 cmake

# Add Qt to PATH (add to ~/.zshrc for persistence)
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"
```

For Intel Macs, the path is `/usr/local/opt/qt@6` instead of `/opt/homebrew/opt/qt@6`.

### Option 2: Qt Online Installer

1. Download the Qt Online Installer from [qt.io](https://www.qt.io/download-qt-installer)
2. Run the installer and select:
   - Qt 6.5+ for macOS
   - Qt Quick (included by default)
   - Qt Charts (under Additional Libraries)
3. Note the installation path (e.g., `~/Qt/6.5.0/macos`)

## Building

### Debug Build

```bash
cd /path/to/crs-dso
mkdir -p build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

The application bundle will be at `build/src/crs-dso.app`.

### Release Build

```bash
cd /path/to/crs-dso
mkdir -p build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
```

The application bundle will be at `build-release/src/crs-dso.app`.

### Using Ninja (Faster Builds)

```bash
brew install ninja

cd /path/to/crs-dso
mkdir -p build-release && cd build-release
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### Using CMake Presets

```bash
cmake --preset release
cmake --build --preset release
```

### Specifying Qt Path Manually

If CMake cannot find Qt, specify the path:

```bash
# Homebrew (Apple Silicon)
cmake -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6 ..

# Homebrew (Intel)
cmake -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@6 ..

# Qt Online Installer
cmake -DCMAKE_PREFIX_PATH=~/Qt/6.5.0/macos ..
```

## Running

```bash
# Run directly
./build-release/src/crs-dso.app/Contents/MacOS/crs-dso

# Or open as application
open build-release/src/crs-dso.app
```

## Creating a Distribution Package

### Using macdeployqt

Qt provides `macdeployqt` to bundle all required frameworks and plugins into the app bundle.

1. Deploy the application:
   ```bash
   macdeployqt build-release/src/crs-dso.app -qmldir=src/qml
   ```

2. Verify the app is self-contained:
   ```bash
   otool -L build-release/src/crs-dso.app/Contents/MacOS/crs-dso
   ```

### Creating a DMG (Disk Image)

```bash
macdeployqt build-release/src/crs-dso.app -qmldir=src/qml -dmg
```

This creates `build-release/src/crs-dso.dmg`.

### Creating a ZIP Archive

```bash
cd build-release/src
zip -r crs-dso-macos.zip crs-dso.app
```

### Manual DMG Creation (Custom Layout)

For a customized DMG with background image and Applications symlink:

```bash
# Create a temporary directory
mkdir -p dmg-contents
cp -R build-release/src/crs-dso.app dmg-contents/
ln -s /Applications dmg-contents/Applications

# Create the DMG
hdiutil create -volname "CRS-DSO" -srcfolder dmg-contents -ov -format UDZO crs-dso-macos.dmg

# Cleanup
rm -rf dmg-contents
```

## Code Signing and Notarization

For distribution outside the App Store, Apple requires code signing and notarization.

### Code Signing

1. Obtain a Developer ID certificate from Apple Developer Program

2. Sign the application:
   ```bash
   codesign --deep --force --verify --verbose \
       --sign "Developer ID Application: Your Name (TEAM_ID)" \
       --options runtime \
       build-release/src/crs-dso.app
   ```

3. Verify the signature:
   ```bash
   codesign --verify --verbose build-release/src/crs-dso.app
   ```

### Notarization

1. Create a ZIP for notarization:
   ```bash
   ditto -c -k --keepParent build-release/src/crs-dso.app crs-dso-notarize.zip
   ```

2. Submit for notarization:
   ```bash
   xcrun notarytool submit crs-dso-notarize.zip \
       --apple-id "your@email.com" \
       --team-id "TEAM_ID" \
       --password "app-specific-password" \
       --wait
   ```

3. Staple the notarization ticket:
   ```bash
   xcrun stapler staple build-release/src/crs-dso.app
   ```

4. Create the final DMG after stapling:
   ```bash
   macdeployqt build-release/src/crs-dso.app -qmldir=src/qml -dmg
   ```

## Universal Binary (Intel + Apple Silicon)

To create a universal binary that runs natively on both Intel and Apple Silicon Macs:

```bash
mkdir -p build-universal && cd build-universal
cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
```

Verify the architectures:
```bash
lipo -info build-universal/src/crs-dso.app/Contents/MacOS/crs-dso
```

## Troubleshooting

### CMake cannot find Qt6

Set the `CMAKE_PREFIX_PATH`:

```bash
# Homebrew (Apple Silicon)
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"

# Homebrew (Intel)
export CMAKE_PREFIX_PATH="/usr/local/opt/qt@6"

# Qt Online Installer
export CMAKE_PREFIX_PATH="$HOME/Qt/6.5.0/macos"
```

### "App is damaged and can't be opened"

This happens when the app is not signed or notarized. For testing locally:

```bash
xattr -cr /path/to/crs-dso.app
```

For distribution, properly sign and notarize the application.

### QML module not found

Ensure `macdeployqt` is run with the `-qmldir` flag:

```bash
macdeployqt crs-dso.app -qmldir=src/qml
```

### Library not loaded errors

Check which libraries are missing:

```bash
otool -L crs-dso.app/Contents/MacOS/crs-dso
```

Re-run `macdeployqt` to fix library paths:

```bash
macdeployqt crs-dso.app -qmldir=src/qml
```

### Minimum macOS version

To set a specific minimum macOS version:

```bash
cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 ..
```
