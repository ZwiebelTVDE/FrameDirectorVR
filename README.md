# Frame Director VR

Place one or more HDMI / SDI capture feeds as numbered screens in a small, warm studio and move them with your hands.

## Requirements

- Godot **4.5** or later (Forward Plus). Tested with 4.5.1; the project file was last saved in 4.7
- Webcams and capture cards on Windows: nothing extra, the capture plugin ships prebuilt (see [Capture plugin](#capture-plugin))
- DELTACAST SDI cards: the DELTACAST VideoMaster SDK 6.30 or later, Visual Studio 2022 and CMake 3.20+ to build the plugin yourself
- An OpenXR runtime (SteamVR, Quest Link/Air Link, Windows Mixed Reality) for headset mode

## Open the project

1. Install [Godot 4.5+](https://godotengine.org/download)
2. Import `FrameDirectorVR` in the Project Manager
3. Press **F5** to run

Without a headset, the app starts a desktop fly camera so you can still place screens and edit settings.

## Capture inputs

Each channel in **Settings** is one screen in the room.

| Setting | What it does |
| --- | --- |
| Channel number | Large **CH n** badge on the screen plus a watermark on the picture |
| HDMI / SDI | Label for the physical input, shown on the screen and in the test pattern. It does not change how the device is opened |
| Resolution | 720p, 1080p, 1440p, 2160p. The device mode closest to this is used. For DELTACAST inputs the format comes from the signal instead |
| Framerate | 24 / 25 / 30 / 50 / 60. Used together with the resolution to pick the device mode |
| Capture device | Any device Godot sees through `CameraServer`, or a simulated test pattern. Remembered by name, so it reconnects after a re-plug |
| Flip vertically | For drivers that deliver frames upside down |
| Visible | Hides the screen. Hidden screens can't be grabbed |

Each channel card shows a status line: the device name, or "not found · showing test pattern" when a saved device is unplugged. Unplugged devices are kept in the list as "(not connected)" and reconnect automatically when they come back.

RGB and YCbCr (4:2:2 packed or planar) frames are converted to RGB on the GPU using BT.709 limited range, which is what HDMI and SDI sources normally send.

Settings are saved to `user://frame_director_settings.json`, including when the window is closed.

### Which capture cards work

| Device | Windows | Linux / macOS |
| --- | --- | --- |
| Webcams and standard capture cards (for example Elgato HD60 Pro) | Through the capture plugin (Media Foundation), included prebuilt. Not yet tested on real hardware | Through Godot's own camera support. Not tested |
| DELTACAST SDI cards (for example DELTA-3G-elp-d 40) | Through the capture plugin, after building it with the VideoMaster SDK. Not yet tested on real hardware | Plugin code is cross-platform but untested |
| OBS Virtual Camera | Shows up only if Windows lists it as a Media Foundation device, which depends on the OBS version. Not tested | Through Godot's camera support. Not tested |
| Blackmagic DeckLink | Only if its driver offers a standard Windows video device. Not tested | Needs a DeckLink SDK backend. Not implemented |

A quick check for Windows: if a device works in the **Windows Camera** app, the capture plugin should see it too.

Without any capture device, every channel can still use the simulated test pattern.

### Capture plugin

Godot has no camera support on Windows (its Windows camera backend is an empty placeholder up to and including 4.7.1). The native plugin (GDExtension) in `native/capture/` fills that gap and adds DELTACAST cards. It has two backends:

**Media Foundation (Windows)** — webcams, Elgato and other standard capture cards.

- Ships prebuilt: `addons/capture/bin/capture.windows.x86_64.dll`. Nothing to install; it only uses Windows' own Media Foundation.
- Every video capture device appears in the **Capture device** list under its Windows name (for example `Game Capture HD60 Pro`). Identical models get `(2)`, `(3)`... appended.
- Devices plugged in while the app runs appear within about 2 seconds. A device that is unplugged while in use falls back to the test pattern and reconnects when it comes back.
- The channel's **Resolution** and **Framerate** pick the closest mode the device offers. The status line shows what was chosen, for example `1920x1080 @ 60 fps (NV12 > YUY2)`.
- Compressed webcam formats (MJPG, H.264) and other pixel formats are converted to YUY2 by Windows; the picture is then converted to RGB on the GPU.

**DELTACAST** — SDI (and HDMI/DisplayPort) cards through the VideoMaster SDK. Based on DELTACAST's own capture sample, [videomaster-video-monitor](https://github.com/deltacasttv/videomaster-video-monitor) (Apache-2.0).

- Every input of every DELTACAST card appears in the **Capture device** list, for example `DELTACAST DELTA-3G-elp-d 40 #0 · SDI In 1`. Choosing one sets the channel to SDI.
- The input format is detected from the signal (for example 1080p60 or 720p50). If the source changes format, or the cable is pulled and plugged back in, capture restarts on its own.
- The status line shows `Waiting for signal`, `Signal lost`, or the detected format. Screens without a signal stay dark.
- A single 3G-SDI input carries at most 1080p60.
- Not included in the prebuilt DLL, because it needs the VideoMaster SDK. Build the plugin yourself to add it (below).

In both backends, capture only runs while a device is selected on a channel, on its own thread per device. If the plugin is missing, the project still runs (test pattern only on Windows).

#### Build the plugin yourself (adds DELTACAST)

1. Install the DELTACAST driver and the **VideoMaster SDK 6.30 or later**, following DELTACAST's instructions. The SDK's CMake package must be findable, as for DELTACAST's own samples.
2. Install **Visual Studio 2022** with "Desktop development with C++", **CMake 3.20+**, **Git** and **Python 3** (godot-cpp uses it to generate code).
3. Close the Godot editor (it locks the DLL), then in a terminal, from the project folder:
   ```
   cd native/capture
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```
   The configure step prints `DELTACAST backend enabled` when it found the SDK. The first build downloads godot-cpp and takes a few minutes.
4. The build replaces `addons/capture/bin/capture.windows.x86_64.dll` and rewrites `addons/capture/capture.gdextension`.
5. Run the project. The output log shows `Media Foundation: n video device(s) found` and `DELTACAST: 4 input(s) registered`.

The VideoMaster DLLs installed with the SDK must be findable at runtime (normally they are on the system path after installation). If Godot reports that the plugin library can't be loaded, copy the VideoMaster DLLs next to `capture.windows.x86_64.dll`.

Options: `-DCAPTURE_DELTACAST=OFF` builds without DELTACAST even if the SDK is installed; `-DCAPTURE_DELTACAST=ON` stops with an error if it isn't found.

#### Testing without hardware

`cmake -B build -DDELTACAST_MOCK=ON` builds the DELTACAST backend against a simulated 4-input 3G-SDI board (`native/capture/mock/`): In 1 sends 1080p60 color bars, In 2 720p50, In 3 loses and regains its signal every 4 seconds, and In 4 has no signal. This is how the DELTACAST backend was tested so far. The simulated board only stands in for the parts of the SDK the plugin uses, so a real SDK build may still need small fixes.

The prebuilt Windows DLL was cross-compiled with MinGW and checked to load in Godot 4.5.1 for Windows (under Wine, which has no cameras). Capture from real Windows devices has not been tested yet.

### Troubleshooting

- **No capture devices on Windows:** check the output log for `Media Foundation: n video device(s) found`. If that line is missing, the plugin didn't load: make sure `addons/capture/bin/capture.windows.x86_64.dll` and `addons/capture/capture.gdextension` exist, then restart the editor.
- **Device listed but "Not available":** another program (OBS, Elgato's software, a video call) is probably using it. Close that program; the app retries every second. Windows' camera privacy setting ("Let desktop apps access your camera") must also be on.
- **Webcam colors slightly off:** the GPU conversion assumes HD video (BT.709). Some webcams use the SD standard (BT.601), which makes colors look a little different.
- **DELTACAST inputs missing:** check the output log for a `DELTACAST` message. "Could not enumerate" or "Could not open" means the driver or SDK isn't installed correctly, or another program (OBS, DELTACAST tools) is using the card.
- **DELTACAST input stays on "Waiting for signal":** check the cable and that the source outputs a format the card supports.
- **"No GDExtension library found" on Linux / macOS:** harmless. The included plugin is Windows-only; build it from `native/capture/` to add DELTACAST there, or ignore the message.
- **Device missing on Linux / macOS:** close OBS and the manufacturer's capture software. Most cards can only be used by one program at a time. Devices may take a moment to appear; the list refreshes on its own.
- **Black picture with an HDMI source:** the source may be copy-protected (HDCP), for example a game console with HDCP on or a streaming box.
- **Upside-down picture:** turn on **Flip vertically**.
- **Interlaced sources (1080i):** not verified yet. If the picture looks like two squashed half-height images or combs heavily, please report it.
- **Wrong colors or garbled picture:** note the card, the source format and your OS, and open an issue.

## Controls

**Desktop**

- **WASD** move, **Q / E** up / down (disabled while typing in a text field)
- **Right-click** toggle mouse look
- **Left-click** grab the screen under the cursor (screen centre while mouse look is on), **wheel** push/pull
- Click into the 3D view to leave a text field and get movement back
- **Tab** or **F1** settings, **+** add a channel, **Quit app** in settings to exit

**VR**

- **Grip** grab and place a screen
- **Trigger** click the settings panel
- **Menu / A / X / B / Y** toggle settings

## Project layout

- `scenes/main.tscn` — entry scene
- `scripts/` — VR rig, capture feeds, grab pointers, settings UI
- `shaders/simulated_feed.gdshader` — color-bar fallback when no device is selected
- `shaders/camera_feed.gdshader` — converts RGB / YCbCr capture frames for display
- `native/capture/` — source of the capture plugin (C++): `src/mediafoundation/`, `src/deltacast/`, shared code in `src/common/`, simulated DELTACAST board in `mock/`
- `addons/capture/` — the built plugin (prebuilt Windows DLL included)

## Changelog

### Unreleased

**Windows webcams and capture cards**

- Added: Media Foundation backend, so webcams, Elgato and other standard capture cards work on Windows. Included as a prebuilt DLL.
- Added: devices plugged in or removed while the app runs are picked up automatically; a removed device falls back to the test pattern and reconnects when it returns.
- Changed: the DELTACAST plugin is now part of one capture plugin (`native/capture/`, `addons/capture/`). DELTACAST is built in automatically when the VideoMaster SDK is installed.

**DELTACAST support**

- Added: native plugin for DELTACAST SDI (and HDMI/DisplayPort) capture cards via the VideoMaster SDK, based on DELTACAST's videomaster-video-monitor sample. Every card input shows up as a capture device.
- Added: automatic format detection, restart on format change or signal loss, live status per channel, dark screen while waiting for a signal.
- Added: simulated 4-input board for building and testing without the SDK.
- Changed: the device list refreshes when devices are added or removed.
- Corrected: the README previously said standard HDMI cards such as the Elgato HD60 Pro were expected to work on Windows. Godot has no Windows camera backend, so they don't yet.

**Capture devices**

- Fixed: capture devices never appeared in the list because `CameraServer` monitoring was off.
- Fixed: selecting a real device crashed because of a call to a nonexistent `CameraFeed.get_format()`. The device mode is now picked from `get_formats()` by closest resolution and framerate.
- Added: GPU conversion for YCbCr frames, which most capture cards deliver. Before, those showed black or garbled.
- Added: devices are remembered by name instead of index and reconnect automatically after a re-plug.
- Added: status line per channel, "(not connected)" entries for unplugged devices, and a **Flip vertically** option.

**Settings and screens**

- Fixed: typing a channel name restarted the capture feed on every keystroke. Name, visibility and channel number changes now only update the labels.
- Fixed: hidden screens could still be grabbed.
- Fixed: closing the window didn't save screen positions.
- Changed: the simulated test pattern renders at most 720p internally, to save GPU time in VR.

**Desktop and VR**

- Fixed: left-click always grabbed the screen in the centre of the view instead of the one under the cursor.
- Fixed: WASD moved the camera while typing in text fields.
- Changed: screens stay upright while dragged with the mouse.
- Fixed: the VR settings panel now redraws reliably and is no longer slightly stretched.
