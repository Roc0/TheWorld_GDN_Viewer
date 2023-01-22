#include <Godot.hpp>

#include "GDN_Template.h"
#include "GDN_TheWorld_MainNode.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Camera.h"
#include "GDN_TheWorld_Quadrant.h"
#include "Chunk.h"

extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options * o) 
{
	godot::Godot::gdnative_init(o);
}

extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options * o)
{
	godot::Godot::gdnative_terminate(o);
}

extern "C" void GDN_EXPORT godot_nativescript_init(void* handle)
{
	godot::Godot::nativescript_init(handle);
	godot::register_class<godot::GDN_Template>();
	godot::register_class<godot::GDN_TheWorld_MainNode>();
	godot::register_class<godot::GDN_TheWorld_Globals>();
	godot::register_class<godot::GDN_TheWorld_Viewer>();
	godot::register_class<godot::GDN_TheWorld_Camera>();
	godot::register_class<godot::GDN_Chunk_MeshInstance>();
	godot::register_class<godot::GDN_TheWorld_Quadrant>();
	godot::register_class<godot::GDN_Collider_MeshInstance>();
}
