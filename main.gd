extends Node3D

var stage: CaptureStage
var settings_ui: SettingsUi
var desktop_layer: CanvasLayer
var vr_panel: WorldUiPanel
var xr_player: XrPlayer
var desktop_player: DesktopPlayer
var settings_visible := true

func _ready() -> void:
	add_child(StudioRoom.new())
	stage = CaptureStage.new()
	add_child(stage)

	settings_ui = SettingsUi.new()
	settings_ui.stage = stage
	settings_ui.add_requested.connect(_on_add)
	settings_ui.remove_requested.connect(_on_remove)
	settings_ui.channel_changed.connect(_on_channel_changed)
	settings_ui.quit_requested.connect(_on_quit)
	stage.screens_changed.connect(_refresh_settings)
	stage.devices_changed.connect(_refresh_settings)
	get_tree().auto_accept_quit = false

	var vr_ok := XrPlayer.try_start()
	if vr_ok:
		xr_player = XrPlayer.new()
		add_child(xr_player)
		xr_player.menu_toggled.connect(toggle_settings)
		_spawn_vr_panel()
	else:
		desktop_player = DesktopPlayer.new()
		add_child(desktop_player)
		desktop_layer = CanvasLayer.new()
		add_child(desktop_layer)
		var host := Control.new()
		host.set_anchors_preset(Control.PRESET_FULL_RECT)
		host.mouse_filter = Control.MOUSE_FILTER_IGNORE
		desktop_layer.add_child(host)
		var panel := MarginContainer.new()
		panel.set_anchors_preset(Control.PRESET_LEFT_WIDE)
		panel.offset_left = 24
		panel.offset_top = 24
		panel.offset_right = 560
		panel.offset_bottom = -24
		panel.mouse_filter = Control.MOUSE_FILTER_STOP
		host.add_child(panel)
		panel.add_child(settings_ui)
		_add_help_label(host)

	_refresh_settings()

func _add_help_label(host: Control) -> void:
	var help := Label.new()
	help.text = "WASD move · Q/E up/down · Right-click look · Left-click drag screens · Wheel distance · Tab settings · + add channel · Quit in settings"
	help.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	help.offset_left = 24
	help.offset_bottom = -16
	help.offset_top = -48
	help.mouse_filter = Control.MOUSE_FILTER_IGNORE
	host.add_child(help)

func _spawn_vr_panel() -> void:
	vr_panel = WorldUiPanel.new()
	add_child(vr_panel)
	vr_panel.setup(settings_ui, Vector2(1.36, 0.85))
	vr_panel.position = Vector3(-0.2, 1.35, -1.15)
	vr_panel.rotate_y(deg_to_rad(12))

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("toggle_settings") or (event is InputEventKey and event.pressed and not event.echo and event.keycode == KEY_TAB):
		toggle_settings()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("add_channel"):
		_on_add()

func toggle_settings() -> void:
	settings_visible = not settings_visible
	if desktop_layer:
		desktop_layer.visible = settings_visible
	if vr_panel:
		vr_panel.visible = settings_visible

func _on_add() -> void:
	stage.add_channel()

func _on_remove(channel: CaptureChannelSettings) -> void:
	stage.remove_channel(channel)

func _on_channel_changed(channel: CaptureChannelSettings, reconnect_source: bool) -> void:
	stage.apply_channel_settings(channel, reconnect_source)
	if reconnect_source:
		# Refresh status lines ("not found", device name...) after the feed restarts.
		_refresh_settings.call_deferred()

func _refresh_settings() -> void:
	settings_ui.rebuild()

func _notification(what: int) -> void:
	# Closing the window should save placement just like the Quit button.
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		_on_quit()

func _on_quit() -> void:
	if stage:
		stage.persist_transforms()
	get_tree().quit()
