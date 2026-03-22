# Simulation And Test Mode Plan

This document outlines the changes needed to turn the current test mode into a fuller oscilloscope simulation environment with configurable waveforms and proper scope controls.

## Goals

- Support multiple simulated waveform types, not just the current hardcoded traces.
- Add configurable noise and distortion so test mode can mimic real-world signals.
- Make sampling rate, sampling duration, horizontal shift, vertical shift, and trigger controls behave consistently in simulation and hardware modes.
- Separate true acquisition controls from display-only chart panning.
- Reduce divergence between the active `ScopeController` path and the older `DeviceController` API surface.

## Current State

### What exists today

- Test mode exists and is backed by `SignalGenerator` in `src/signalgenerator.cpp`.
- Trigger source, trigger edge, and trigger level are wired from QML through `ScopeController` into both test mode and hardware mode.
- Acquisition mode buttons (`Auto`, `Normal`, `Single`) exist in the UI.
- Channel volts/div, coupling, enable/disable, and vertical offset controls exist in the UI.
- A horizontal position slider exists in the UI.

### What is missing or incomplete

- Test mode only generates two fixed waveforms: sine-like CH1 and cosine-like CH2.
- There is no waveform selector, no noise control, no amplitude/frequency/phase settings, and no per-channel simulation profile.
- Sampling rate is supported by the hardware engine API but is not exposed through the active `ScopeController` + QML path.
- Sampling duration or record length is not exposed in the active UI.
- Horizontal shift is currently a chart-axis pan in QML, not a real scope acquisition control.
- Vertical offset is sent to hardware mode but is not applied in the signal generator.
- Acquisition mode is selected in the UI, but test mode does not implement true `Auto` / `Normal` / `Single` semantics.
- `--demoMode` uses `MockTransport`, but that mock transport does not generate waveform frames, so it is not a true simulated device backend.

## Evidence In The Code

- Hardcoded simulated waveforms: `src/signalgenerator.cpp:98`, `src/signalgenerator.cpp:102`
- Trigger detection in simulation: `src/signalgenerator.cpp:114`
- Fixed-size synthetic buffer and decimation: `src/signalgenerator.cpp:147`
- Active test/hardware controller surface: `src/core/ScopeController.h:16`, `src/core/ScopeController.cpp:206`
- Hardware sample-rate API exists but is unused by active UI: `src/core/AcquisitionEngine.h:42`
- UI horizontal position is display-only: `src/qml/Main.qml:573`
- UI vertical offset exists for both channels: `src/qml/Main.qml:649`, `src/qml/Main.qml:730`
- Active trigger controls in UI: `src/qml/Main.qml:455`, `src/qml/Main.qml:485`, `src/qml/Main.qml:515`

## Recommended Direction

Build a single shared scope-settings model that both simulation mode and hardware mode implement.

Instead of treating test mode as a special-case toy generator, treat it as a first-class backend with the same control contract as hardware mode.

## Proposed Architecture Changes

### 1. Introduce a shared settings model

Add a central settings structure, for example `ScopeSettings`, owned by `ScopeController` and consumed by either backend.

Suggested fields:

- acquisition mode
- sample rate
- record length or acquisition duration
- timebase
- horizontal position
- trigger source
- trigger edge
- trigger level
- trigger holdoff
- pre-trigger percentage
- per-channel enabled state
- per-channel volts/div
- per-channel coupling
- per-channel vertical offset
- per-channel simulation waveform config

This removes the current split where some controls live only in QML, some only in `ScopeController`, and some only in `AcquisitionEngine` or `DeviceController`.

### 2. Replace the fixed `SignalGenerator` model with a parameterized simulation backend

Refactor test mode so each channel can generate a chosen signal type with configurable parameters.

Minimum waveform types:

- sine
- square
- triangle
- sawtooth
- pulse
- DC level
- noise-only

Recommended per-channel parameters:

- amplitude
- frequency
- phase
- DC offset
- duty cycle for pulse/square
- noise amount
- optional harmonic distortion amount

The simulation should generate samples in physical time rather than from a simple point index. That is the key change needed to make sample rate, duration, and timebase meaningful.

### 3. Add a richer simulation profile layer

Support named presets so users can quickly switch scenarios.

Example presets:

- clean sine wave
- sine with Gaussian noise
- square wave with ringing
- dual-tone mixed signal
- pulse train
- clipped waveform
- drifting baseline
- trigger stress test

These presets should populate the same underlying simulation settings rather than bypassing them.

### 4. Unify acquisition semantics across backends

Test mode should implement:

- `Auto`: free-run if no trigger occurs within a timeout
- `Normal`: update only on valid trigger
- `Single`: arm once, capture once, then stop

The simulation backend should expose state transitions similar to hardware mode so the UI status bar behaves consistently.

### 5. Make horizontal shift a real modeled control

Today `H. Position` only shifts chart axes in QML.

Needed changes:

- Add a `horizontalPosition` property to `ScopeController`
- Store it in shared settings
- Apply it in simulation as pre-trigger/post-trigger positioning within the generated acquisition window
- Apply it in hardware only if the device/protocol supports it
- If hardware does not support it yet, expose a capability flag and label the control as display-only or disable it in hardware mode

