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
#include <ResourceLoader.hpp>
#include <Shader.hpp>
#include <ImageTexture.hpp>
#include <File.hpp>

#include "MeshCache.h"
#include "QuadTree.h"

#include <algorithm>

using namespace godot;
using namespace std;

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
	//register_method("get_camera_chunk_global_transform_of_aabb", &GDN_TheWorld_Viewer::getCameraChunkGlobalTransformOfAABB);
	register_method("get_camera_chunk_local_aabb", &GDN_TheWorld_Viewer::getCameraChunkLocalAABB);
	register_method("get_camera_chunk_local_debug_aabb", &GDN_TheWorld_Viewer::getCameraChunkLocalDebugAABB);
	register_method("get_camera_chunk_mesh_global_transform_applied", &GDN_TheWorld_Viewer::getCameraChunkMeshGlobalTransformApplied);
	register_method("get_camera_chunk_debug_mesh_global_transform_applied", &GDN_TheWorld_Viewer::getCameraChunkDebugMeshGlobalTransformApplied);
	register_method("get_camera_chunk_id", &GDN_TheWorld_Viewer::getCameraChunkId);
	register_method("get_num_splits", &GDN_TheWorld_Viewer::getNumSplits);
	register_method("get_num_joins", &GDN_TheWorld_Viewer::getNumJoins);
	register_method("get_num_chunks", &GDN_TheWorld_Viewer::getNumChunks);
	register_method("get_debug_draw_mode", &GDN_TheWorld_Viewer::getDebugDrawMode);
	register_method("get_chunk_debug_mode", &GDN_TheWorld_Viewer::getChunkDebugModeStr);
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_initialized = false;
	m_useVisualServer = true;
	m_firstProcess = true;
	m_initialWordlViewerPosSet = false;
	m_dumpRequired = false;
	m_refreshRequired = false;
	m_worldViewerLevel = 0;
	m_worldCamera = nullptr;
	m_cameraChunk = nullptr;
	//m_numWorldVerticesX = 0;
	//m_numWorldVerticesZ = 0;
	m_globals = nullptr;
	m_mapScaleVector = Vector3(1, 1, 1);
	m_timeElapsedFromLastDump = 0;
	m_debugVisibility = true;
	m_updateTerrainVisibilityRequired = false;
	m_currentChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
	m_requiredChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
	m_updateDebugModeRequired = false;
	m_debugDraw = Viewport::DebugDraw::DEBUG_DRAW_DISABLED;
	m_ctrlPressed = false;	
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

		m_mapQuadTree.clear();
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

	if (VISUAL_SERVER_WIREFRAME_MODE)
		VisualServer::get_singleton()->set_debug_generate_wireframes(true);
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
	
	if (event->is_action_pressed("ui_rotate_chunk_debug_mode"))
	{

		m_requiredChunkDebugMode = GDN_TheWorld_Globals::rotateChunkDebugMode(m_currentChunkDebugMode);
		m_updateDebugModeRequired = true;
	}
	
	if (event->is_action_pressed("ui_rotate_drawing_mode"))
	{
		Viewport* vp = get_viewport();
		Viewport::DebugDraw dd = vp->get_debug_draw();
		vp->set_debug_draw((dd + 1) % 4);
		m_debugDraw = vp->get_debug_draw();
	}

	if (event->is_action_pressed("ui_dump") && !m_ctrlPressed)
	{
		m_dumpRequired = true;
	}

	if (event->is_action_pressed("ui_refresh"))
	{
		m_refreshRequired = true;
	}
}

