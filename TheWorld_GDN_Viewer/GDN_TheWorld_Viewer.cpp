//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Camera.h"
#include <SceneTree.hpp>
#include <Viewport.hpp>
#include <Engine.hpp>
#include <OS.hpp>
#include <Input.hpp>
#include <VisualServer.hpp>

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
	register_method("_physics_process", &GDN_TheWorld_Viewer::_physics_process);
	register_method("_input", &GDN_TheWorld_Viewer::_input);
	register_method("_notification", &GDN_TheWorld_Viewer::_notification);

	register_method("reset_initial_world_viewer_pos", &GDN_TheWorld_Viewer::resetInitialWordlViewerPos);
	register_method("dump_required", &GDN_TheWorld_Viewer::setDumpRequired);
	register_method("get_camera_chunk_global_transform_of_aabb", &GDN_TheWorld_Viewer::getCameraChunkGlobalTransformOfAABB);
	register_method("get_camera_chunk_id", &GDN_TheWorld_Viewer::getCameraChunkId);
	register_method("get_num_splits", &GDN_TheWorld_Viewer::getNumSplits);
	register_method("get_num_joins", &GDN_TheWorld_Viewer::getNumJoins);
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_initialized = false;
	m_firstProcess = true;
	m_initialWordlViewerPosSet = false;
	m_dumpRequired = false;
	m_worldViewerLevel = 0;
	m_worldCamera = nullptr;
	m_cameraChunk = nullptr;
	m_numWorldVerticesX = 0;
	m_numWorldVerticesZ = 0;
	m_globals = nullptr;
	m_mapScaleVector = Vector3(1, 1, 1);
	m_timeElapsedFromLastDump = 0;
	m_debugVisibility = true;
	m_updateTerrainVisibilityRequired = false;
	m_debugMode = GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB;
	m_updateDebugModeRequired = false;
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
		
		Globals()->debugPrint("GDN_TheWorld_Viewer::deinit DONE!");
	}

}

void GDN_TheWorld_Viewer::_init(void)
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//Godot::print("GDN_TheWorld_Viewer::_init");
}

void GDN_TheWorld_Viewer::_ready(void)
{
	Globals()->debugPrint("GDN_TheWorld_Viewer::_ready");

	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");

	// Camera stuff
	if (WorldCamera())
	{
		WorldCamera()->deactivateCamera();
		WorldCamera()->queue_free();
	}
	assignWorldCamera(GDN_TheWorld_Camera::_new());
	//getWorldNode()->add_child(WorldCamera());		// Viewer and WorldCamera are at the same level : both child of WorldNode
	getWorldNode()->call_deferred("add_child", WorldCamera());
}

void GDN_TheWorld_Viewer::_input(const Ref<InputEvent> event)
{
	if (event->is_action_pressed("ui_toggle_debug_visibility"))
	{
		m_debugVisibility = !m_debugVisibility;
		m_updateTerrainVisibilityRequired = true;
	}
	
	if (event->is_action_pressed("ui_toggle_debug_mode"))
	{

		m_debugMode = GDN_TheWorld_Globals::toggleChunkDebugMode(m_debugMode);
		m_updateDebugModeRequired = true;
	}
	if (event->is_action_pressed("ui_dump"))
	{
		m_dumpRequired = true;
	}
}

void GDN_TheWorld_Viewer::_notification(int p_what)
{
	switch (p_what)
	{
		case NOTIFICATION_PREDELETE:
		{
			Globals()->debugPrint("GDN_TheWorld_Viewer::_notification - Destroy Viewer");
			deinit();
		}
		break;
		case NOTIFICATION_ENTER_WORLD:
		{
			Globals()->debugPrint("Enter world");
			if (m_quadTree && m_initialWordlViewerPosSet)
			{
				Chunk::EnterWorldChunkAction action;
				m_quadTree->ForAllChunk(action);
			}
		}
		break;
		case NOTIFICATION_EXIT_WORLD:
		{
			Globals()->debugPrint("Exit world");
			if (m_quadTree && m_initialWordlViewerPosSet)
			{
				Chunk::ExitWorldChunkAction action;
				m_quadTree->ForAllChunk(action);
			}
		}
		break;
		case NOTIFICATION_VISIBILITY_CHANGED:
		{
			Globals()->debugPrint("Visibility changed");
			if (m_quadTree && m_initialWordlViewerPosSet)
			{
				Chunk::VisibilityChangedChunkAction action(is_visible_in_tree());
				m_quadTree->ForAllChunk(action);
			}
		}
		break;
		case NOTIFICATION_TRANSFORM_CHANGED:
			if (m_quadTree && m_initialWordlViewerPosSet)
			{
				onTransformChanged();
			}
			break;
	// TODORIC
	}
}

