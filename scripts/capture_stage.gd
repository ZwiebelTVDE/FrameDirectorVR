class_name CaptureStage
extends Node3D

signal screens_changed
## Emitted when capture devices appear or disappear (card plugged in, driver ready).
signal devices_changed

var screens: Array[CaptureScreen] = []
var feeds: Array[CaptureFeed] = []

func _ready() -> void:
	CameraServer.camera_feeds_updated.connect(_on_camera_feeds_updated)
	CameraServer.camera_feed_added.connect(func(_id: int): _on_camera_feeds_updated())
	CameraServer.camera_feed_removed.connect(func(_id: int): _on_camera_feeds_updated())
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
	persist_transforms()
	var channel: CaptureChannelSettings = AppSettings.add_channel()
	_spawn_channel(channel)
	screens_changed.emit()

func remove_channel(channel: CaptureChannelSettings) -> void:
	persist_transforms()
	AppSettings.remove_channel(channel)
	rebuild()

## reconnect_source: true when the change affects the capture device/format
## (device, resolution, framerate, signal). Cosmetic changes such as the name or
## visibility only refresh the screen, so typing a name doesn't restart a feed.
func apply_channel_settings(channel: CaptureChannelSettings, reconnect_source: bool = true) -> void:
	for i in screens.size():
		if screens[i].settings == channel:
			screens[i].snap_settings_from_transform()
			if reconnect_source:
				feeds[i].configure(channel)
			screens[i].configure(channel, feeds[i])
			break
	AppSettings.save_to_disk()

func persist_transforms() -> void:
	for screen in screens:
		screen.snap_settings_from_transform()
	AppSettings.save_to_disk()

func _on_camera_feeds_updated() -> void:
	# A card that wasn't ready at startup (or was re-plugged) should come back on
	# its own; channels that are already live are left alone.
	for i in feeds.size():
		var channel := feeds[i].settings
		if channel == null or channel.use_simulated_feed or feeds[i].is_live():
			continue
		# Either the device just came back (reconnect), or it just went away
		# (switch to the test pattern and show "not found" until it returns).
		var was_connected: bool = feeds[i]._camera_feed != null
		if resolve_feed(channel) != null or was_connected:
			feeds[i].configure(channel)
			screens[i].configure(channel, feeds[i])
	devices_changed.emit()

## Finds the CameraFeed for a channel, by name first (stable across runs), then id.
static func resolve_feed(channel: CaptureChannelSettings) -> CameraFeed:
	var by_id: CameraFeed = null
	for feed in CameraServer.feeds():
		if feed == null:
			continue
		if channel.device_name != "" and feed.get_name() == channel.device_name:
			return feed
		if feed.get_id() == channel.device_id:
			by_id = feed
	return by_id if channel.device_name == "" else null

func available_capture_devices() -> Array[Dictionary]:
	var devices: Array[Dictionary] = []
	for feed in CameraServer.feeds():
		if feed == null:
			continue
		devices.append({
			"id": feed.get_id(),
			"name": feed.get_name(),
		})
	return devices