void GDN_TheWorld_Viewer::printKeyboardMapping(void)
{
	Globals()->print("");
	Globals()->print("===========================================================================");
	Globals()->print("");
	Globals()->print("KEYBOARD MAPPING");
	Globals()->print("");
	Globals()->print("LEFT					==> A	MOUSE BUTTON LEFT");
	Globals()->print("RIGHT					==> D	MOUSE BUTTON LEFT");
	Globals()->print("FORWARD					==>	W	MOUSE BUTTON MID	MOUSE WHEEL UP (WITH ALT MOVE ON MAP, WITHOUT MOVE FORWARD IN THE DIRECTION OF CAMERA)");
	Globals()->print("BACKWARD				==>	S	MOUSE BUTTON MID	MOUSE WHEEL DOWN (WITH ALT MOVE ON MAP, WITHOUT MOVE BACKWARD IN THE DIRECTION OF CAMERA)");
	Globals()->print("UP						==>	Q");
	Globals()->print("DOWN					==>	E");
	Globals()->print("");
	Globals()->print("SHIFT					==>	LOW MOVEMENT");
	Globals()->print("CTRL-ALT-D				==> FAST MOVEMENT");
	Globals()->print("");
	Globals()->print("ROTATE CAMERA			==>	MOUSE BUTTON RIGHT");
	Globals()->print("SHIFT ORI CAMERA		==>	MOUSE BUTTON LEFT");
	Globals()->print("SHIFT VERT CAMERA		==>	MOUSE BUTTON MID");
	Globals()->print("");
	Globals()->print("TOGGLE DEBUG STATS		==>	CTRL-ALT-D");
	Globals()->print("DUMP					==>	ALT-D");
	Globals()->print("");
	Globals()->print("TOGGLE DEBUG VISIBILITY	==>	F1");
	Globals()->print("ROTATE CHUNK DEBUG MODE	==>	F2");
	Globals()->print("ROTATE DRAWING MODE		==>	F3");
	Globals()->print("REFRESH VIEW			==>	F4");
	Globals()->print("");
	Globals()->print("EXIT					==>	ESC");
	Globals()->print("");
	Globals()->print("===========================================================================");
	Globals()->print("");
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
			printKeyboardMapping();
			string s = "Use Visual Server: "; s += (m_useVisualServer ? "True" : "False");
			Globals()->infoPrint(s.c_str());
			Globals()->debugPrint("Enter world");
			if (m_initialWordlViewerPosSet)
				for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
				{
					if (!itQuadTree->second->isValid())
						continue;
					Chunk::EnterWorldChunkAction action;
					itQuadTree->second->ForAllChunk(action);
				}
		}
		break;
		case NOTIFICATION_EXIT_WORLD:
		{
			Globals()->debugPrint("Exit world");
			if (m_initialWordlViewerPosSet)
				for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
				{
					if (!itQuadTree->second->isValid())
						continue;
					Chunk::ExitWorldChunkAction action;
					itQuadTree->second->ForAllChunk(action);
				}
		}
		break;
		case NOTIFICATION_VISIBILITY_CHANGED:
		{
			Globals()->debugPrint("Visibility changed");
			if (m_initialWordlViewerPosSet)
				for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
				{
					if (!itQuadTree->second->isValid())
						continue;
					Chunk::VisibilityChangedChunkAction action(is_visible_in_tree());
					itQuadTree->second->ForAllChunk(action);
				}
		}
		break;
		case NOTIFICATION_TRANSFORM_CHANGED:
		{
			if (m_initialWordlViewerPosSet)
			{
				onTransformChanged();
			}
		}
		break;
	// TODORIC
	}
}

String GDN_TheWorld_Viewer::getDebugDrawMode(void)
{
	if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_DISABLED)
		return "DEBUG_DRAW_DISABLED";
	else 	if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_UNSHADED)
		return "DEBUG_DRAW_UNSHADED";
	else 	if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_OVERDRAW)
		return "DEBUG_DRAW_OVERDRAW";
	else 	if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_WIREFRAME)
		return "DEBUG_DRAW_WIREFRAME";
	else
		return "";
}

