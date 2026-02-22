CRS-DSO Application Icons
==========================

This directory contains icon configuration files for all platforms.
You need to provide the actual icon files as described below.

Required Icon Files
-------------------

1. crs-dso.png (Required for all platforms)
   - Format: PNG with transparency
   - Size: 256x256 pixels (minimum), 512x512 or 1024x1024 recommended
   - Used for: Linux desktop, application window icon, Qt resources

2. crs-dso.ico (Required for Windows)
   - Format: Windows ICO format
   - Should contain multiple sizes: 16x16, 32x32, 48x48, 64x64, 128x128, 256x256
   - Used for: Windows executable icon, taskbar, explorer

3. crs-dso.icns (Required for macOS)
   - Format: Apple ICNS format
   - Should contain sizes: 16x16, 32x32, 128x128, 256x256, 512x512, 1024x1024
   - Used for: macOS app bundle, Dock, Finder

Generating Icon Files
---------------------

From a high-resolution PNG (1024x1024 recommended):

Linux (PNG):
  Simply use your source PNG resized to 256x256 or higher.

Windows (ICO):
  Option 1 - Using ImageMagick:
    convert crs-dso.png -define icon:auto-resize=256,128,64,48,32,16 crs-dso.ico

  Option 2 - Online converter:
    Use https://convertio.co/png-ico/ or similar

macOS (ICNS):
  Option 1 - Using iconutil (macOS only):
    mkdir crs-dso.iconset
    sips -z 16 16     crs-dso.png --out crs-dso.iconset/icon_16x16.png
    sips -z 32 32     crs-dso.png --out crs-dso.iconset/icon_16x16@2x.png
    sips -z 32 32     crs-dso.png --out crs-dso.iconset/icon_32x32.png
    sips -z 64 64     crs-dso.png --out crs-dso.iconset/icon_32x32@2x.png
    sips -z 128 128   crs-dso.png --out crs-dso.iconset/icon_128x128.png
    sips -z 256 256   crs-dso.png --out crs-dso.iconset/icon_128x128@2x.png
    sips -z 256 256   crs-dso.png --out crs-dso.iconset/icon_256x256.png
    sips -z 512 512   crs-dso.png --out crs-dso.iconset/icon_256x256@2x.png
    sips -z 512 512   crs-dso.png --out crs-dso.iconset/icon_512x512.png
    sips -z 1024 1024 crs-dso.png --out crs-dso.iconset/icon_512x512@2x.png
    iconutil -c icns crs-dso.iconset

  Option 2 - Using png2icns (Linux):
    png2icns crs-dso.icns crs-dso.png

  Option 3 - Online converter:
    Use https://cloudconvert.com/png-to-icns

Configuration Files (Already Set Up)
------------------------------------

- icons.qrc        : Qt resource file for embedding PNG icon
- crs-dso.rc       : Windows resource file for embedding ICO in executable
- Info.plist       : macOS bundle configuration with icon reference
- crs-dso.desktop  : Linux desktop entry file

Build Integration
-----------------

The CMakeLists.txt has been configured to:
- Embed the icon in the Qt resource system (all platforms)
- Include the ICO in the Windows executable
- Set up the macOS app bundle with ICNS icon
- Install desktop file and icons on Linux
