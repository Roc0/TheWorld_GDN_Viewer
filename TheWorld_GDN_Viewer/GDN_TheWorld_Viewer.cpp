//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Camera.h"
#include <SceneTree.hpp>
#include <Viewport.hpp>

#include "MeshCache.h"
#include "QuadTree.h"

#include <algorithm>

using namespace godot;

// World Node Local Coordinate System is the same as MapManager coordinate system
// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
// Chunk and QuadTree coordinates ar in Viewer Node local coordinate System

void GDN_TheWorld_Viewer::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_Viewer::_ready);
	register_method("_process", &GDN_TheWorld_Viewer::_process);
	register_method("_input", &GDN_TheWorld_Viewer::_input);

	register_method("set_initial_world_viewer_pos", &GDN_TheWorld_Viewer::resetInitialWordlViewerPos);
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_initialized = false;
	m_worldViewerLevel = 0;
	m_worldCamera = NULL;
	m_numWordlVerticesX = 0;
	m_numWordlVerticesZ = 0;
	m_globals = NULL;
	m_mapScaleVector = Vector3(1, 1, 1);
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

		m_quadTree.reset();
		m_meshCache.reset();
		
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
	
	m_meshCache = make_unique<MeshCache>(this);
	m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());

	m_quadTree = make_unique<QuadTree>(this);
	
	m_initialized = true;
	PLOGI << "TheWorld Viewer Initialized!";

	return true;
}

void GDN_TheWorld_Viewer::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_TheWorld_Viewer::_process");

	if (!WorldCamera())
		return;

	GDN_TheWorld_Camera* activeCamera = WorldCamera()->getActiveCamera();
	if (!activeCamera)
		return;

	Vector3 cameraPosGlobalCoord = activeCamera->get_global_transform().get_origin();
	Transform globalTransform = internalTransformGlobalCoord();
	Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node local coordinates of the camera pos
	{
		// verify
		Vector3 cameraPosWorldNodeLocalCoord = activeCamera->get_transform().get_origin();		// WorldNode local coordinates of the camera pos
		Vector3 viewerNodePosWordlNodeLocalCoord = get_transform().get_origin();				// WorldNode local coordinates of the Viewer Node pos
		Vector3 cameraPosViewerNodeLocalCoord2 = cameraPosWorldNodeLocalCoord - viewerNodePosWordlNodeLocalCoord;	// Viewer Node local coordinates of the camera pos
		// cameraPosViewerNodeLocalCoord2 must be equal to cameraPosViewerNodeLocalCoord
	}

	m_quadTree->update(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord);
}

Transform GDN_TheWorld_Viewer::internalTransformGlobalCoord(void)
{
	// Return the transformation in global coordinates of the viewer Node appling a scale factor (m_mapScaleVector)
	return Transform(Basis().scaled(m_mapScaleVector), get_global_transform().origin);
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
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, e.exceptionName() + string(" caught - ") + e.what());
		throw(e);
	}
	catch (std::exception& e)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, string("std::exception caught - ") + e.what());
		throw(e);
	}
	catch (...)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "exception caught");
		throw(new exception("exception caught"));
	}
}

void GDN_TheWorld_Viewer::resetInitialWordlViewerPos(float x, float z, int level)
{
	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates ar in Viewer Node local coordinate System

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
			Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "Not found WorldViewer Pos");
			return;
		}
		m_worldViewerPos.y = it->altitude();

		// Viewer stuff: set viewer position relative to world node at the first point of the bitmap and altitude 0 so that that point is at position (0,0,0) respect to the viewer
		Transform t = get_transform();
		t.origin = Vector3(m_worldVertices[0].posX(), 0, m_worldVertices[0].posZ());
		set_transform(t);
		
		// Camera stuff
		if (WorldCamera())
		{
			WorldCamera()->deactivateCamera();
			WorldCamera()->queue_free();
		}
		assignWorldCamera(GDN_TheWorld_Camera::_new());
		getWorldNode()->add_child(WorldCamera());		// Viewer and WorldCamera are at the same level : both child of WorldNode
		Vector3 cameraPos(m_worldViewerPos.x, m_worldViewerPos.y + 100, m_worldViewerPos.z);	// MapManager coordinates are local coordinates of WorldNode
		Vector3 lookAt(m_worldViewerPos.x, m_worldViewerPos.y, m_worldViewerPos.z);
		WorldCamera()->initCameraInWorld(cameraPos, lookAt);
	}
	catch (TheWorld_MapManager::MapManagerException& e)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, e.exceptionName() + string(" caught - ") + e.what());
		return;
	}
	catch (std::exception& e)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, string("std::exception caught - ") + e.what());
		return;
	}
	catch (...)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "std::exception caught");
		return;
	}
}

Spatial* GDN_TheWorld_Viewer::getWorldNode(void)
{
	// MapManager coordinates are relative to WorldNode
	return (Spatial*)get_parent();
}