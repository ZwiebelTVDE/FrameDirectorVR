# Frame Director VR

Place one or more HDMI / SDI capture feeds as numbered screens in a small, warm studio and move them with your hands.

## Requirements

- Godot **4.5** or later (Forward Plus). Tested with 4.5.1; the project file was last saved in 4.7
- Windows recommended for capture-card drivers
- An OpenXR runtime (SteamVR, Quest Link/Air Link, Windows Mixed Reality) for headset mode

## Open the project

1. Install [Godot 4.3+](https://godotengine.org/download)
2. Import `FrameDirectorVR` in the Project Manager
3. Press **F5** to run

Without a headset, the app starts a desktop fly camera so you can still place screens and edit settings.

## Capture inputs

Each channel in **Settings** is one screen in the room.

| Setting | What it does |
| --- | --- |
| Channel number | Large **CH n** badge on the screen plus a watermark on the picture |
| HDMI / SDI | Marks the expected physical input. Passed to the device format dictionary when the driver supports it |
| Resolution | 720p, 1080p, 1440p, 2160p |
| Framerate | 24 / 25 / 30 / 50 / 60 |
| Capture device | Any camera/capture source Godot sees through `CameraServer`, or a simulated test pattern. Remembered by name, so it reconnects after a re-plug |
| Flip vertically | For drivers that deliver frames upside down |

Many HDMI USB capture cards appear as a camera. SDI decks (Blackmagic DeckLink, AJA, and similar) also show up if the OS exposes them as a video device. Cards that only talk through a vendor SDK need a native plugin; the HDMI/SDI setting is ready for that.

Each channel card shows a status line (device name, or "not found · showing test pattern" when the card is unplugged). Unplugged devices reconnect automatically when they come back.

RGB and YCbCr (4:2:2 packed or planar) frames are converted on the GPU using BT.709 limited range.

Settings are saved to `user://frame_director_settings.json`, including when the window is closed.

## Controls

**Desktop**

- **WASD** move, **Q / E** up / down
- **Right-click** toggle mouse look
- **Left-click** grab the screen under the cursor, **wheel** push/pull
- **Tab** or **F1** settings, **+** add a channel, **Quit app** in settings to exit

**VR**

- **Grip** grab and place a screen
- **Trigger** click the settings panel
- **Menu / A / X / B / Y** toggle settings

## Project layout

- `scenes/main.tscn` — entry scene
- `scripts/` — VR rig, capture feeds, grab pointers, settings UI
- `shaders/simulated_feed.gdshader` — color-bar fallback when no card is selected
