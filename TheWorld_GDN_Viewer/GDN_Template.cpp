//#include "pch.h"
#include "GDN_Template.h"

using namespace godot;

void GDN_Template::_register_methods()
{
	register_method("_ready", &GDN_Template::_ready);
	register_method("_process", &GDN_Template::_process);
	register_method("_input", &GDN_Template::_input);

	register_method("hello", &GDN_Template::hello);
}

GDN_Template::GDN_Template()
{
	m_initialized = false;
}

GDN_Template::~GDN_Template()
{
	deinit();
}

void GDN_Template::init(void)
{
	m_initialized = true;
}

void GDN_Template::deinit(void)
{
	if (m_initialized)
	{
		m_initialized = false;
	}
}

void GDN_Template::_init(void)
{
	//Godot::print("GDN_Template::Init");
}

void GDN_Template::_ready(void)
{
	//Godot::print("GDN_Template::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_Template::_input(const Ref<InputEvent> event)
{
}

void GDN_Template::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_Template::_process");
}