String GDN_TheWorld_Viewer::getChunkDebugModeStr(void)
{
	if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::DoNotSet)
		return "CHUNK_DEBUG_MODE_DO_NOT_SET";
	else 	if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
		return "CHUNK_DEBUG_MODE_NO_DEBUG";
	else 	if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB)
		return "CHUNK_DEBUG_MODE_WIREFRAME_AABB";
	else 	if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
		return "CHUNK_DEBUG_MODE_WIREFRAME_SQUARE";
	else
		return "";
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

	//if (m_quadTree == nullptr)
	//{
	//	return;
	//	m_quadTree = make_unique<QuadTree>(this);
	//	m_quadTree->init();
	//}

	if (m_firstProcess)
	{
		m_debugDraw = get_viewport()->get_debug_draw();

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

	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;
		itQuadTree->second->update(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord);
	}

	if (m_refreshRequired)
	{
		Chunk::RefreshChunkAction action(is_visible_in_tree());
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			if (!itQuadTree->second->isValid())
				continue;
			itQuadTree->second->ForAllChunk(action);
		}
		m_refreshRequired = false;
	}

	// Update chunks
	{
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			if (!itQuadTree->second->isValid())
				continue;
			
			// All chunk that need an update (they are chunk that got split or joined)
			std::vector<Chunk*> vectChunkUpdate = itQuadTree->second->getChunkUpdate();

			// Forcing update to all neighboring chunks to readjust the seams
			std::vector<Chunk*> vectAdditionalUpdateChunk;
			for (std::vector<Chunk*>::iterator itChunk = vectChunkUpdate.begin(); itChunk != vectChunkUpdate.end(); itChunk++)
			{
				if (!(*itChunk)->isActive())
					continue;

				Chunk::ChunkPos pos = (*itChunk)->getPos();

				// In case the chunk got split forcing update to left, right, bottom, top chunks
				Chunk* chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::XMinusChunk);		// x lessening
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//
					// - o
					//
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::XPlusChunk);			// x growing
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//
					//   o -
					//
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::ZMinusChunk);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//   -
					//   o
					//
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::ZPlusChunk);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//
					//   o
					//   -
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}

				// In case the chunk got joined
				int lod = pos.getLod();
				if (lod > 0 && (*itChunk)->gotJustJoined())
				{
					(*itChunk)->setJustJoined(false);

					Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
					//	o =
					//	= =
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::XMinusQuadSecondChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//  
						//	- o =
						//	  = =
						//  
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::XMinusQuadForthChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//  
						//	  o =
						//	- = =
						//  
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::XPlusQuadFirstChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//  
						//	  o = -
						//	  = =
						//  
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::XPlusQuadThirdChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//  
						//	  o = 
						//	  = = -
						//  
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::ZMinusQuadThirdChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//    -
						//	  o =
						//	  = =
						//    
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::ZMinusQuadForthChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//      -
						//	  o =
						//	  = =
						//    
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::ZPlusQuadFirstChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//    
						//	  o =
						//	  = =
						//    -
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::ZPlusQuadSecondChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//      
						//	  o =
						//	  = =
						//      -
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
				}
				// Anycase clear flag just joined
				if ((*itChunk)->gotJustJoined())
					(*itChunk)->setJustJoined(false);
			}

			for (std::vector<Chunk*>::iterator it = vectChunkUpdate.begin(); it != vectChunkUpdate.end(); it++)
				(*it)->update(is_visible_in_tree());

			for (std::vector<Chunk*>::iterator it = vectAdditionalUpdateChunk.begin(); it != vectAdditionalUpdateChunk.end(); it++)
				(*it)->update(is_visible_in_tree());

			itQuadTree->second->clearChunkUpdate();
			vectAdditionalUpdateChunk.clear();
		}
	}

	if (m_updateTerrainVisibilityRequired)
	{
		if (m_initialWordlViewerPosSet)
			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				if (!itQuadTree->second->isValid())
					continue;
				Chunk::DebugVisibilityChangedChunkAction action(m_debugVisibility);
				itQuadTree->second->ForAllChunk(action);
			}
		m_updateTerrainVisibilityRequired = false;
	}

	if (m_updateDebugModeRequired)
	{
		if (m_initialWordlViewerPosSet)
			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				if (!itQuadTree->second->isValid())
					continue;
				Chunk::SwitchDebugModeAction action(m_requiredChunkDebugMode);
				itQuadTree->second->ForAllChunk(action);
				m_currentChunkDebugMode = m_requiredChunkDebugMode;
			}
		m_updateDebugModeRequired = false;
	}

	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;
		itQuadTree->second->updateMaterialParams();
	}
	
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
}

