
#include "GDN_Template.h"

//#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void GDN_Template::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("hello"), &GDN_Template::hello);

	//ClassDB::bind_method(D_METHOD("get_amplitude"), &GDExample::get_amplitude);
	//ClassDB::bind_method(D_METHOD("set_amplitude", "p_amplitude"), &GDExample::set_amplitude);
	//ClassDB::add_property("GDExample", PropertyInfo(Variant::FLOAT, "amplitude"), "set_amplitude", "get_amplitude");
}

GDN_Template::GDN_Template()
{
	m_initialized = false;
	m_ready = false;
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

//void GDN_Template::_init(void)
//{
//	//Godot::print("GDN_Template::Init");
//}

void GDN_Template::_ready(void)
{
	//Godot::print("GDN_Template::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
	m_ready = true;
}

void GDN_Template::_input(const Ref<InputEvent>& event)
{
}

void GDN_Template::_process(double _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_Template::_process");
}