This avoids promising a scope feature that is currently only a visual pan.

### 6. Make vertical shift real in simulation

Vertical offset already exists in the active UI and hardware path, but not in the generator.

Needed changes:

- Add per-channel offset handling in the simulation backend
- Apply offset after waveform generation and before trigger evaluation where appropriate
- Keep the offset semantics aligned with hardware mode as closely as possible

### 7. Expose proper sampling controls in the active UI

The application needs explicit controls for:

- sample rate
- acquisition duration or record length
- derived total samples

Recommended behavior:

- Let users choose either `sample rate + duration` or `sample rate + record length`
- Show derived values in the UI to avoid ambiguity
- Tie `TIME/DIV` to acquisition duration intentionally rather than indirectly

For example:

- total capture window = `10 * timePerDiv`
- record length = `sampleRate * totalCaptureWindow`

If the hardware has limits, the UI should clamp values and explain the active limit.

### 8. Expand trigger control beyond the current minimum

Current trigger support is better than most other controls, but it still needs expansion.

Recommended additions:

- trigger holdoff
- pre-trigger percentage
- force trigger action
- trigger status indicator (`Waiting`, `Triggered`, `Auto`, `Stopped`)
- trigger source options scoped to enabled channels only

Simulation should respect these controls, not just source/edge/level.

### 9. Introduce backend capability flags

Not every backend will support every control equally.

Add capability flags so the UI can react cleanly, for example:

- supports sample rate control
- supports record length control
- supports horizontal acquisition shift
- supports force trigger
- supports simulation waveform editor

This is preferable to leaving controls visible but only partially functional.

### 10. Reconcile `DeviceController` with the active path

`DeviceController` already contains parts of the API surface that the active `ScopeController` path lacks, especially around device settings.

Needed changes:

- move missing concepts from `DeviceController` into the active controller path
- avoid maintaining two separate control contracts
- retire or isolate unused legacy controller code once the active path is complete

## Proposed UI Changes

### Test Mode panel additions

Add a new section when running in test mode:

- simulation preset selector
- CH1 waveform selector
- CH2 waveform selector
- per-channel amplitude/frequency/phase controls
- per-channel noise slider
- per-channel DC offset
- optional advanced toggle for duty cycle and distortion

### Shared acquisition panel additions

Add or expose these controls in both modes where supported:

- sample rate selector or numeric input
- acquisition duration or record length selector
- horizontal position / pre-trigger control
- trigger holdoff
- force trigger button
- acquisition status display

### UI labeling cleanup

Clarify the difference between:

- acquisition controls
- display controls
- simulation-only controls

Also fix mismatched comments/defaults around timebase indexing, since the current comments describing `timebaseIndex = 12` as `1 ms/div` do not match the actual step table.

## Implementation Phases

### Phase 1: Control contract cleanup

- Add `ScopeSettings` and capability flags
- Add missing properties to `ScopeController`
- Wire sample rate, duration/record length, and horizontal position through the active controller
- Fix misleading comments/defaults in timebase-related code and docs

### Phase 2: Simulation backend refactor

- Replace hardcoded generator logic with waveform objects or strategy functions
- Generate samples in time units instead of display bucket indices
- Apply channel offset, amplitude, phase, and noise consistently
- Implement acquisition-mode behavior in simulation

### Phase 3: UI expansion

- Add simulation controls and presets
- Add explicit sampling controls
- Add force trigger and trigger status UI
- Use capability flags to enable/disable mode-specific controls cleanly

### Phase 4: Validation and scenario coverage

- Add manual verification scenarios for each waveform type
- Validate trigger behavior in `Auto`, `Normal`, and `Single`
- Validate sample-rate and duration interactions
- Validate horizontal and vertical shifts in both modes
- Validate disabled controls and capability messaging where hardware support is absent

## Acceptance Criteria

The plan should be considered complete when:

- Test mode supports multiple selectable waveform families
- Noise can be added per channel
- Sample rate and acquisition duration affect the generated waveform meaningfully
- Horizontal shift is either truly modeled or clearly marked as display-only
- Vertical shift works in both simulation and hardware paths
- Trigger controls behave consistently across modes
- `Auto`, `Normal`, and `Single` behave correctly in test mode
- The UI reflects backend capabilities instead of silently ignoring settings

## Suggested File Touch Points

- `src/core/ScopeController.h`
- `src/core/ScopeController.cpp`
- `src/signalgenerator.h`
- `src/signalgenerator.cpp`
- `src/qml/Main.qml`
- `src/core/AcquisitionEngine.h`
- `src/core/AcquisitionEngine.cpp`
- `src/core/DeviceSettings.h`
- `src/core/DeviceController.h`
- `src/core/DeviceController.cpp`

## Notes

- The quickest visible improvement is adding waveform selection and noise in test mode.
- The most important structural improvement is introducing a shared settings/capabilities contract so simulation and hardware stop drifting apart.
- Without that shared contract, adding more controls will likely create more UI-only or backend-only behavior.
