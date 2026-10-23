class_name CaptureFeed
extends Node

## Produces one Texture2D per channel, either from a real CameraServer feed
## (HDMI/SDI capture cards that the OS exposes as a video device) or from the
## simulated test-pattern shader. The output is always a SubViewport texture so
## screens never need to care which kind of source or pixel format is behind it.

signal texture_changed(texture: Texture2D)
signal status_changed(text: String)

const SIMULATED_MAX := Vector2i(1280, 720)

var settings: CaptureChannelSettings
var output_texture: Texture2D
var status_text: String = "Idle"

var _camera_feed: CameraFeed
var _viewport: SubViewport
var _rect: ColorRect
var _sim_material: ShaderMaterial
var _cam_material: ShaderMaterial
var _tex_rgba: CameraTexture
var _tex_y: CameraTexture
var _tex_cbcr: CameraTexture
var _time := 0.0

func configure(channel: CaptureChannelSettings) -> void:
	settings = channel
	_ensure_viewport()
	_teardown_hardware()
	if settings.use_simulated_feed:
		_setup_simulated("Simulated %s" % settings.signal_label())
	else:
		_setup_hardware()

func is_live() -> bool:
	return _camera_feed != null

func _process(delta: float) -> void:
	if settings == null or _rect == null:
		return
	if _rect.material == _sim_material:
		_time += delta
		_sim_material.set_shader_parameter("time", _time)
		_sim_material.set_shader_parameter("channel_number", settings.channel_number)
		_sim_material.set_shader_parameter("is_sdi", 1 if settings.signal_kind == CaptureChannelSettings.SignalKind.SDI else 0)
		_sim_material.set_shader_parameter("framerate", settings.framerate)
		_sim_material.set_shader_parameter("resolution", Vector2(settings.width, settings.height))
	elif _camera_feed != null:
		# The datatype is only known once the driver delivers frames, and it can
		# change if the source switches format, so keep the shader mode in sync.
		_cam_material.set_shader_parameter("mode", _camera_feed.get_datatype())

func _ensure_viewport() -> void:
	if _viewport != null:
		return
	_viewport = SubViewport.new()
	_viewport.disable_3d = true
	_viewport.transparent_bg = false
	_viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	_rect = ColorRect.new()
	_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	_viewport.add_child(_rect)
	add_child(_viewport)

	_sim_material = ShaderMaterial.new()
	_sim_material.shader = load("res://shaders/simulated_feed.gdshader")
	_cam_material = ShaderMaterial.new()
	_cam_material.shader = load("res://shaders/camera_feed.gdshader")

	output_texture = _viewport.get_texture()

func _set_status(text: String) -> void:
	status_text = text
	status_changed.emit(text)

func _setup_simulated(status: String) -> void:
	# The pattern is resolution independent, so don't burn a 4K render target per
	# screen just to draw colour bars (that matters a lot in VR).
	var size := Vector2i(settings.width, settings.height)
	if size.x > SIMULATED_MAX.x:
		size = Vector2i(SIMULATED_MAX.x, int(SIMULATED_MAX.x * float(size.y) / size.x))
	_viewport.size = size
	_rect.material = _sim_material
	_set_status(status)
	texture_changed.emit(output_texture)

func _setup_hardware() -> void:
	var feed := CaptureStage.resolve_feed(settings)
	if feed == null:
		_setup_simulated("%s not found · showing test pattern" % _device_label())
		return
	_camera_feed = feed
	settings.device_id = feed.get_id()
	settings.device_name = feed.get_name()
	_apply_best_format()
	_camera_feed.set_active(true)

	_tex_rgba = CameraTexture.new()
	_tex_rgba.camera_feed_id = feed.get_id()
	_tex_rgba.which_feed = CameraServer.FEED_RGBA_IMAGE
	_tex_y = CameraTexture.new()
	_tex_y.camera_feed_id = feed.get_id()
	_tex_y.which_feed = CameraServer.FEED_Y_IMAGE
	_tex_cbcr = CameraTexture.new()
	_tex_cbcr.camera_feed_id = feed.get_id()
	_tex_cbcr.which_feed = CameraServer.FEED_CBCR_IMAGE
	_cam_material.set_shader_parameter("rgba_tex", _tex_rgba)
	_cam_material.set_shader_parameter("y_tex", _tex_y)
	_cam_material.set_shader_parameter("cbcr_tex", _tex_cbcr)
	_cam_material.set_shader_parameter("flip_v", settings.flip_vertical)
	_cam_material.set_shader_parameter("mode", feed.get_datatype())

	_viewport.size = Vector2i(settings.width, settings.height)
	_rect.material = _cam_material
	_set_status("%s · %s" % [feed.get_name(), settings.signal_label()])
	texture_changed.emit(output_texture)

func _device_label() -> String:
	if settings.device_name != "":
		return "\"%s\"" % settings.device_name
	return "Capture device %d" % settings.device_id

## Picks the driver format that matches the requested resolution, preferring the
## closest framerate. Formats are dictionaries from CameraFeed.get_formats().
func _apply_best_format() -> void:
	var formats: Array = _camera_feed.get_formats()
	var best := -1
	var best_score := INF
	for i in formats.size():
		var format: Dictionary = formats[i]
		var w := int(format.get("width", 0))
		var h := int(format.get("height", 0))
		var num := float(format.get("frame_numerator", 0))
		var den := float(format.get("frame_denominator", 0))
		# Some backends describe the interval (den/num), others the rate.
		var fps := 0.0
		if num > 0.0 and den > 0.0:
			fps = num / den
			if fps < 1.0:
				fps = den / num
		var score := absf(w - settings.width) * 10.0 + absf(h - settings.height) * 10.0
		score += absf(fps - settings.framerate) if fps > 0.0 else 30.0
		if score < best_score:
			best_score = score
			best = i
	if best < 0:
		return
	if not _camera_feed.set_format(best, {}):
		push_warning("Capture feed %s rejected format %d" % [_camera_feed.get_name(), best])
		return
	var chosen: Dictionary = formats[best]
	if int(chosen.get("width", 0)) != settings.width or int(chosen.get("height", 0)) != settings.height:
		push_warning("%s has no %dx%d mode, using %sx%s" % [
			_camera_feed.get_name(), settings.width, settings.height,
			chosen.get("width", "?"), chosen.get("height", "?")])

func _teardown_hardware() -> void:
	if _camera_feed != null and is_instance_valid(_camera_feed):
		_camera_feed.set_active(false)
	_camera_feed = null
	_tex_rgba = null
	_tex_y = null
	_tex_cbcr = null

func _exit_tree() -> void:
	_teardown_hardware()
