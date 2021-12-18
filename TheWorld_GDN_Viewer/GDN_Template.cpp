//#include "pch.h"
#include "GDN_Template.h"

using namespace godot;

void GDN_Template::_register_methods()
{
	register_method("_ready", &GDN_Template::_ready);
	register_method("_process", &GDN_Template::_process);
	register_method("_input", &GDN_Template::_input);

	register_method("debug_print", &GDN_Template::debugPrint);
	register_method("hello", &GDN_Template::hello);
	register_method("set_debug_enabled", &GDN_Template::setDebugEnabled);
	register_method("is_debug_enabled", &GDN_Template::isDebugEnabled);
}

GDN_Template::GDN_Template()
{
	m_isDebugEnabled = false;
}

GDN_Template::~GDN_Template()
{
}

void GDN_Template::_init(void)
{
	//Godot::print("GD_ClientApp::Init");
}

void GDN_Template::_ready(void)
{
	//Godot::print("GD_ClientApp::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_Template::_input(const Ref<InputEvent> event)
{
}

void GDN_Template::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GD_ClientApp::_process");
}
