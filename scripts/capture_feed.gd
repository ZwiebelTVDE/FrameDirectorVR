class_name CaptureFeed
extends Node

signal texture_changed(texture: Texture2D)

var settings: CaptureChannelSettings
var output_texture: Texture2D
var status_text: String = "Idle"

var _camera_feed: CameraFeed
var _camera_texture: CameraTexture
var _viewport: SubViewport
var _shader_rect: ColorRect
var _material: ShaderMaterial
var _time := 0.0

func configure(channel: CaptureChannelSettings) -> void:
	settings = channel
	_teardown_hardware()
	if settings.use_simulated_feed or settings.device_id < 0:
		_setup_simulated()
	else:
		_setup_hardware()

func _process(delta: float) -> void:
	if settings == null or _material == null:
		return
	if settings.use_simulated_feed or _camera_feed == null:
		_time += delta
		_material.set_shader_parameter("time", _time)
		_material.set_shader_parameter("channel_number", settings.channel_number)
		_material.set_shader_parameter("is_sdi", 1 if settings.signal_kind == CaptureChannelSettings.SignalKind.SDI else 0)
		_material.set_shader_parameter("framerate", settings.framerate)
		_material.set_shader_parameter("resolution", Vector2(settings.width, settings.height))

func _setup_simulated() -> void:
	status_text = "Simulated %s · waiting for lock" % settings.signal_label()
	if _viewport == null:
		_viewport = SubViewport.new()
		_viewport.disable_3d = true
		_viewport.transparent_bg = false
		_viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
		_shader_rect = ColorRect.new()
		_shader_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
		_material = ShaderMaterial.new()
		_material.shader = load("res://shaders/simulated_feed.gdshader")
		_shader_rect.material = _material
		_viewport.add_child(_shader_rect)
		add_child(_viewport)
	_viewport.size = Vector2i(settings.width, settings.height)
	_shader_rect.size = Vector2(settings.width, settings.height)
	output_texture = _viewport.get_texture()
	texture_changed.emit(output_texture)
	status_text = "Simulated %s" % settings.signal_label()

func _setup_hardware() -> void:
	if CameraServer.get_feed_count() <= settings.device_id:
		status_text = "Capture device %d missing — simulated" % settings.device_id
		_setup_simulated()
		return
	_camera_feed = CameraServer.get_feed(settings.device_id)
	if _camera_feed == null:
		status_text = "Feed unavailable — simulated"
		_setup_simulated()
		return
	_try_apply_format()
	_camera_feed.set_active(true)
	_camera_texture = CameraTexture.new()
	_camera_texture.camera_feed_id = _camera_feed.get_id()
	output_texture = _camera_texture
	status_text = "%s · %s" % [_camera_feed.get_name(), settings.signal_label()]
	texture_changed.emit(output_texture)

func _try_apply_format() -> void:
	if _camera_feed == null:
		return
	var count := _camera_feed.get_formats()
	var best := -1
	for i in count:
		var format: Dictionary = _camera_feed.get_format(i)
		var w := int(format.get("width", 0))
		var h := int(format.get("height", 0))
		if w == settings.width and h == settings.height:
			best = i
			break
	if best >= 0:
		_camera_feed.set_format(best, {
			"frame_numerator": settings.framerate,
			"frame_denominator": 1,
			"signal": settings.signal_label().to_lower(),
		})

func _teardown_hardware() -> void:
	if _camera_feed != null:
		_camera_feed.set_active(false)
	_camera_feed = null
	_camera_texture = null

func _exit_tree() -> void:
	_teardown_hardware()
