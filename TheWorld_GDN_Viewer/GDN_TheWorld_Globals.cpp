//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include <MapManager.h>

namespace godot
{
	GDN_TheWorld_Globals::GDN_TheWorld_Globals()
	{
		m_mapManager = new TheWorld_MapManager::MapManager();
		m_mapManager->instrument(true);
		m_mapManager->debugMode(true);

		m_numVerticesPerChuckSide = 1 << THEWORLD_CHUNK_SIZE_SHIFT;
		m_bitmapResolution = 1 << THEWORLD_BITMAP_RESOLUTION_SHIFT;
		m_lodMaxDepth = THEWORLD_BITMAP_RESOLUTION_SHIFT - THEWORLD_CHUNK_SIZE_SHIFT;
		m_numLods = m_lodMaxDepth + 1;
	}

	GDN_TheWorld_Globals::~GDN_TheWorld_Globals()
	{
		delete m_mapManager;
	};

	void GDN_TheWorld_Globals::_init(void)
	{
		//Godot::print("GD_ClientApp::Init");
	}

	void GDN_TheWorld_Globals::_ready(void)
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
}