class_name CaptureStage
extends Node3D

signal screens_changed

var screens: Array[CaptureScreen] = []
var feeds: Array[CaptureFeed] = []

func _ready() -> void:
	rebuild()

func rebuild() -> void:
	for child in get_children():
		remove_child(child)
		child.free()
	screens.clear()
	feeds.clear()
	for channel in AppSettings.channels:
		_spawn_channel(channel)
	screens_changed.emit()

func _spawn_channel(channel: CaptureChannelSettings) -> void:
	var feed := CaptureFeed.new()
	add_child(feed)
	feed.configure(channel)
	feeds.append(feed)

	var screen := CaptureScreen.new()
	add_child(screen)
	screen.configure(channel, feed)
	screens.append(screen)

func add_channel() -> void:
	var channel: CaptureChannelSettings = AppSettings.add_channel()
	_spawn_channel(channel)
	screens_changed.emit()

func remove_channel(channel: CaptureChannelSettings) -> void:
	AppSettings.remove_channel(channel)
	rebuild()

func apply_channel_settings(channel: CaptureChannelSettings) -> void:
	for i in screens.size():
		if screens[i].settings == channel:
			feeds[i].configure(channel)
			screens[i].configure(channel, feeds[i])
			break
	AppSettings.save_to_disk()

func persist_transforms() -> void:
	for screen in screens:
		screen.snap_settings_from_transform()
	AppSettings.save_to_disk()

func available_capture_devices() -> Array[Dictionary]:
	var devices: Array[Dictionary] = []
	for i in CameraServer.get_feed_count():
		var feed := CameraServer.get_feed(i)
		if feed == null:
			continue
		devices.append({
			"id": i,
			"name": feed.get_name(),
		})
	return devices