void GDN_TheWorld_Viewer::_physics_process(float _delta)
{
	Input* input = Input::get_singleton();

	if (input->is_action_pressed("ui_ctrl"))
		m_ctrlPressed = true;
	else
		m_ctrlPressed = false;
}

Transform GDN_TheWorld_Viewer::internalTransformGlobalCoord(void)
{
	// Return the transformation of the viewer Node in global coordinates appling a scale factor (m_mapScaleVector)
	// Viewer Node origin is set to match the lower point of the grid in global coords (as expressed by Map manager) in resetInitialWordlViewerPos which load points from DB
	// ==> t.origin = Vector3(m_worldVertices[0].posX(), 0, m_worldVertices[0].posZ());
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

int GDN_TheWorld_Viewer::getNumChunks(void)
{
	int numChunks = 0;
	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;
		numChunks += itQuadTree->second->getNumChunks();
	}
	return numChunks; 
}

TheWorld_MapManager::MapManager::Quadrant* GDN_TheWorld_Viewer::loadWorldData(float& x, float& z, int level, int numWorldVerticesPerSize)
{
	try
	{
		return Globals()->mapManager()->getQuadrant(x, z, level, numWorldVerticesPerSize);

		//{	// SUPER DEBUGRIC
		//	Globals()->debugPrint("Inizio SUPER DEBUGRIC!");
		//	ResourceLoader* resLoader = ResourceLoader::get_singleton();
		//	Ref<Image> image = resLoader->load("res://height.res");
		//	int res = (int)image->get_width();
		//	assert(res == Globals()->heightmapResolution() + 1);
		//	float gridStepInWUs = Globals()->gridStepInWU();
		//	image->lock();
		//	for (int pz = 0; pz < res; pz++)
		//		for (int px = 0; px < res; px++)
		//		{
		//			Color c = image->get_pixel(px, pz);
		//			//m_worldVertices[px + pz * res].setAltitude(c.r);
		//			m_worldVertices[px + pz * res].setAltitude(c.r * 3);
		//		}
		//	image->unlock();
		//	Globals()->debugPrint("Fine SUPER DEBUGRIC!");
		//}	// SUPER DEBUGRIC
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

void GDN_TheWorld_Viewer::resetInitialWordlViewerPos(float x, float z, float cameraDistanceFromTerrain, int level, int chunkSizeShift, int heightmapResolutionShift)
{
	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System

	m_initialWordlViewerPosSet = false;
	
	m_worldViewerLevel = level;

	try
	{
		bool changed = Globals()->resize(chunkSizeShift, heightmapResolutionShift);
		if (changed)
		{
			if (m_meshCache)
				m_meshCache.reset();
			m_meshCache = make_unique<MeshCache>(this);
			m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());
		}

		m_numWorldVerticesPerSize = Globals()->heightmapResolution() + 1;
		
		TheWorld_MapManager::MapManager::Quadrant* quadrant = loadWorldData(x, z, level, m_numWorldVerticesPerSize);
		TheWorld_MapManager::MapManager::QuadrantId quadrantId = quadrant->getId();

		m_mapQuadTree.clear();
		m_mapQuadTree[quadrantId] = make_unique<QuadTree>(this);
		m_mapQuadTree[quadrantId]->init(quadrant);

		TheWorld_MapManager::MapManager::GridVertex viewerPos(x, 0, z, level);
		std::vector<TheWorld_MapManager::MapManager::GridVertex>::iterator it = std::find(m_mapQuadTree[quadrantId]->getQuadrant()->getGridVertices().begin(), m_mapQuadTree[quadrantId]->getQuadrant()->getGridVertices().end(), viewerPos);
		if (it == m_mapQuadTree[quadrantId]->getQuadrant()->getGridVertices().end())
		{
			Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "Not found WorldViewer Pos");
			return;
		}
		Vector3 cameraPos(x, it->altitude() + cameraDistanceFromTerrain, z);		// MapManager coordinates are local coordinates of WorldNode
		Vector3 lookAt(x + cameraDistanceFromTerrain, it->altitude(), z + cameraDistanceFromTerrain);

		// Viewer stuff: set viewer position relative to world node at the first point of the bitmap and altitude 0 so that that point is at position (0,0,0) respect to the viewer
		Transform t = get_transform();
		t.origin = Vector3(m_mapQuadTree[quadrantId]->getQuadrant()->getGridVertices()[0].posX(), 0, m_mapQuadTree[quadrantId]->getQuadrant()->getGridVertices()[0].posZ());
		set_transform(t);
		
		WorldCamera()->initCameraInWorld(cameraPos, lookAt);

		//m_mapQuadTree[quadrantId]->resetMaterialParams();

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

//void GDN_TheWorld_Viewer::getPartialAABB(AABB& aabb, int firstWorldVertCol, int lastWorldVertCol, int firstWorldVertRow, int lastWorldVertRow, int step)
//{
//	int idxFirstColFirstRowWorldVert = m_numWorldVerticesPerSize * firstWorldVertRow + firstWorldVertCol;
//	int idxLastColFirstRowWorldVert = m_numWorldVerticesPerSize * firstWorldVertRow + lastWorldVertCol;
//	int idxFirstColLastRowWorldVert = m_numWorldVerticesPerSize * lastWorldVertRow + firstWorldVertCol;
//	int idxLastColLastRowWorldVert = m_numWorldVerticesPerSize * lastWorldVertRow + lastWorldVertCol;
//
//	// altitudes
//	float minHeigth = 0, maxHeigth = 0;
//	bool firstTime = true;
//	for (int idxRow = 0; idxRow < lastWorldVertRow - firstWorldVertRow + 1; idxRow += step)
//	{
//		for (int idxVert = idxFirstColFirstRowWorldVert + idxRow * m_numWorldVerticesPerSize; idxVert < idxLastColFirstRowWorldVert + idxRow * m_numWorldVerticesPerSize + 1; idxVert += step)
//		{
//			if (firstTime)
//			{
//				minHeigth = maxHeigth = m_quadTree->getQuadrant()->getGridVertices()[idxVert].altitude();
//				firstTime = false;
//			}
//			else
//			{
//				minHeigth = Utils::min2(minHeigth, m_quadTree->getQuadrant()->getGridVertices()[idxVert].altitude());
//				maxHeigth = Utils::max2(maxHeigth, m_quadTree->getQuadrant()->getGridVertices()[idxVert].altitude());
//			}
//		}
//	}
//	
//	Vector3 startPosition(m_quadTree->getQuadrant()->getGridVertices()[idxFirstColFirstRowWorldVert].posX(), minHeigth, m_quadTree->getQuadrant()->getGridVertices()[idxFirstColFirstRowWorldVert].posZ());
//	//Vector3 endPosition(m_worldVertices[idxLastColLastRowWorldVert].posX() - m_worldVertices[idxFirstColFirstRowWorldVert].posX(), maxHeigth, m_worldVertices[idxLastColLastRowWorldVert].posZ() - m_worldVertices[idxFirstColFirstRowWorldVert].posZ());
//	Vector3 endPosition(m_quadTree->getQuadrant()->getGridVertices()[idxLastColLastRowWorldVert].posX(), maxHeigth, m_quadTree->getQuadrant()->getGridVertices()[idxLastColLastRowWorldVert].posZ());
//	Vector3 size = endPosition - startPosition;
//
//	aabb.set_position(startPosition);
//	aabb.set_size(size);
//}

void GDN_TheWorld_Viewer::onTransformChanged(void)
{
	Globals()->debugPrint("Transform changed");
	
	//The transformand other properties can be set by the scene loader, before we enter the tree
	if (!is_inside_tree())
		return;

	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;
		Transform gt = internalTransformGlobalCoord();

		Chunk::TransformChangedChunkAction action(gt);
		itQuadTree->second->ForAllChunk(action);

		itQuadTree->second->materialParamsNeedUpdate(true);

		// TODORIC Collider stuff
		//_if _collider != null:
		//	_collider.set_transform(gt)
	}
}

void GDN_TheWorld_Viewer::setMapScale(Vector3 mapScaleVector)
{
	if (m_mapScaleVector == mapScaleVector)
		return;

	mapScaleVector.x = Utils::max2(mapScaleVector.x, MIN_MAP_SCALE);
	mapScaleVector.y = Utils::max2(mapScaleVector.y, MIN_MAP_SCALE);
	mapScaleVector.z = Utils::max2(mapScaleVector.z, MIN_MAP_SCALE);

	m_mapScaleVector = mapScaleVector;

	onTransformChanged();
}

AABB GDN_TheWorld_Viewer::getCameraChunkLocalAABB(void)
{
	if (m_cameraChunk && !m_cameraChunk->isMeshNull())
		return m_cameraChunk->getAABB();
	else
		return AABB();
}

AABB GDN_TheWorld_Viewer::getCameraChunkLocalDebugAABB(void)
{
	if (m_cameraChunk && !m_cameraChunk->isDebugMeshNull())
		return m_cameraChunk->getDebugMeshAABB();
	else
		return AABB();
}

Transform GDN_TheWorld_Viewer::getCameraChunkMeshGlobalTransformApplied(void)
{
	if (m_cameraChunk && !m_cameraChunk->isMeshNull())
		return m_cameraChunk->getMeshGlobalTransformApplied();
	else
		return Transform();
}

Transform GDN_TheWorld_Viewer::getCameraChunkDebugMeshGlobalTransformApplied(void)
{
	if (m_cameraChunk && !m_cameraChunk->isDebugMeshNull())
		return m_cameraChunk->getDebugMeshGlobalTransformApplied();
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
	int numSplits = 0;
	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;
		numSplits += itQuadTree->second->getNumSplits();
	}
	return numSplits;
}

