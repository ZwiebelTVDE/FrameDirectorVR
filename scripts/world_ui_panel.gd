class_name WorldUiPanel
extends StaticBody3D

var viewport: SubViewport
var _quad_size := Vector2(1.2, 0.75)
var _mesh: MeshInstance3D
var _last_hover := Vector2(-1, -1)

func setup(ui: Control, size: Vector2 = Vector2(1.2, 0.75)) -> void:
	_quad_size = size
	collision_layer = 4
	collision_mask = 0

	viewport = SubViewport.new()
	viewport.size = Vector2i(1280, 800)
	viewport.transparent_bg = true
	viewport.handle_input_locally = true
	viewport.gui_embed_subwindows = true
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	viewport.add_child(ui)
	ui.set_anchors_preset(Control.PRESET_FULL_RECT)
	ui.custom_minimum_size = Vector2(viewport.size)
	add_child(viewport)

	_mesh = MeshInstance3D.new()
	var quad := QuadMesh.new()
	quad.size = _quad_size
	_mesh.mesh = quad
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.albedo_texture = viewport.get_texture()
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_mesh.set_surface_override_material(0, mat)
	add_child(_mesh)

	var collision := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(_quad_size.x, _quad_size.y, 0.02)
	collision.shape = box
	add_child(collision)

func handle_pointer(world_point: Vector3, click: bool) -> void:
	var local := to_local(world_point)
	var uv := Vector2(
		(local.x / _quad_size.x) + 0.5,
		0.5 - (local.y / _quad_size.y)
	)
	var pixel := Vector2(uv.x * viewport.size.x, uv.y * viewport.size.y)
	var motion := InputEventMouseMotion.new()
	motion.position = pixel
	motion.relative = pixel - _last_hover if _last_hover.x >= 0 else Vector2.ZERO
	viewport.push_input(motion)
	_last_hover = pixel
	if click:
		var press := InputEventMouseButton.new()
		press.button_index = MOUSE_BUTTON_LEFT
		press.pressed = true
		press.position = pixel
		viewport.push_input(press)
		var release := InputEventMouseButton.new()
		release.button_index = MOUSE_BUTTON_LEFT
		release.pressed = false
		release.position = pixel
		viewport.push_input(release)
