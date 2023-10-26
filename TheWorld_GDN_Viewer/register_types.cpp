//
// W A R N I N G : Remember to set precompiler variable TYPED_METHOD_BIND
//

#include "register_types.h"

#include "GDN_Template.h"
#include "GDN_TheWorld_MainNode.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Camera.h"
#include "GDN_TheWorld_Quadrant.h"
#include "GDN_TheWorld_Edit.h"
#include "Chunk.h"
#include "Tools.h"

#pragma warning(push, 0)
#include <gdextension_interface.h>
//#include <godot_cpp/core/defs.hpp>
//#include <godot_cpp/core/class_db.hpp>
//#include <godot_cpp/godot.hpp>
#pragma warning(pop)

using namespace godot;

void initialize_tw_main_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    //ClassDB::register_class<godot::GDN_Template>();
    ClassDB::register_class<godot::GDN_TheWorld_MainNode>();
    ClassDB::register_class<godot::GDN_TheWorld_Globals>();
    ClassDB::register_class<godot::GDN_TheWorld_Viewer>();
    ClassDB::register_class<godot::GDN_TheWorld_Camera>();
    ClassDB::register_class<godot::GDN_TheWorld_Quadrant>();
    ClassDB::register_class<godot::GDN_TheWorld_Edit>();
    ClassDB::register_class<godot::GDN_TheWorld_MapModder>();
    ClassDB::register_class<godot::GDN_TheWorld_Drawer>();
    ClassDB::register_class<godot::GDN_TheWorld_Gizmo3d>();
}

void uninitialize_tw_main_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
}

extern "C"
{
    // Initialization.
    GDExtensionBool GDE_EXPORT tw_main_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization)
    //GDExtensionBool GDE_EXPORT tw_main_library_init(const GDExtensionInterfaceGetProcAddress* p_interface, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization)
    {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_tw_main_module);
        init_obj.register_terminator(uninitialize_tw_main_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}