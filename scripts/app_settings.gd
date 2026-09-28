extends Node

const SETTINGS_PATH := "user://frame_director_settings.json"

var channels: Array[CaptureChannelSettings] = []

func _ready() -> void:
	if not load_from_disk():
		_seed_default_channels()
		save_to_disk()

func _seed_default_channels() -> void:
	channels.clear()
	channels.append(_make_channel(1, Vector3(-1.4, 1.5, -2.4), SignalKindHDMI()))
	channels.append(_make_channel(2, Vector3(1.4, 1.5, -2.4), SignalKindSDI()))

func SignalKindHDMI() -> CaptureChannelSettings.SignalKind:
	return CaptureChannelSettings.SignalKind.HDMI

func SignalKindSDI() -> CaptureChannelSettings.SignalKind:
	return CaptureChannelSettings.SignalKind.SDI

func _make_channel(number: int, origin: Vector3, kind: CaptureChannelSettings.SignalKind) -> CaptureChannelSettings:
	var channel := CaptureChannelSettings.new()
	channel.channel_number = number
	channel.display_name = "Input %d" % number
	channel.signal_kind = kind
	channel.transform = Transform3D(Basis.IDENTITY, origin)
	channel.use_simulated_feed = true
	return channel

func add_channel() -> CaptureChannelSettings:
	var used: Dictionary = {}
	for existing in channels:
		used[existing.channel_number] = true
	var next_number := 1
	while used.has(next_number):
		next_number += 1
	var offset := Vector3((next_number - 1) * 0.35, 0.0, 0.0)
	var channel := _make_channel(next_number, Vector3(0.0, 1.5, -2.2) + offset, CaptureChannelSettings.SignalKind.HDMI)
	channels.append(channel)
	save_to_disk()
	return channel

func remove_channel(channel: CaptureChannelSettings) -> void:
	channels.erase(channel)
	save_to_disk()

func find_by_number(channel_number: int) -> CaptureChannelSettings:
	for channel in channels:
		if channel.channel_number == channel_number:
			return channel
	return null

func save_to_disk() -> void:
	var payload := {"channels": []}
	for channel in channels:
		payload["channels"].append(channel.to_dict())
	var file := FileAccess.open(SETTINGS_PATH, FileAccess.WRITE)
	if file == null:
		push_warning("Could not save Frame Director settings.")
		return
	file.store_string(JSON.stringify(payload, "\t"))

func load_from_disk() -> bool:
	if not FileAccess.file_exists(SETTINGS_PATH):
		return false
	var file := FileAccess.open(SETTINGS_PATH, FileAccess.READ)
	if file == null:
		return false
	var parsed: Variant = JSON.parse_string(file.get_as_text())
	if typeof(parsed) != TYPE_DICTIONARY:
		return false
	channels.clear()
	for item in parsed.get("channels", []):
		if typeof(item) == TYPE_DICTIONARY:
			channels.append(CaptureChannelSettings.from_dict(item))
	return not channels.is_empty()