int GDN_TheWorld_Viewer::getNumJoins()
{
	int numJoins = 0;
	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;
		numJoins += itQuadTree->second->getNumJoins();
	}
	return numJoins;
}

void GDN_TheWorld_Viewer::dump()
{
	Globals()->debugPrint("*************");
	Globals()->debugPrint("STARTING DUMP");

	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;

		Globals()->debugPrint("DUMP QUADRANT " + String(itQuadTree->second->getQuadrant()->getId().getId().c_str()));

		float f = Engine::get_singleton()->get_frames_per_second();
		Globals()->debugPrint("FPS: " + String(std::to_string(f).c_str()));
		itQuadTree->second->dump();

		Node* node = get_node(NodePath("/root"));
		if (node != nullptr)
		{
			Globals()->debugPrint("============================================");
			Globals()->debugPrint("");
			Globals()->debugPrint("@@2 = res://native/GDN_TheWorld_Viewer.gdns");
			Globals()->debugPrint("");
			Globals()->debugPrint(node->get_name());
			Array nodes = node->get_children();
			dumpRecurseIntoChildrenNodes(nodes, 1);
			Globals()->debugPrint("============================================");
		}

		Globals()->debugPrint("DUMP COMPLETE QUADRANT " + String(itQuadTree->second->getQuadrant()->getId().getId().c_str()));
	}

	Globals()->debugPrint("DUMP COMPLETE");
	Globals()->debugPrint("*************");
}

void GDN_TheWorld_Viewer::dumpRecurseIntoChildrenNodes(Array nodes, int level)
{
	for (int i = 0; i < nodes.size(); i++)
	{
		std::string header(level, '\t');
		Node* node = nodes[i];
		Globals()->debugPrint(String(header.c_str()) + node->get_name());
		Array nodes = node->get_children();
		dumpRecurseIntoChildrenNodes(nodes, level + 1);
	}
}

void GDN_TheWorld_Viewer::setCameraChunk(Chunk* chunk, QuadTree* quadTree)
{
	if (m_cameraChunk != nullptr && m_cameraChunk != chunk)
	{
		m_cameraChunk->resetCameraVerticalOnChunk();
		quadTree->addChunkUpdate(m_cameraChunk);
	}

	m_cameraChunk = chunk;
}