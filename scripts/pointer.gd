class_name Pointer
extends Node3D

signal ui_clicked(event: InputEvent)

const SCREEN_MASK := 2
const UI_MASK := 4

@export var is_right_hand := true

var grabbed_screen: CaptureScreen
var _grab_offset: Transform3D
var _trigger_was_pressed := false
var _trigger_clicked := false
var _ray: RayCast3D
var _beam: MeshInstance3D
var _dot: MeshInstance3D
var _controller: XRController3D

func setup(controller: XRController3D) -> void:
	_controller = controller
	_ray = RayCast3D.new()
	_ray.target_position = Vector3(0, 0, -4.0)
	_ray.collide_with_areas = true
	_ray.collide_with_bodies = true
	_ray.collision_mask = SCREEN_MASK | UI_MASK
	add_child(_ray)

	_beam = MeshInstance3D.new()
	var cylinder := CylinderMesh.new()
	cylinder.top_radius = 0.002
	cylinder.bottom_radius = 0.002
	cylinder.height = 1.0
	_beam.mesh = cylinder
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.albedo_color = Color(0.92, 0.82, 0.68, 0.7)
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_beam.set_surface_override_material(0, mat)
	_beam.rotation_degrees.x = 90
	_beam.position = Vector3(0, 0, -0.5)
	add_child(_beam)

	_dot = MeshInstance3D.new()
	var sphere := SphereMesh.new()
	sphere.radius = 0.012
	sphere.height = 0.024
	_dot.mesh = sphere
	_dot.set_surface_override_material(0, mat)
	add_child(_dot)

func _physics_process(_delta: float) -> void:
	if _controller == null:
		return
	if grabbed_screen:
		grabbed_screen.global_transform = _controller.global_transform * _grab_offset
		_update_beam(0.15, true)
		if not _grip_pressed():
			_release()
		return

	_update_trigger_edge()
	if not _ray.is_colliding():
		_update_beam(1.2, false)
		return

	var hit := _ray.get_collision_point()
	var dist := global_position.distance_to(hit)
	_update_beam(dist, true)
	_dot.global_position = hit

	var collider := _ray.get_collider()
	var trigger_click := _trigger_clicked
	if _grip_pressed() and collider is CaptureScreen:
		_grab(collider as CaptureScreen)
	elif collider is WorldUiPanel:
		(collider as WorldUiPanel).handle_pointer(hit, trigger_click)

func _grip_pressed() -> bool:
	return _controller.is_button_pressed("grip_click") or _controller.get_float("grip") > 0.75

func _update_trigger_edge() -> void:
	var trigger_now := _controller.is_button_pressed("trigger") or _controller.get_float("trigger") > 0.7
	_trigger_clicked = trigger_now and not _trigger_was_pressed
	_trigger_was_pressed = trigger_now

func _grab(screen: CaptureScreen) -> void:
	grabbed_screen = screen
	_grab_offset = _controller.global_transform.affine_inverse() * screen.global_transform
	screen.grabbed.emit(screen)

func _release() -> void:
	if grabbed_screen:
		grabbed_screen.released.emit(grabbed_screen)
		var stage := grabbed_screen.get_parent()
		if stage is CaptureStage:
			(stage as CaptureStage).persist_transforms()
	grabbed_screen = null

func _update_beam(length: float, visible_dot: bool) -> void:
	length = max(length, 0.05)
	(_beam.mesh as CylinderMesh).height = length
	_beam.position = Vector3(0, 0, -length * 0.5)
	_dot.visible = visible_dot