bool GDN_TheWorld_Viewer::init(void)
{
	PLOGI << "TheWorld Viewer Initializing...";
	
	m_meshCache = make_unique<MeshCache>(this);
	m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());

	m_initialized = true;
	PLOGI << "TheWorld Viewer Initialized!";

	return true;
}

void GDN_TheWorld_Viewer::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_TheWorld_Viewer::_process");

	if (!WorldCamera() || !m_initialWordlViewerPosSet)
		return;

	GDN_TheWorld_Camera* activeCamera = WorldCamera()->getActiveCamera();
	if (!activeCamera)
		return;

	if (m_quadTree == nullptr)
	{
		m_quadTree = make_unique<QuadTree>(this);
		m_quadTree->init();
	}

	if (m_firstProcess)
	{
		// TODORIC
		//Ref<Mesh> _mesh = activeCamera->DrawViewFrustum(GDN_TheWorld_Globals::g_color_green);
		//SpatialMaterial* mat = SpatialMaterial::_new();
		//mat->set_flag(SpatialMaterial::Flags::FLAG_UNSHADED, true);
		//mat->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		//mat->set_albedo(GDN_TheWorld_Globals::g_color_green);
		//_mesh->surface_set_material(0, mat);
		//VisualServer* vs = VisualServer::get_singleton();
		//m_viewFrustumMeshInstance = vs->instance_create();
		//vs->instance_set_visible(m_viewFrustumMeshInstance, true);
		//RID meshRID = _mesh->get_rid();
		//VisualServer::get_singleton()->instance_set_base(m_viewFrustumMeshInstance, meshRID);
		//m_viewFrustumMeshRID = meshRID;
		//m_viewFrustumMesh = _mesh;

		m_firstProcess = false;
	}
	
	Vector3 cameraPosGlobalCoord = activeCamera->get_global_transform().get_origin();
	Transform globalTransform = internalTransformGlobalCoord();
	Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node (grid) local coordinates of the camera pos
	// DEBUG: verify
	{
		Vector3 cameraPosWorldNodeLocalCoord = activeCamera->get_transform().get_origin();		// WorldNode local coordinates of the camera pos
		Vector3 viewerNodePosWordlNodeLocalCoord = get_transform().get_origin();				// WorldNode local coordinates of the Viewer Node pos
		Vector3 cameraPosViewerNodeLocalCoord2 = cameraPosWorldNodeLocalCoord - viewerNodePosWordlNodeLocalCoord;	// Viewer Node local coordinates of the camera pos
		// cameraPosViewerNodeLocalCoord2 must be equal to cameraPosViewerNodeLocalCoord
		Vector3 viewerPosGlobalCoord = get_global_transform().get_origin();		// da mettere in relazione con cameraPosGlobalCoord
		
		Transform globalTransform = internalTransformGlobalCoord();
	}
	// DEBUG: verify

	m_quadTree->update(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord);

	// All chunk that need an update (they are chunk that got split or joined)
	std::vector<Chunk*> vectChunkUpdate = m_quadTree->getChunkUpdate();

	// Forcing update to all neighboring chunks to readjust the seams
	std::vector<Chunk*> vectAdditionalUpdateChunk;
	for (std::vector<Chunk*>::iterator it = vectChunkUpdate.begin(); it != vectChunkUpdate.end(); it++)
	{
		if (!(*it)->isActive())
			continue;
		
		Chunk::ChunkPos pos = (*it)->getPos();

		// In case the chunk got split forcing update to left, right, bottom, top chunks
		Chunk* chunk = m_quadTree->getChunkAt(pos, Chunk::DirectionSlot::Left);
		if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
			vectAdditionalUpdateChunk.push_back(chunk);
		chunk = m_quadTree->getChunkAt(pos, Chunk::DirectionSlot::Right);
		if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
			vectAdditionalUpdateChunk.push_back(chunk);
		chunk = m_quadTree->getChunkAt(pos, Chunk::DirectionSlot::Bottom);
		if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
			vectAdditionalUpdateChunk.push_back(chunk);
		chunk = m_quadTree->getChunkAt(pos, Chunk::DirectionSlot::Top);
		if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
			vectAdditionalUpdateChunk.push_back(chunk);

		// In case the chunk got joined
		int lod = pos.getLod();
		if (lod > 0 && (*it)->gotJustJoined())
		{
			(*it)->setJustJoined(false);

			Chunk::ChunkPos pos1(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::LeftTop);
			if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::LeftBottom);
			if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::RightTop);
			if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::RightBottom);
			if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::BottomLeft);
			if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::BottomRight);
			if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::TopLeft);
			if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::TopRight);
			if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				vectAdditionalUpdateChunk.push_back(chunk);
		}
		// Anycase clear flag just joined
		if ((*it)->gotJustJoined())
			(*it)->setJustJoined(false);
	}

	for (std::vector<Chunk*>::iterator it = vectChunkUpdate.begin(); it != vectChunkUpdate.end(); it++)
		(*it)->update(is_visible_in_tree());

	for (std::vector<Chunk*>::iterator it = vectAdditionalUpdateChunk.begin(); it != vectAdditionalUpdateChunk.end(); it++)
		(*it)->update(is_visible_in_tree());

	m_quadTree->clearChunkUpdate();
	vectAdditionalUpdateChunk.clear();

	// Check for Dump
	if (TIME_INTERVAL_BETWEEN_DUMP != 0 && Globals()->isDebugEnabled())
	{
		int64_t timeElapsed = OS::get_singleton()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastDump > TIME_INTERVAL_BETWEEN_DUMP * 1000)
		{
			m_dumpRequired = true;
			m_timeElapsedFromLastDump = timeElapsed;
		}
	}
	if (m_dumpRequired)
	{
		m_dumpRequired = false;
		dump();
	}

	if (m_updateTerrainVisibilityRequired)
	{
		if (m_quadTree && m_initialWordlViewerPosSet)
		{
			Chunk::DebugVisibilityChangedChunkAction action(m_debugVisibility);
			m_quadTree->ForAllChunk(action);
		}
		m_updateTerrainVisibilityRequired = false;
	}

	if (m_updateDebugModeRequired)
	{
		if (m_quadTree && m_initialWordlViewerPosSet)
		{
			Chunk::SwitchDebugModeAction action(m_debugMode);
			m_quadTree->ForAllChunk(action);
		}
		m_updateDebugModeRequired = false;
	}
}

