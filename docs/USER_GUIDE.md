# CRS-DSO User Guide

CRS-DSO is a dual-channel desktop oscilloscope application with two ways to work:

- Test Mode uses the built-in signal generator so you can explore the UI without hardware.
- Device Mode connects to a supported USB oscilloscope device.

## Quick Start

1. Launch the application.
2. Confirm the status bar shows `TEST MODE`.
3. Use `RUN` or `STOP` to start or pause the live waveform.
4. Adjust `TIME/DIV`, trigger source, trigger edge, and trigger level.
5. Change channel scale, coupling, and vertical position for `CH1` and `CH2`.
6. Drag the trigger marker or cursor markers directly on the chart for faster setup.

The application starts in Test Mode by default, so you can verify the UI immediately even without a device attached.

## Main Screen

The window is split into two main areas:

- The waveform display shows both channels, the trigger level, and two horizontal measurement cursors.
- The right-side control panel contains acquisition, trigger, horizontal, and per-channel controls.
- The top menu bar lets you switch between Test Mode and Device Mode, scan for devices, and start or stop acquisition.
- The status bar at the top of the window shows the current mode and connection state.

## Controls Overview

### Acquisition

- `Auto` continuously updates the display.
- `Normal` waits for a trigger event before showing an acquisition.
- `Single` captures one acquisition and then stops.
- `RUN` starts acquisition.
- `STOP` pauses acquisition.

### Trigger

- `Source` selects `CH 1` or `CH 2`.
- `Edge` selects `Rising` or `Falling`.
- `Level` can be adjusted with the slider or by dragging the yellow trigger handle on the chart.

### Horizontal

- `TIME/DIV` changes the horizontal time scale using standard 1-2-5 steps.
- `H. Position` pans the displayed time window left or right.

Note: horizontal position currently affects the displayed chart window. It should not be treated as guaranteed hardware time-offset control.

### Channel Controls

Each channel includes:

- `AC`, `DC`, and `GND` coupling selection.
- `VOLTS/DIV` adjustment.
- `Position` slider for vertical offset.
- `Show CH1` or `Show CH2` to hide or reveal a trace.

### Cursors And Autoset

- `C1` and `C2` are horizontal voltage cursors.
- The `Delta V` readout shows the voltage difference between the two cursors.
- `AUTOSET` restores a known baseline setup:
  - `1 ms/div`
  - `1 V/div` on both channels
  - trigger on `CH 1`, rising edge, `0 V`
  - zero channel offsets
  - default cursor positions

## Working In Test Mode

Test Mode is the fastest way to learn the interface.

- No hardware is required.
- The application generates synthetic waveforms for both channels.
- Trigger controls and run/stop behavior are active.
- This mode is intended for UI previewing, demos, and general experimentation.

Current limitation: some channel and hardware-style controls are stored in the UI but do not fully change the generated test waveform behavior. Treat Test Mode as a functional preview rather than a full simulation of hardware acquisition.

## Working In Device Mode

To use real hardware:

1. Connect the supported USB device.
2. Open `Mode -> Device Mode (Hardware)`.
3. Wait for the status message to show scanning and connection progress.
4. If needed, use `Mode -> Scan for Devices`.
5. Select acquisition settings and press `RUN`.

Important behavior:

- The app currently connects to the first discovered device automatically.
- There is no device picker yet.
- Status text is the main place to confirm whether the application is scanning, connected, armed, acquiring, stopped, or reporting an error.

### Demo Mode

You can force the hardware path to use the mock transport with:

```bash
./build/src/crs-dso --demoMode
```

This is useful when you want to exercise Device Mode behavior without attaching hardware.

## Hardware Setup Notes

### Linux

USB access usually requires a udev rule for non-root access. The repository includes `udev/99-crs-dso.rules`, but the vendor and product IDs are still placeholders and must be replaced before installing the rule.

After updating the IDs, install the rule:

```bash
sudo cp udev/99-crs-dso.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Windows

Bind the device to WinUSB before using hardware mode. The repository includes `driver/crs-dso.inf` as a starting point for driver packaging.

### macOS

Hardware mode is intended to work when the device uses a vendor-specific USB class.

## Known Limitations

- No waveform export, screenshot export, or settings save/recall workflow is available in the current UI.
- Device Mode auto-connects the first detected device instead of offering manual selection.
- Some status and diagnostic reporting are still basic.
- Test Mode does not mirror every hardware behavior exactly.

## Troubleshooting

### The app opens but no hardware is found

- Confirm the device is connected.
- Switch to `Device Mode` before scanning.
- On Linux, confirm the udev rule uses the correct USB vendor/product IDs.
- Confirm your build includes libusb support. If libusb was missing during configuration, the app is built in mock-only mode.

### I only want to verify the UI

Use the default Test Mode, or launch Device Mode with `--demoMode`.

### The status text says `No devices found`

- Reconnect the device and scan again.
- Check OS-specific USB driver permissions.
- Rebuild with libusb available if your current build is mock-only.

## Additional Documentation

- [Documentation Index](README.md)
- [Linux Build Instructions](LINUX_BUILD.md)
- [Windows Build Instructions](WINDOWS_BUILD.md)
- [macOS Build Instructions](MACOS_BUILD.md)
