class_name StudioRoom
extends Node3D

const PLASTER := Color(0.94, 0.91, 0.86)
const CEILING := Color(0.97, 0.95, 0.91)
const OAK := Color(0.74, 0.61, 0.46)
const OAK_DARK := Color(0.55, 0.42, 0.31)
const LINEN := Color(0.88, 0.84, 0.78)
const LINEN_DEEP := Color(0.78, 0.72, 0.64)
const OLIVE := Color(0.45, 0.52, 0.38)
const SKY := Color(0.86, 0.90, 0.93)

func _ready() -> void:
	_add_lights()
	_add_environment()
	_add_shell()
	_add_window()
	_add_furnishings()

func _add_lights() -> void:
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-38, 18, 0)
	sun.light_energy = 1.05
	sun.light_color = Color(1.0, 0.96, 0.90)
	sun.shadow_enabled = true
	sun.light_indirect_energy = 0.8
	add_child(sun)

	var window_light := OmniLight3D.new()
	window_light.position = Vector3(0.0, 2.1, -3.35)
	window_light.light_energy = 2.4
	window_light.omni_range = 9.0
	window_light.light_color = Color(1.0, 0.95, 0.88)
	window_light.shadow_enabled = true
	add_child(window_light)

	var lamp := OmniLight3D.new()
	lamp.position = Vector3(-1.6, 1.35, 1.1)
	lamp.light_energy = 0.55
	lamp.omni_range = 4.5
	lamp.light_color = Color(1.0, 0.82, 0.62)
	add_child(lamp)

func _add_environment() -> void:
	var env_node := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color(0.90, 0.88, 0.83)
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color(0.92, 0.88, 0.82)
	environment.ambient_light_energy = 0.55
	environment.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	environment.ssao_enabled = true
	environment.ssao_radius = 1.2
	environment.ssao_intensity = 1.4
	environment.glow_enabled = false
	env_node.environment = environment
	add_child(env_node)

func _add_shell() -> void:
	_add_box(Vector3(0, -0.04, 0), Vector3(8.4, 0.08, 8.4), OAK, 0.62, true)
	_add_box(Vector3(0, 3.18, 0), Vector3(8.4, 0.08, 8.4), CEILING, 0.95, false)
	_add_box(Vector3(0, 1.55, -4.16), Vector3(8.4, 3.2, 0.12), PLASTER, 0.94, true)
	_add_box(Vector3(-4.16, 1.55, 0), Vector3(0.12, 3.2, 8.4), PLASTER, 0.94, true)
	_add_box(Vector3(4.16, 1.55, 0), Vector3(0.12, 3.2, 8.4), PLASTER, 0.94, true)
	_add_box(Vector3(0, 1.55, 4.16), Vector3(8.4, 3.2, 0.12), PLASTER, 0.94, true)

func _add_window() -> void:
	_add_box(Vector3(0, 1.85, -4.07), Vector3(2.6, 1.9, 0.04), Color(0.82, 0.78, 0.72), 0.7, false)
	var glass := MeshInstance3D.new()
	var pane := QuadMesh.new()
	pane.size = Vector2(2.28, 1.62)
	glass.mesh = pane
	glass.position = Vector3(0, 1.85, -4.04)
	glass.set_surface_override_material(0, _unshaded(SKY))
	add_child(glass)
	_add_box(Vector3(-0.78, 1.85, -4.05), Vector3(0.04, 1.62, 0.05), LINEN, 0.85, false)
	_add_box(Vector3(0.78, 1.85, -4.05), Vector3(0.04, 1.62, 0.05), LINEN, 0.85, false)
	_add_box(Vector3(-1.55, 1.85, -3.95), Vector3(0.08, 2.4, 0.18), LINEN_DEEP, 0.9, false)
	_add_box(Vector3(1.55, 1.85, -3.95), Vector3(0.08, 2.4, 0.18), LINEN_DEEP, 0.9, false)

	_add_box(Vector3(0, 0.92, -3.92), Vector3(2.7, 0.08, 0.28), OAK_DARK, 0.5, false)

