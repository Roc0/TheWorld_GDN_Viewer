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
	register_method("_notification", &GDN_TheWorld_Viewer::_notification);

	register_method("set_initial_world_viewer_pos", &GDN_TheWorld_Viewer::resetInitialWordlViewerPos);
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_initialized = false;
	m_worldViewerLevel = 0;
	m_worldCamera = NULL;
	m_numWorldVerticesX = 0;
	m_numWorldVerticesZ = 0;
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

void GDN_TheWorld_Viewer::_notification(int p_what)
{
	switch (p_what)
	{
		case NOTIFICATION_PREDELETE:
		{
			Globals()->debugPrint("Destroy Viewer");
			deinit();
		}
		break;
		case NOTIFICATION_ENTER_WORLD:
		{
			Globals()->debugPrint("Enter world");
			Chunk::EnterWorldChunkAction action;
			m_quadTree->ForAllChunk(action);
		}
		break;
		case NOTIFICATION_EXIT_WORLD:
		{
			Globals()->debugPrint("Exit world");
			Chunk::ExitWorldChunkAction action;
			m_quadTree->ForAllChunk(action);
		}
		break;
		case NOTIFICATION_VISIBILITY_CHANGED:
		{
			Globals()->debugPrint("Visibility changed");
			Chunk::VisibilityChangedChunkAction action(is_visible_in_tree());
			m_quadTree->ForAllChunk(action);
		}
		break;
		case NOTIFICATION_TRANSFORM_CHANGED:
			onTransformChanged();
			break;
	// TODORIC
	}
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

	// All chunk that need an update (they are chunk that got split or joined)
	std::vector<Chunk*> vectChunkUpdate = m_quadTree->getChunkUpdate();

	// Forcing update to all neighboring chunks to readjust the seams
	std::vector<Chunk*> vectAdditionalUpdateChunk;
	for (std::vector<Chunk*>::iterator it = vectChunkUpdate.begin(); it != vectChunkUpdate.end(); it++)
	{
		Chunk::ChunkPos pos = (*it)->getPos();

		// In case the chunk got split forcing update to left, right, bottom, top chunks
		Chunk* chunk = m_quadTree->getChunkAt(pos, Chunk::DirectionSlot::Left);
		if (chunk != nullptr && chunk->isActive())
			vectAdditionalUpdateChunk.push_back(chunk);
		chunk = m_quadTree->getChunkAt(pos, Chunk::DirectionSlot::Right);
		if (chunk != nullptr && chunk->isActive())
			vectAdditionalUpdateChunk.push_back(chunk);
		chunk = m_quadTree->getChunkAt(pos, Chunk::DirectionSlot::Bottom);
		if (chunk != nullptr && chunk->isActive())
			vectAdditionalUpdateChunk.push_back(chunk);
		chunk = m_quadTree->getChunkAt(pos, Chunk::DirectionSlot::Top);
		if (chunk != nullptr && chunk->isActive())
			vectAdditionalUpdateChunk.push_back(chunk);

		// In case the chunk got joined
		int lod = pos.getLod();
		if (lod > 0 && (*it)->gotJustJoined())
		{
			(*it)->setJustJoined(false);

			Chunk::ChunkPos pos1(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::LeftTop);
			if (chunk != nullptr && chunk->isActive())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::LeftBottom);
			if (chunk != nullptr && chunk->isActive())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::RightTop);
			if (chunk != nullptr && chunk->isActive())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::RightBottom);
			if (chunk != nullptr && chunk->isActive())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::BottomLeft);
			if (chunk != nullptr && chunk->isActive())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::BottomRight);
			if (chunk != nullptr && chunk->isActive())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::TopLeft);
			if (chunk != nullptr && chunk->isActive())
				vectAdditionalUpdateChunk.push_back(chunk);
			chunk = m_quadTree->getChunkAt(pos1, Chunk::DirectionSlot::TopRight);
			if (chunk != nullptr && chunk->isActive())
				vectAdditionalUpdateChunk.push_back(chunk);
		}
	}

	for (std::vector<Chunk*>::iterator it = vectChunkUpdate.begin(); it != vectChunkUpdate.end(); it++)
		(*it)->update(is_visible_in_tree());

	m_quadTree->clearChunkUpdate();
}

Transform GDN_TheWorld_Viewer::internalTransformGlobalCoord(void)
{
	// TODORIC mah
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

void GDN_TheWorld_Viewer::resetInitialWordlViewerPos(float x, float z, int level)
{
	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System

	Vector3 worldViewerPos(x, 0, z);
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
		worldViewerPos.y = it->altitude();

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
		Vector3 cameraPos(worldViewerPos.x, worldViewerPos.y + 100, worldViewerPos.z);	// MapManager coordinates are local coordinates of WorldNode
		Vector3 lookAt(worldViewerPos.x, worldViewerPos.y, worldViewerPos.z);
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

void GDN_TheWorld_Viewer::getPartialAABB(AABB& aabb, int firstWorldVertCol, int lastWorldVertCol, int firstWorldVertRow, int lastWorldVertRow, int step)
{
	int idxlowerLeftVert = m_numWorldVerticesX * firstWorldVertRow + firstWorldVertCol;
	int idxlowerRightVert = m_numWorldVerticesX * firstWorldVertRow + lastWorldVertCol;
	int idxUpperLeftVert = m_numWorldVerticesX * lastWorldVertRow + firstWorldVertCol;
	int idxUpperRightVert = m_numWorldVerticesX * lastWorldVertRow + lastWorldVertCol;

	// altitudes
	float minHeigth = 0, maxHeigth = 0;
	for (int idxRow = 0; idxRow < lastWorldVertRow - firstWorldVertRow + 1; idxRow += step)
	{
		for (int idxVert = idxlowerLeftVert + idxRow * m_numWorldVerticesX; idxVert < idxlowerRightVert + idxRow * m_numWorldVerticesX + 1; idxVert += step)
		{
			minHeigth = min2(minHeigth, m_worldVertices[idxVert].altitude());
			maxHeigth = max2(maxHeigth, m_worldVertices[idxVert].altitude());
		}
	}
	
	Vector3 startPosition(m_worldVertices[idxlowerLeftVert].posX(), minHeigth, m_worldVertices[idxlowerLeftVert].posZ());
	Vector3 size(m_worldVertices[idxUpperRightVert].posX() - m_worldVertices[idxlowerLeftVert].posX(), maxHeigth, m_worldVertices[idxUpperRightVert].posZ() - m_worldVertices[idxlowerLeftVert].posZ());

	aabb.set_position(startPosition);
	aabb.set_size(size);
}

void GDN_TheWorld_Viewer::onTransformChanged(void)
{
	Globals()->debugPrint("Transform changed");
	
	//The transformand other properties can be set by the scene loader, before we enter the tree
	if (!is_inside_tree())
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
