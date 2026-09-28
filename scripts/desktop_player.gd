class_name DesktopPlayer
extends CharacterBody3D

const SPEED := 3.4
const LOOK_SENS := 0.0025

var _camera: Camera3D
var _grabbed: CaptureScreen
var _grab_distance := 2.2
var capturing_mouse := false

func _ready() -> void:
	_camera = Camera3D.new()
	_camera.current = true
	_camera.fov = 75.0
	add_child(_camera)
	position = Vector3(0, 1.6, 1.6)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_RIGHT:
		capturing_mouse = not capturing_mouse
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED if capturing_mouse else Input.MOUSE_MODE_VISIBLE
	if capturing_mouse and event is InputEventMouseMotion:
		rotate_y(-event.relative.x * LOOK_SENS)
		_camera.rotate_x(-event.relative.y * LOOK_SENS)
		_camera.rotation.x = clampf(_camera.rotation.x, deg_to_rad(-80), deg_to_rad(80))
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		if event.pressed:
			_try_grab()
		else:
			_release()
	if event is InputEventMouseButton and _grabbed:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_grab_distance = min(6.0, _grab_distance + 0.15)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_grab_distance = max(0.6, _grab_distance - 0.15)

func _physics_process(_delta: float) -> void:
	var input := _move_vector()
	var direction := (transform.basis * Vector3(input.x, 0, input.y)).normalized()
	var vertical := 0.0
	if Input.is_key_pressed(KEY_E):
		vertical += 1.0
	if Input.is_key_pressed(KEY_Q):
		vertical -= 1.0
	velocity = Vector3(direction.x, vertical, direction.z) * SPEED
	move_and_slide()
	if _grabbed:
		var from := _camera.global_position
		var to := from + -_camera.global_transform.basis.z * _grab_distance
		_grabbed.global_position = to
		_grabbed.look_at(from, Vector3.UP)
		_grabbed.rotate_object_local(Vector3.UP, PI)

func _try_grab() -> void:
	var space := get_world_3d().direct_space_state
	var from := _camera.global_position
	var to := from + -_camera.global_transform.basis.z * 8.0
	var query := PhysicsRayQueryParameters3D.create(from, to, CaptureScreen.COLLISION_LAYER)
	var hit := space.intersect_ray(query)
	if hit.is_empty():
		return
	if hit.collider is CaptureScreen:
		_grabbed = hit.collider
		_grab_distance = from.distance_to(_grabbed.global_position)

func _release() -> void:
	if _grabbed:
		var stage := _grabbed.get_parent()
		if stage is CaptureStage:
			(stage as CaptureStage).persist_transforms()
	_grabbed = null

func _move_vector() -> Vector2:
	var input := Vector2.ZERO
	if Input.is_physical_key_pressed(KEY_A) or Input.is_physical_key_pressed(KEY_LEFT):
		input.x -= 1.0
	if Input.is_physical_key_pressed(KEY_D) or Input.is_physical_key_pressed(KEY_RIGHT):
		input.x += 1.0
	if Input.is_physical_key_pressed(KEY_W) or Input.is_physical_key_pressed(KEY_UP):
		input.y -= 1.0
	if Input.is_physical_key_pressed(KEY_S) or Input.is_physical_key_pressed(KEY_DOWN):
		input.y += 1.0
	return input.normalized()
