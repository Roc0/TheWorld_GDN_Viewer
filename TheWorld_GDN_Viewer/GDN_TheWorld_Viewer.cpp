//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"
#include "MeshCache.h"

using namespace godot;

void GDN_TheWorld_Viewer::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_Viewer::_ready);
	register_method("_process", &GDN_TheWorld_Viewer::_process);
	register_method("_input", &GDN_TheWorld_Viewer::_input);

	register_method("debug_print", &GDN_TheWorld_Viewer::debugPrint);
	register_method("set_debug_enabled", &GDN_TheWorld_Viewer::setDebugEnabled);
	register_method("is_debug_enabled", &GDN_TheWorld_Viewer::isDebugEnabled);
	register_method("globals", &GDN_TheWorld_Viewer::Globals);
	register_method("destroy", &GDN_TheWorld_Viewer::destroy);
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_isDebugEnabled = false;
	m_globals = GDN_TheWorld_Globals::_new();
	m_meshCache = new MeshCache(this);
	m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());
}

GDN_TheWorld_Viewer::~GDN_TheWorld_Viewer()
{
	destroy();
}

void GDN_TheWorld_Viewer::_init(void)
{
	//Godot::print("GD_ClientApp::Init");
}

void GDN_TheWorld_Viewer::_ready(void)
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

void GDN_TheWorld_Viewer::destroy(void)
{
	if (m_meshCache)
	{
		delete m_meshCache;
		m_meshCache = NULL;
	}

	if (m_globals)
	{
		m_globals->call_deferred("free");
		m_globals = NULL;
	}
}
