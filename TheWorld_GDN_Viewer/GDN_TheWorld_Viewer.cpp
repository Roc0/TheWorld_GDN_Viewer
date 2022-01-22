//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"
#include "MeshCache.h"

#include <algorithm>

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
	register_method("init", &GDN_TheWorld_Viewer::init);
	//register_method("set_initial_world_viewer_pos", &GDN_TheWorld_Viewer::setInitialWordlViewerPos);
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_isDebugEnabled = false;
	m_globals = NULL;
	m_meshCache = NULL;
	m_worldViewerLevel = 0;
}

GDN_TheWorld_Viewer::~GDN_TheWorld_Viewer()
{
	destroy();
}

void GDN_TheWorld_Viewer::_init(void)
{
	//Godot::print("GD_ClientApp::Init");
	m_globals = GDN_TheWorld_Globals::_new();
	m_meshCache = new MeshCache(this);
	m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());
}

void GDN_TheWorld_Viewer::_ready(void)
{
	//Godot::print("GD_ClientApp::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_TheWorld_Viewer::_input(const Ref<InputEvent> event)
{
}

bool GDN_TheWorld_Viewer::init(Node* pWorldNode, float x, float z, int level)
{
	PLOGI << "TheWorld Viewer Initializing ...";
	
	if (!pWorldNode)
		return false;
	
	setInitialWordlViewerPos(x, z, level);

	// Must exist a Spatial Node acting as the world; the viewer will be a child of this node
	pWorldNode->add_child(this);

	return true;
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

void GDN_TheWorld_Viewer::setInitialWordlViewerPos(float x, float z, int level)
{
	m_worldViewerPos.x = x;
	m_worldViewerPos.z = z;
	m_worldViewerLevel = level;

	vector<TheWorld_MapManager::SQLInterface::GridVertex> vertices;
	int numPointX, numPointZ;
	float gridStepInWU;
	Globals()->mapManager()->getVertices(m_worldViewerPos.x, m_worldViewerPos.z, TheWorld_MapManager::MapManager::anchorType::center, Globals()->bitmapSizeInWUs(),	vertices, numPointX, numPointZ, gridStepInWU, 0);

	try
	{
		TheWorld_MapManager::SQLInterface::GridVertex viewerPos(x, z, level);
		vector<TheWorld_MapManager::SQLInterface::GridVertex>::iterator it = std::find(vertices.begin(), vertices.end(), viewerPos);
		if (it == vertices.end())
		{
			Globals()->setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "Not found WorldViewer Pos");
			return;
		}

	}
	catch (TheWorld_MapManager::MapManagerException& e)
	{
		Globals()->setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, e.exceptionName() + string(" caught - ") + e.what());
		return;
	}
	catch (std::exception& e)
	{
		Globals()->setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, string("std::exception caught - ") + e.what());
		return;
	}
	catch (...)
	{
		Globals()->setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "std::exception caught - ");
		return;
	}
}
