class_name CaptureScreen
extends StaticBody3D

signal grabbed(screen: CaptureScreen)
signal released(screen: CaptureScreen)

const COLLISION_LAYER := 2

var settings: CaptureChannelSettings
var _mesh: MeshInstance3D
var _material: StandardMaterial3D
var _badge: Label3D
var _watermark: Label3D
var _meta: Label3D
var _frame: MeshInstance3D
var _collision: CollisionShape3D
var _feed: CaptureFeed
var _aspect := 16.0 / 9.0

func _ready() -> void:
	collision_layer = COLLISION_LAYER
	collision_mask = 0
	_build()

func configure(channel: CaptureChannelSettings, feed: CaptureFeed) -> void:
	settings = channel
	_feed = feed
	if not is_inside_tree():
		await ready
	global_transform = channel.transform
	_apply_geometry()
	_refresh_labels()
	if feed.output_texture:
		_set_texture(feed.output_texture)
	if not feed.texture_changed.is_connected(_set_texture):
		feed.texture_changed.connect(_set_texture)

func _build() -> void:
	_collision = CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(1.6, 0.9, 0.04)
	_collision.shape = box
	add_child(_collision)

	_frame = MeshInstance3D.new()
	var frame_mesh := BoxMesh.new()
	frame_mesh.size = Vector3(1.66, 0.96, 0.03)
	_frame.mesh = frame_mesh
	var frame_mat := StandardMaterial3D.new()
	frame_mat.albedo_color = Color(0.78, 0.68, 0.54)
	frame_mat.metallic = 0.0
	frame_mat.roughness = 0.62
	_frame.set_surface_override_material(0, frame_mat)
	add_child(_frame)

	_mesh = MeshInstance3D.new()
	var quad := QuadMesh.new()
	quad.size = Vector2(1.6, 0.9)
	_mesh.mesh = quad
	_mesh.position = Vector3(0, 0, 0.02)
	_material = StandardMaterial3D.new()
	_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_material.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR
	_mesh.set_surface_override_material(0, _material)
	add_child(_mesh)

	_badge = Label3D.new()
	_badge.pixel_size = 0.0045
	_badge.font_size = 96
	_badge.outline_size = 18
	_badge.modulate = Color(0.28, 0.24, 0.20)
	_badge.outline_modulate = Color(0.96, 0.93, 0.88, 0.9)
	_badge.billboard = BaseMaterial3D.BILLBOARD_DISABLED
	_badge.position = Vector3(-0.72, 0.58, 0.04)
	_badge.horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT
	add_child(_badge)

	_watermark = Label3D.new()
	_watermark.pixel_size = 0.008
	_watermark.font_size = 180
	_watermark.outline_size = 12
	_watermark.modulate = Color(0.30, 0.26, 0.22, 0.22)
	_watermark.position = Vector3(0, 0, 0.03)
	add_child(_watermark)

	_meta = Label3D.new()
	_meta.pixel_size = 0.0028
	_meta.font_size = 42
	_meta.outline_size = 10
	_meta.position = Vector3(0.0, -0.58, 0.04)
	_meta.modulate = Color(0.42, 0.38, 0.34)
	add_child(_meta)

func _apply_geometry() -> void:
	if settings == null:
		return
	_aspect = float(settings.width) / max(1.0, float(settings.height))
	var height := 0.9
	var width := height * _aspect
	(_mesh.mesh as QuadMesh).size = Vector2(width, height)
	(_frame.mesh as BoxMesh).size = Vector3(width + 0.06, height + 0.06, 0.03)
	(_collision.shape as BoxShape3D).size = Vector3(width, height, 0.04)
	_badge.position = Vector3(-width * 0.5 + 0.08, height * 0.5 + 0.13, 0.04)
	_meta.position = Vector3(0.0, -height * 0.5 - 0.12, 0.04)

func _refresh_labels() -> void:
	if settings == null:
		return
	_badge.text = "CH %d" % settings.channel_number
	_watermark.text = str(settings.channel_number)
	var kind := settings.signal_label()
	_meta.text = "%s  ·  %s  ·  %s" % [settings.display_name, kind, settings.resolution_label()]
	visible = settings.visible

func _set_texture(texture: Texture2D) -> void:
	_material.albedo_texture = texture
	_refresh_labels()

func snap_settings_from_transform() -> void:
	if settings:
		settings.transform = global_transform
