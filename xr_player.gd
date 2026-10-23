class_name XrPlayer
extends XROrigin3D

signal menu_toggled

var xr_interface: XRInterface
var left_controller: XRController3D
var right_controller: XRController3D
var left_pointer: Pointer
var right_pointer: Pointer

func _ready() -> void:
	xr_interface = XRServer.find_interface("OpenXR")
	if xr_interface and xr_interface.is_initialized():
		get_viewport().use_xr = true
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)

	var camera := XRCamera3D.new()
	camera.current = true
	add_child(camera)

	left_controller = XRController3D.new()
	left_controller.tracker = &"left_hand"
	left_controller.pose = &"aim"
	add_child(left_controller)
	_add_controller_mesh(left_controller)

	right_controller = XRController3D.new()
	right_controller.tracker = &"right_hand"
	right_controller.pose = &"aim"
	add_child(right_controller)
	_add_controller_mesh(right_controller)

	left_pointer = Pointer.new()
	left_pointer.is_right_hand = false
	left_controller.add_child(left_pointer)
	left_pointer.setup(left_controller)

	right_pointer = Pointer.new()
	right_controller.add_child(right_pointer)
	right_pointer.setup(right_controller)

	left_controller.button_pressed.connect(_on_button.bind(false))
	right_controller.button_pressed.connect(_on_button.bind(true))

func _add_controller_mesh(controller: XRController3D) -> void:
	var mesh := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(0.04, 0.04, 0.12)
	mesh.mesh = box
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.86, 0.80, 0.72)
	mat.metallic = 0.05
	mat.roughness = 0.55
	mesh.set_surface_override_material(0, mat)
	controller.add_child(mesh)

func _on_button(button_name: String, _right: bool) -> void:
	if button_name == "menu_button" or button_name == "by_button" or button_name == "ax_button":
		menu_toggled.emit()

static func try_start() -> bool:
	var xr := XRServer.find_interface("OpenXR")
	if xr == null:
		return false
	if xr.is_initialized():
		return true
	return xr.initialize()