func _add_furnishings() -> void:
	# Soft rug
	var rug := MeshInstance3D.new()
	var rug_mesh := CylinderMesh.new()
	rug_mesh.top_radius = 1.55
	rug_mesh.bottom_radius = 1.55
	rug_mesh.height = 0.02
	rug_mesh.radial_segments = 32
	rug.mesh = rug_mesh
	rug.position = Vector3(0.15, 0.02, 0.35)
	rug.set_surface_override_material(0, _mat(Color(0.82, 0.70, 0.58), 0.95))
	add_child(rug)

	# Low oak bench / daybed
	_add_box(Vector3(-2.15, 0.22, 1.55), Vector3(1.9, 0.28, 0.72), OAK, 0.58, false)
	_add_box(Vector3(-2.15, 0.42, 1.55), Vector3(1.86, 0.12, 0.68), LINEN, 0.92, false)
	_add_box(Vector3(-2.15, 0.62, 1.82), Vector3(1.86, 0.42, 0.16), LINEN_DEEP, 0.92, false)
	_add_box(Vector3(-1.28, 0.52, 1.55), Vector3(0.18, 0.18, 0.42), LINEN, 0.92, false)

	# Coffee table
	_add_box(Vector3(0.2, 0.18, 0.45), Vector3(0.9, 0.04, 0.55), OAK_DARK, 0.45, false)
	_add_box(Vector3(-0.18, 0.09, 0.22), Vector3(0.06, 0.16, 0.06), OAK, 0.5, false)
	_add_box(Vector3(0.58, 0.09, 0.22), Vector3(0.06, 0.16, 0.06), OAK, 0.5, false)
	_add_box(Vector3(-0.18, 0.09, 0.68), Vector3(0.06, 0.16, 0.06), OAK, 0.5, false)
	_add_box(Vector3(0.58, 0.09, 0.68), Vector3(0.06, 0.16, 0.06), OAK, 0.5, false)

	# Ceramic lamp
	_add_box(Vector3(-1.55, 0.52, 1.15), Vector3(0.12, 0.28, 0.12), Color(0.91, 0.89, 0.84), 0.35, false)
	var shade := MeshInstance3D.new()
	var shade_mesh := CylinderMesh.new()
	shade_mesh.top_radius = 0.11
	shade_mesh.bottom_radius = 0.16
	shade_mesh.height = 0.16
	shade.mesh = shade_mesh
	shade.position = Vector3(-1.55, 0.74, 1.15)
	shade.set_surface_override_material(0, _mat(Color(0.96, 0.90, 0.78), 0.85))
	add_child(shade)

	# Plant
	_add_box(Vector3(2.45, 0.16, -2.55), Vector3(0.28, 0.22, 0.28), Color(0.72, 0.62, 0.52), 0.7, false)
	var trunk := MeshInstance3D.new()
	var trunk_mesh := CylinderMesh.new()
	trunk_mesh.top_radius = 0.025
	trunk_mesh.bottom_radius = 0.035
	trunk_mesh.height = 0.7
	trunk.mesh = trunk_mesh
	trunk.position = Vector3(2.45, 0.55, -2.55)
	trunk.set_surface_override_material(0, _mat(OAK_DARK, 0.7))
	add_child(trunk)
	_add_foliage(Vector3(2.45, 1.05, -2.55), 0.28)
	_add_foliage(Vector3(2.58, 0.92, -2.42), 0.18)
	_add_foliage(Vector3(2.32, 0.88, -2.64), 0.16)

	# Slim shelf
	_add_box(Vector3(3.92, 1.35, -0.4), Vector3(0.16, 0.04, 1.6), OAK, 0.5, false)
	_add_box(Vector3(3.92, 1.42, -0.85), Vector3(0.12, 0.18, 0.14), LINEN, 0.9, false)
	_add_box(Vector3(3.92, 1.46, -0.15), Vector3(0.1, 0.26, 0.1), Color(0.62, 0.48, 0.42), 0.4, false)

	var word := Label3D.new()
	word.text = "frame director"
	word.font_size = 42
	word.modulate = Color(0.45, 0.40, 0.35, 0.55)
	word.position = Vector3(0, 2.85, -4.08)
	word.pixel_size = 0.004
	add_child(word)

func _add_foliage(origin: Vector3, radius: float) -> void:
	var leaf := MeshInstance3D.new()
	var sphere := SphereMesh.new()
	sphere.radius = radius
	sphere.height = radius * 1.7
	leaf.mesh = sphere
	leaf.position = origin
	leaf.set_surface_override_material(0, _mat(OLIVE, 0.88))
	add_child(leaf)

func _add_box(origin: Vector3, size: Vector3, color: Color, roughness: float, collider: bool, transparent: bool = false) -> MeshInstance3D:
	var mesh := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = size
	mesh.mesh = box
	mesh.position = origin
	var mat := _mat(color, roughness)
	if transparent:
		mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mesh.set_surface_override_material(0, mat)
	add_child(mesh)
	if collider:
		var body := StaticBody3D.new()
		body.position = origin
		var shape := CollisionShape3D.new()
		var box_shape := BoxShape3D.new()
		box_shape.size = size
		shape.shape = box_shape
		body.add_child(shape)
		add_child(body)
	return mesh

func _mat(color: Color, roughness: float) -> StandardMaterial3D:
	var mat := StandardMaterial3D.new()
	mat.albedo_color = color
	mat.roughness = roughness
	mat.metallic = 0.0
	return mat

func _unshaded(color: Color) -> StandardMaterial3D:
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.albedo_color = color
	return mat
