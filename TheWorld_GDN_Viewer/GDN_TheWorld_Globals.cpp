//#include "pch.h"
#include "GDN_TheWorld_Globals.h"

using namespace godot;

void GDN_TheWorld_Globals::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_Globals::_ready);
	register_method("_process", &GDN_TheWorld_Globals::_process);
	register_method("_input", &GDN_TheWorld_Globals::_input);

	register_method("chunk_size", &GDN_TheWorld_Globals::chuckSize);
}

GDN_TheWorld_Globals::GDN_TheWorld_Globals()
{
	m_chunkSize = THEWORLD_CHUNK_SIZE;
}

GDN_TheWorld_Globals::~GDN_TheWorld_Globals()
{
}

void GDN_TheWorld_Globals::_init(void)
{
	//Godot::print("GD_ClientApp::Init");
}

void GDN_TheWorld_Globals::_ready()
{
	//Godot::print("GD_ClientApp::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_TheWorld_Globals::_input(const Ref<InputEvent> event)
{
}

void GDN_TheWorld_Globals::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GD_ClientApp::_process");
}
