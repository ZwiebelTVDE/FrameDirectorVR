// Frame Director VR capture plugin: adds capture devices that Godot can't see
// on its own to the CameraServer.
//   - Media Foundation (Windows): webcams, Elgato and other standard capture cards
//   - DELTACAST (VideoMaster SDK): SDI / HDMI / DisplayPort cards
// Each backend registers an engine singleton; the app calls register_devices().

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#ifdef CAPTURE_WITH_DELTACAST
#include "deltacast/deltacast_devices.h"
#include "deltacast/deltacast_feed.h"
#endif
#ifdef CAPTURE_WITH_MEDIA_FOUNDATION
#include "mediafoundation/mf_devices.h"
#include "mediafoundation/mf_feed.h"
#endif

using namespace godot;
using namespace FrameDirector;

#ifdef CAPTURE_WITH_DELTACAST
static DeltacastDevices *deltacast_devices = nullptr;
#endif
#ifdef CAPTURE_WITH_MEDIA_FOUNDATION
static MediaFoundationDevices *mf_devices = nullptr;
#endif

static void initialize_capture(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
#ifdef CAPTURE_WITH_DELTACAST
	GDREGISTER_CLASS(DeltacastFeed);
	GDREGISTER_CLASS(DeltacastDevices);
	deltacast_devices = memnew(DeltacastDevices);
	Engine::get_singleton()->register_singleton("Deltacast", deltacast_devices);
#endif
#ifdef CAPTURE_WITH_MEDIA_FOUNDATION
	GDREGISTER_CLASS(MediaFoundationFeed);
	GDREGISTER_CLASS(MediaFoundationDevices);
	mf_devices = memnew(MediaFoundationDevices);
	Engine::get_singleton()->register_singleton("MediaFoundation", mf_devices);
#endif
}

static void uninitialize_capture(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
#ifdef CAPTURE_WITH_MEDIA_FOUNDATION
	if (mf_devices) {
		Engine::get_singleton()->unregister_singleton("MediaFoundation");
		memdelete(mf_devices);
		mf_devices = nullptr;
	}
#endif
#ifdef CAPTURE_WITH_DELTACAST
	if (deltacast_devices) {
		Engine::get_singleton()->unregister_singleton("Deltacast");
		memdelete(deltacast_devices);
		deltacast_devices = nullptr;
	}
#endif
}

extern "C" {
GDExtensionBool GDE_EXPORT capture_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_capture);
	init_obj.register_terminator(uninitialize_capture);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
	return init_obj.init();
}
}
