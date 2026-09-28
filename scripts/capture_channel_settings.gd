class_name CaptureChannelSettings
extends Resource

enum SignalKind { HDMI, SDI }

const RESOLUTIONS: Array[Vector2i] = [
	Vector2i(1280, 720),
	Vector2i(1920, 1080),
	Vector2i(2560, 1440),
	Vector2i(3840, 2160),
]

const FRAMERATES: Array[int] = [24, 25, 30, 50, 60]

@export var channel_number: int = 1
@export var display_name: String = "Input 1"
@export var signal_kind: SignalKind = SignalKind.HDMI
@export var width: int = 1920
@export var height: int = 1080
@export var framerate: int = 60
@export var device_id: int = -1
@export var use_simulated_feed: bool = true
@export var transform: Transform3D = Transform3D.IDENTITY
@export var visible: bool = true

func to_dict() -> Dictionary:
	return {
		"channel_number": channel_number,
		"display_name": display_name,
		"signal_kind": int(signal_kind),
		"width": width,
		"height": height,
		"framerate": framerate,
		"device_id": device_id,
		"use_simulated_feed": use_simulated_feed,
		"transform": [
			transform.origin.x, transform.origin.y, transform.origin.z,
			transform.basis.x.x, transform.basis.x.y, transform.basis.x.z,
			transform.basis.y.x, transform.basis.y.y, transform.basis.y.z,
			transform.basis.z.x, transform.basis.z.y, transform.basis.z.z,
		],
		"visible": visible,
	}

static func from_dict(data: Dictionary) -> CaptureChannelSettings:
	var settings := CaptureChannelSettings.new()
	settings.channel_number = int(data.get("channel_number", 1))
	settings.display_name = String(data.get("display_name", "Input %d" % settings.channel_number))
	settings.signal_kind = int(data.get("signal_kind", SignalKind.HDMI)) as SignalKind
	settings.width = int(data.get("width", 1920))
	settings.height = int(data.get("height", 1080))
	settings.framerate = int(data.get("framerate", 60))
	settings.device_id = int(data.get("device_id", -1))
	settings.use_simulated_feed = bool(data.get("use_simulated_feed", true))
	settings.visible = bool(data.get("visible", true))
	var t: Array = data.get("transform", [])
	if t.size() == 12:
		var origin := Vector3(float(t[0]), float(t[1]), float(t[2]))
		var basis := Basis(
			Vector3(float(t[3]), float(t[4]), float(t[5])),
			Vector3(float(t[6]), float(t[7]), float(t[8])),
			Vector3(float(t[9]), float(t[10]), float(t[11]))
		)
		settings.transform = Transform3D(basis, origin)
	return settings

func signal_label() -> String:
	return "HDMI" if signal_kind == SignalKind.HDMI else "SDI"

func resolution_label() -> String:
	return "%dx%d @ %dHz" % [width, height, framerate]
