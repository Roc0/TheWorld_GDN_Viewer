//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"
#include <SceneTree.hpp>
#include <Viewport.hpp>

#include "MeshCache.h"

#include <algorithm>

using namespace godot;

void GDN_TheWorld_Viewer::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_Viewer::_ready);
	register_method("_process", &GDN_TheWorld_Viewer::_process);
	register_method("_input", &GDN_TheWorld_Viewer::_input);

	register_method("set_initial_world_viewer_pos", &GDN_TheWorld_Viewer::setInitialWordlViewerPos);
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_initialized = false;
	m_globals = NULL;
	m_meshCache = NULL;
	m_worldViewerLevel = 0;
	m_numWordlVerticesX = 0;
	m_numWordlVerticesZ = 0;
}

GDN_TheWorld_Viewer::~GDN_TheWorld_Viewer()
{
	deinit();
}

void GDN_TheWorld_Viewer::deinit(void)
{
	if (m_initialized)
	{
		PLOGI << "TheWorld Viewer Deinitializing...";

		if (m_meshCache)
		{
			delete m_meshCache;
			m_meshCache = NULL;
		}

		m_initialized = false;
		PLOGI << "TheWorld Viewer Deinitialized!";
	}
}

void GDN_TheWorld_Viewer::_init(void)
{
	//Godot::print("GDN_TheWorld_Viewer::Init");
}

void GDN_TheWorld_Viewer::_ready(void)
{
	//Godot::print("GDN_TheWorld_Viewer::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_TheWorld_Viewer::_input(const Ref<InputEvent> event)
{
}

bool GDN_TheWorld_Viewer::init(void)
{
	PLOGI << "TheWorld Viewer Initializing...";
	
	m_meshCache = new MeshCache(this);
	m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());

	m_initialized = true;
	PLOGI << "TheWorld Viewer Initialized!";

	return true;
}

void GDN_TheWorld_Viewer::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_TheWorld_Viewer::_process");
}

GDN_TheWorld_Globals* GDN_TheWorld_Viewer::Globals(bool useCache)
{
	if (m_globals == NULL || !useCache)
	{
		SceneTree* scene = get_tree();
		if (!scene)
			return NULL;
		Viewport* root = scene->get_root();
		if (!root)
			return NULL;
		m_globals = Object::cast_to<GDN_TheWorld_Globals>(root->find_node(THEWORLD_GLOBALS_NODE_NAME, true, false));
		//m_globals = Object::cast_to<GDN_TheWorld_Globals>(get_tree()->get_root()->find_node(THEWORLD_GLOBALS_NODE_NAME, true, false));
	}

	return m_globals;
}

void GDN_TheWorld_Viewer::loadWorldData(float& x, float& z, int level)
{
	try
	{
		float gridStepInWU;
		m_numWordlVerticesX = m_numWordlVerticesZ = Globals()->bitmapResolution() + 1;
		Globals()->mapManager()->getVertices(x, z, TheWorld_MapManager::MapManager::anchorType::center, m_numWordlVerticesX, m_numWordlVerticesZ, m_worldVertices, gridStepInWU, level);
	}
	catch (TheWorld_MapManager::MapManagerException& e)
	{
		Globals()->setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, e.exceptionName() + string(" caught - ") + e.what());
		throw(e);
	}
	catch (std::exception& e)
	{
		Globals()->setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, string("std::exception caught - ") + e.what());
		throw(e);
	}
	catch (...)
	{
		Globals()->setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "exception caught");
		throw(new exception("exception caught"));
	}
}

void GDN_TheWorld_Viewer::setInitialWordlViewerPos(float x, float z, int level)
{
	m_worldViewerPos.x = x;
	m_worldViewerPos.z = z;
	m_worldViewerLevel = level;

	try
	{
		loadWorldData(x, z, level);

		TheWorld_MapManager::SQLInterface::GridVertex viewerPos(x, z, level);
		vector<TheWorld_MapManager::SQLInterface::GridVertex>::iterator it = std::find(m_worldVertices.begin(), m_worldVertices.end(), viewerPos);
		if (it == m_worldVertices.end())
		{
			Globals()->setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "Not found WorldViewer Pos");
			return;
		}
		m_worldViewerPos.y = it->altitude() + 100;
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
		Globals()->setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "std::exception caught");
		return;
	}
}
