//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"

using namespace godot;

void GDN_TheWorld_Viewer::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_Viewer::_ready);
	register_method("_process", &GDN_TheWorld_Viewer::_process);
	register_method("_input", &GDN_TheWorld_Viewer::_input);

	register_method("debug_print", &GDN_TheWorld_Viewer::debugPrint);
	register_method("hello", &GDN_TheWorld_Viewer::hello);
	register_method("set_debug_enabled", &GDN_TheWorld_Viewer::setDebugEnabled);
	register_method("is_debug_enabled", &GDN_TheWorld_Viewer::isDebugEnabled);
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_isDebugEnabled = false;
}

GDN_TheWorld_Viewer::~GDN_TheWorld_Viewer()
{
}

void GDN_TheWorld_Viewer::_init(void)
{
	//Godot::print("GD_ClientApp::Init");
}

void GDN_TheWorld_Viewer::_ready()
{
	//Godot::print("GD_ClientApp::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_TheWorld_Viewer::_input(const Ref<InputEvent> event)
{
}

void GDN_TheWorld_Viewer::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GD_ClientApp::_process");
}