void GDN_TheWorld_Viewer::_physics_process(float _delta)
{
}

Transform GDN_TheWorld_Viewer::internalTransformGlobalCoord(void)
{
	// Return the transformation of the viewer Node in global coordinates appling a scale factor (m_mapScaleVector)
	return Transform(Basis().scaled(m_mapScaleVector), get_global_transform().origin);
}

Transform GDN_TheWorld_Viewer::internalTransformLocalCoord(void)
{
	// Return the transformation of the viewer Node in local coordinates relative to itself appling a scale factor (m_mapScaleVector)
	return Transform(Basis().scaled(m_mapScaleVector), Vector3(0, 0, 0));
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
		m_numWorldVerticesX = m_numWorldVerticesZ = Globals()->bitmapResolution() + 1;
		Globals()->mapManager()->getVertices(x, z, TheWorld_MapManager::MapManager::anchorType::center, m_numWorldVerticesX, m_numWorldVerticesZ, m_worldVertices, gridStepInWU, level);
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

void GDN_TheWorld_Viewer::resetInitialWordlViewerPos(float x, float z, float cameraDistanceFromTerrain, int level)
{
	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System

	m_initialWordlViewerPosSet = false;
	
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
		Vector3 cameraPos(x, it->altitude() + cameraDistanceFromTerrain, z);		// MapManager coordinates are local coordinates of WorldNode
		Vector3 lookAt(x + cameraDistanceFromTerrain, it->altitude(), z + cameraDistanceFromTerrain);

		// Viewer stuff: set viewer position relative to world node at the first point of the bitmap and altitude 0 so that that point is at position (0,0,0) respect to the viewer
		Transform t = get_transform();
		t.origin = Vector3(m_worldVertices[0].posX(), 0, m_worldVertices[0].posZ());
		set_transform(t);
		
		WorldCamera()->initCameraInWorld(cameraPos, lookAt);

		if (m_quadTree)
			m_quadTree.reset();
		//m_quadTree = make_unique<QuadTree>(this);
		//m_quadTree->init();

		m_initialWordlViewerPosSet = true;
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

void GDN_TheWorld_Viewer::getPartialAABB(AABB& aabb, int firstWorldVertCol, int lastWorldVertCol, int firstWorldVertRow, int lastWorldVertRow, int step)
{
	int idxFirstColFirstRowWorldVert = m_numWorldVerticesX * firstWorldVertRow + firstWorldVertCol;
	int idxLastColFirstRowWorldVert = m_numWorldVerticesX * firstWorldVertRow + lastWorldVertCol;
	int idxFirstColLastRowWorldVert = m_numWorldVerticesX * lastWorldVertRow + firstWorldVertCol;
	int idxLastColLastRowWorldVert = m_numWorldVerticesX * lastWorldVertRow + lastWorldVertCol;

	// altitudes
	float minHeigth = 0, maxHeigth = 0;
	bool firstTime = true;
	for (int idxRow = 0; idxRow < lastWorldVertRow - firstWorldVertRow + 1; idxRow += step)
	{
		for (int idxVert = idxFirstColFirstRowWorldVert + idxRow * m_numWorldVerticesX; idxVert < idxLastColFirstRowWorldVert + idxRow * m_numWorldVerticesX + 1; idxVert += step)
		{
			if (firstTime)
			{
				minHeigth = maxHeigth = m_worldVertices[idxVert].altitude();
				firstTime = false;
			}
			else
			{
				minHeigth = min2(minHeigth, m_worldVertices[idxVert].altitude());
				maxHeigth = max2(maxHeigth, m_worldVertices[idxVert].altitude());
			}
		}
	}
	
	Vector3 startPosition(m_worldVertices[idxFirstColFirstRowWorldVert].posX(), minHeigth, m_worldVertices[idxFirstColFirstRowWorldVert].posZ());
	//Vector3 endPosition(m_worldVertices[idxLastColLastRowWorldVert].posX() - m_worldVertices[idxFirstColFirstRowWorldVert].posX(), maxHeigth, m_worldVertices[idxLastColLastRowWorldVert].posZ() - m_worldVertices[idxFirstColFirstRowWorldVert].posZ());
	Vector3 endPosition(m_worldVertices[idxLastColLastRowWorldVert].posX(), maxHeigth, m_worldVertices[idxLastColLastRowWorldVert].posZ());
	Vector3 size = endPosition - startPosition;

	aabb.set_position(startPosition);
	aabb.set_size(size);
}

void GDN_TheWorld_Viewer::onTransformChanged(void)
{
	Globals()->debugPrint("Transform changed");
	
	//The transformand other properties can be set by the scene loader, before we enter the tree
	if (!is_inside_tree())
		return;

	if (!m_quadTree)
		return;

	Transform gt = internalTransformGlobalCoord();

	Chunk::TransformChangedChunkAction action(gt);
	m_quadTree->ForAllChunk(action);

	// TODORIC Material stuff
	//_material_params_need_update = true
}

void GDN_TheWorld_Viewer::setMapScale(Vector3 mapScaleVector)
{
	if (m_mapScaleVector == mapScaleVector)
		return;

	mapScaleVector.x = max2(mapScaleVector.x, MIN_MAP_SCALE);
	mapScaleVector.y = max2(mapScaleVector.y, MIN_MAP_SCALE);
	mapScaleVector.z = max2(mapScaleVector.z, MIN_MAP_SCALE);

	m_mapScaleVector = mapScaleVector;

	onTransformChanged();
}

Transform GDN_TheWorld_Viewer::getCameraChunkGlobalTransformOfAABB(void)
{
	if (m_cameraChunk)
		return m_cameraChunk->getGlobalTransformOfAABB();
	else
		return Transform();
}

String GDN_TheWorld_Viewer::getCameraChunkId(void)
{
	if (m_cameraChunk)
		return m_cameraChunk->getPos().getId().c_str();
	else
		return "";
}

int GDN_TheWorld_Viewer::getNumSplits()
{
	return m_quadTree ? m_quadTree->getNumSplits() : 0;
}

int GDN_TheWorld_Viewer::getNumJoins()
{
	return m_quadTree ? m_quadTree->getNumJoins() : 0;
}

void GDN_TheWorld_Viewer::dump()
{
	if (!m_quadTree)
		return;

	Globals()->debugPrint("*************");
	Globals()->debugPrint("STARTING DUMP");

	float f = Engine::get_singleton()->get_frames_per_second();
	Globals()->debugPrint("FPS: " + String(std::to_string(f).c_str()));
	m_quadTree->dump();

	Globals()->debugPrint("DUMP COMPLETE");
	Globals()->debugPrint("*************");
}