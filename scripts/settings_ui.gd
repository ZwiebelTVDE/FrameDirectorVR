class_name SettingsUi
extends Control

signal add_requested
signal remove_requested(channel: CaptureChannelSettings)
signal channel_changed(channel: CaptureChannelSettings)
signal quit_requested

var stage: CaptureStage
var _list: VBoxContainer
var _hint: Label

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	var root := PanelContainer.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.add_theme_stylebox_override("panel", _panel_style())
	add_child(root)

	var margin := MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 28)
	margin.add_theme_constant_override("margin_right", 28)
	margin.add_theme_constant_override("margin_top", 24)
	margin.add_theme_constant_override("margin_bottom", 24)
	root.add_child(margin)

	var layout := VBoxContainer.new()
	layout.add_theme_constant_override("separation", 16)
	margin.add_child(layout)

	var title := Label.new()
	title.text = "Frame Director — Capture Inputs"
	title.add_theme_font_size_override("font_size", 32)
	title.add_theme_color_override("font_color", Color(0.28, 0.24, 0.20))
	layout.add_child(title)

	_hint = Label.new()
	_hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_hint.add_theme_color_override("font_color", Color(0.42, 0.38, 0.34))
	_hint.text = "Each channel is a numbered screen in the 3D stage. Set HDMI or SDI, resolution, and framerate. Hold grip (VR) or left-click (desktop) to place a screen."
	layout.add_child(_hint)

	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	layout.add_child(scroll)
	_list = VBoxContainer.new()
	_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_list.add_theme_constant_override("separation", 12)
	scroll.add_child(_list)

	var actions := HBoxContainer.new()
	actions.add_theme_constant_override("separation", 12)
	layout.add_child(actions)

	var add_button := Button.new()
	add_button.text = "Add capture channel"
	add_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	add_button.pressed.connect(func(): add_requested.emit())
	actions.add_child(add_button)

	var quit_button := Button.new()
	quit_button.text = "Quit app"
	quit_button.pressed.connect(func(): quit_requested.emit())
	actions.add_child(quit_button)

func rebuild() -> void:
	for child in _list.get_children():
		child.queue_free()
	if stage == null:
		return
	var devices := stage.available_capture_devices()
	for channel in AppSettings.channels:
		_list.add_child(_make_card(channel, devices))

func _make_card(channel: CaptureChannelSettings, devices: Array[Dictionary]) -> Control:
	var card := PanelContainer.new()
	card.add_theme_stylebox_override("panel", _card_style())
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 8)
	card.add_child(box)

	var header := HBoxContainer.new()
	box.add_child(header)
	var badge := Label.new()
	badge.text = "CH %d" % channel.channel_number
	badge.add_theme_font_size_override("font_size", 28)
	header.add_child(badge)

	var name_edit := LineEdit.new()
	name_edit.text = channel.display_name
	name_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	name_edit.placeholder_text = "Input name"
	name_edit.text_changed.connect(func(v: String):
		channel.display_name = v
		channel_changed.emit(channel)
	)
	header.add_child(name_edit)

	var visible_toggle := CheckBox.new()
	visible_toggle.text = "Visible"
	visible_toggle.button_pressed = channel.visible
	visible_toggle.toggled.connect(func(on: bool):
		channel.visible = on
		channel_changed.emit(channel)
	)
	header.add_child(visible_toggle)

	var grid := GridContainer.new()
	grid.columns = 2
	box.add_child(grid)

	_add_label(grid, "Signal")
	var signal_box := OptionButton.new()
	signal_box.add_item("HDMI", CaptureChannelSettings.SignalKind.HDMI)
	signal_box.add_item("SDI", CaptureChannelSettings.SignalKind.SDI)
	signal_box.select(channel.signal_kind)
	signal_box.item_selected.connect(func(idx: int):
		channel.signal_kind = signal_box.get_item_id(idx) as CaptureChannelSettings.SignalKind
		channel_changed.emit(channel)
	)
	grid.add_child(signal_box)

	_add_label(grid, "Resolution")
	var res_box := OptionButton.new()
	for i in CaptureChannelSettings.RESOLUTIONS.size():
		var res: Vector2i = CaptureChannelSettings.RESOLUTIONS[i]
		res_box.add_item("%d x %d" % [res.x, res.y], i)
		if res.x == channel.width and res.y == channel.height:
			res_box.select(i)
	res_box.item_selected.connect(func(idx: int):
		var res: Vector2i = CaptureChannelSettings.RESOLUTIONS[idx]
		channel.width = res.x
		channel.height = res.y
		channel_changed.emit(channel)
	)
	grid.add_child(res_box)

	_add_label(grid, "Framerate")
	var fps_box := OptionButton.new()
	for i in CaptureChannelSettings.FRAMERATES.size():
		var fps: int = CaptureChannelSettings.FRAMERATES[i]
		fps_box.add_item("%d fps" % fps, fps)
		if fps == channel.framerate:
			fps_box.select(i)
	fps_box.item_selected.connect(func(idx: int):
		channel.framerate = fps_box.get_item_id(idx)
		channel_changed.emit(channel)
	)
	grid.add_child(fps_box)

	_add_label(grid, "Capture device")
	var device_box := OptionButton.new()
	device_box.add_item("Simulated test pattern", 1000)
	var selected := 0
	for i in devices.size():
		var device: Dictionary = devices[i]
		device_box.add_item("%d — %s" % [device.id, device.name], int(device.id))
		if int(device.id) == channel.device_id and not channel.use_simulated_feed:
			selected = i + 1
	device_box.select(selected)
	device_box.item_selected.connect(func(idx: int):
		var id := device_box.get_item_id(idx)
		if id >= 1000:
			channel.device_id = -1
			channel.use_simulated_feed = true
		else:
			channel.device_id = id
			channel.use_simulated_feed = false
		channel_changed.emit(channel)
	)
	grid.add_child(device_box)

	var footer := HBoxContainer.new()
	box.add_child(footer)
	var channel_spin := SpinBox.new()
	channel_spin.min_value = 1
	channel_spin.max_value = 64
	channel_spin.value = channel.channel_number
	channel_spin.prefix = "Channel "
	channel_spin.value_changed.connect(func(v: float):
		channel.channel_number = int(v)
		badge.text = "CH %d" % channel.channel_number
		channel_changed.emit(channel)
	)
	footer.add_child(channel_spin)
	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	footer.add_child(spacer)
	var remove := Button.new()
	remove.text = "Remove"
	remove.pressed.connect(func(): remove_requested.emit(channel))
	footer.add_child(remove)
	return card

func _add_label(parent: Control, text: String) -> void:
	var label := Label.new()
	label.text = text
	parent.add_child(label)

func _panel_style() -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.96, 0.94, 0.90, 0.96)
	style.corner_radius_top_left = 16
	style.corner_radius_top_right = 16
	style.corner_radius_bottom_left = 16
	style.corner_radius_bottom_right = 16
	return style

func _card_style() -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(1.0, 0.98, 0.95, 0.98)
	style.content_margin_left = 16
	style.content_margin_right = 16
	style.content_margin_top = 12
	style.content_margin_bottom = 12
	style.corner_radius_top_left = 10
	style.corner_radius_top_right = 10
	style.corner_radius_bottom_left = 10
	style.corner_radius_bottom_right = 10
	return style
