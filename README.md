# Frame Director VR

Place one or more HDMI / SDI capture feeds as numbered screens in a small, warm studio and move them with your hands.

## Requirements

- Godot **4.5** or later (Forward Plus). Tested with 4.5.1; the project file was last saved in 4.7
- Windows recommended for capture-card drivers
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
| Resolution | 720p, 1080p, 1440p, 2160p. The closest mode the device offers is used |
| Framerate | 24 / 25 / 30 / 50 / 60. Used together with the resolution to pick the device mode |
| Capture device | Any device Godot sees through `CameraServer`, or a simulated test pattern. Remembered by name, so it reconnects after a re-plug |
| Flip vertically | For drivers that deliver frames upside down |
| Visible | Hides the screen. Hidden screens can't be grabbed |

Each channel card shows a status line: the device name, or "not found · showing test pattern" when a saved device is unplugged. Unplugged devices are kept in the list as "(not connected)" and reconnect automatically when they come back.

RGB and YCbCr (4:2:2 packed or planar) frames are converted to RGB on the GPU using BT.709 limited range, which is what HDMI and SDI sources normally send.

Settings are saved to `user://frame_director_settings.json`, including when the window is closed.

### Which capture cards work

The app can only use devices that the operating system exposes as a standard video device (the same kind a webcam uses). On Windows, Godot reads these through Media Foundation.

| Device | Status |
| --- | --- |
| USB / PCIe HDMI capture cards that work as a "Video Capture Device" in OBS (for example Elgato HD60 Pro) | Expected to work. Not yet tested on real hardware |
| Blackmagic DeckLink (Windows) | May work through the Desktop Video driver's standard capture device. Not tested |
| Blackmagic DeckLink (Linux / macOS) | Needs a native plugin using the DeckLink SDK. Not implemented |
| Deltacast (for example DELTA-3G-elp-d 40) | Needs a native plugin using the Deltacast VideoMaster SDK. Planned, see below |

A quick check for Windows: if the card shows a picture in the **Windows Camera** app, Godot should be able to see it too.

SDI sources can also be used with an HDMI-only card by putting an SDI-to-HDMI converter in between.

### Troubleshooting

- **Device missing from the list:** close OBS and the manufacturer's capture software. Most cards can only be used by one program at a time. Devices may take a moment to appear after startup; the list refreshes on its own.
- **Black picture:** the source may be copy-protected (HDCP), for example a game console with HDCP on or a streaming box.
- **Upside-down picture:** turn on **Flip vertically**.
- **Wrong colors or garbled picture:** note the card, the source resolution/framerate and your OS, and open an issue.
- The Elgato HD60 Pro and a single 3G-SDI input both top out at 1080p60.

### Planned: Deltacast plugin

A GDExtension (C++) plugin using the Deltacast VideoMaster SDK for Windows. It will open each SDI input of the card on its own capture thread and register it as a normal camera feed, so every input appears in the **Capture device** list next to the other devices. The rest of the app does not need to change.

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

## Changelog

### Unreleased

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
