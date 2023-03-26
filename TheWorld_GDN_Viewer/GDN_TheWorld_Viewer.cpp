//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Camera.h"
#include "GDN_TheWorld_Edit.h"
#include <SceneTree.hpp>
#include <Viewport.hpp>
#include <Engine.hpp>
#include <World.hpp>
#include <OS.hpp>
#include <Input.hpp>
#include <VisualServer.hpp>
#include <ResourceLoader.hpp>
#include <Shader.hpp>
#include <ImageTexture.hpp>
#include <File.hpp>
#include <PhysicsDirectSpaceState.hpp>
#include <filesystem>
#include <limits>

#include "MeshCache.h"
#include "QuadTree.h"
#include "Profiler.h"
#include "FastNoiseLite.h"

#include <algorithm>

using namespace godot;
using namespace std;

namespace fs = std::filesystem;

// TODO
// . passare allo shader (nuova texture ?):
//		- 4 bit per indicare se il vertice corrispondente è sul perimetro in uno dei 4 lati NON IMPORTA BASTA VALITARE LE COORDINATE UV e verificare se valgono 0 o 1 (o ci si approssimano)
//		- le 4 altezze nelle 4 direzioni adiacenti per: 
//			- calcolare le normali dinamicamente se la normal map presenta un vettore nullo
//			- eseguire lo streching verso l'esterno dei vertici sul perimetro in modo da sovrapporsi con i chunk adiacenti ed evitare l'effetto "Fireflies along seams" SE CI LIMITIAMO A QUESTO BASTA UNA SOLA DIREZIONE PER I VERTICI SUL BORDO (NEI 4 ANGOLI PREVALE UNO DEI DUE BORDI)
// . calcolare le normali tramite uno shader applicato ad uno sprite 2D per sfruttare le capacità di calcolo della GPU (vedi: normalmap_baker.gd e bump2normal_tex.shader)
// TODO

// World Node Local Coordinate System is the same as MapManager coordinate system
// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
// Chunk and QuadTree coordinates are in Viewer Node local coordinate System

void GDN_TheWorld_Viewer::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_Viewer::_ready);
	register_method("_process", &GDN_TheWorld_Viewer::_process);
	register_method("_physics_process", &GDN_TheWorld_Viewer::_physics_process);
	register_method("_input", &GDN_TheWorld_Viewer::_input);
	register_method("_notification", &GDN_TheWorld_Viewer::_notification);

	register_method("reset_initial_world_viewer_pos", &GDN_TheWorld_Viewer::resetInitialWordlViewerPos);
	register_method("initial_world_viewer_pos_set", &GDN_TheWorld_Viewer::initialWordlViewerPosSet);
	register_method("dump_required", &GDN_TheWorld_Viewer::setDumpRequired);
	//register_method("get_camera_chunk_global_transform_of_aabb", &GDN_TheWorld_Viewer::getCameraChunkGlobalTransformOfAABB);
	register_method("get_camera_chunk_local_aabb", &GDN_TheWorld_Viewer::getCameraChunkLocalAABB);
	register_method("get_camera_chunk_local_debug_aabb", &GDN_TheWorld_Viewer::getCameraChunkLocalDebugAABB);
	register_method("get_camera_chunk_global_transform_applied", &GDN_TheWorld_Viewer::getCameraChunkGlobalTransformApplied);
	register_method("get_camera_chunk_debug_global_transform_applied", &GDN_TheWorld_Viewer::getCameraChunkDebugGlobalTransformApplied);
	register_method("get_camera_chunk_id", &GDN_TheWorld_Viewer::getCameraChunkId);
	register_method("get_camera_quadrant_name", &GDN_TheWorld_Viewer::getCameraQuadrantName);
	register_method("get_num_splits", &GDN_TheWorld_Viewer::getNumSplits);
	register_method("get_num_joins", &GDN_TheWorld_Viewer::getNumJoins);
	register_method("get_num_chunks", &GDN_TheWorld_Viewer::getNumChunks);
	register_method("get_num_active_chunks", &GDN_TheWorld_Viewer::getNumActiveChunks);
	register_method("get_num_quadrant", &GDN_TheWorld_Viewer::getNumQuadrant);
	register_method("get_num_initialized_quadrant", &GDN_TheWorld_Viewer::getNuminitializedQuadrant);
	register_method("get_num_visible_quadrant", &GDN_TheWorld_Viewer::getNumVisibleQuadrant);
	register_method("get_num_initialized_visible_quadrant", &GDN_TheWorld_Viewer::getNuminitializedVisibleQuadrant);
	register_method("get_num_empty_quadrant", &GDN_TheWorld_Viewer::getNumEmptyQuadrant);
	register_method("get_num_flushed_quadrant", &GDN_TheWorld_Viewer::getNumFlushedQuadrant);
	register_method("get_num_process_not_owns_lock", &GDN_TheWorld_Viewer::getProcessNotOwnsLock);
	register_method("get_process_duration", &GDN_TheWorld_Viewer::getProcessDuration);
	register_method("get_refresh_quads_duration", &GDN_TheWorld_Viewer::getRefreshMapQuadDuration);
	register_method("get_update_quads1_duration", &GDN_TheWorld_Viewer::getUpdateQuads1Duration);
	register_method("get_update_quads2_duration", &GDN_TheWorld_Viewer::getUpdateQuads2Duration);
	register_method("get_update_quads3_duration", &GDN_TheWorld_Viewer::getUpdateQuads3Duration);
	register_method("get_update_chunks_duration", &GDN_TheWorld_Viewer::getUpdateChunksDuration);
	register_method("get_update_material_params_duration", &GDN_TheWorld_Viewer::getUpdateMaterialParamsDuration);
	register_method("get_mouse_track_hit_duration", &GDN_TheWorld_Viewer::getMouseTrackHitDuration);
	register_method("get_debug_draw_mode", &GDN_TheWorld_Viewer::getDebugDrawMode);
	register_method("get_chunk_debug_mode", &GDN_TheWorld_Viewer::getChunkDebugModeStr);
	register_method("get_mouse_hit", &GDN_TheWorld_Viewer::getMouseHit);
	register_method("get_mouse_quadrant_hit_name", &GDN_TheWorld_Viewer::_getMouseQuadrantHitName);
	register_method("get_mouse_quadrant_hit_tag", &GDN_TheWorld_Viewer::_getMouseQuadrantHitTag);
	register_method("get_mouse_quadrant_hit_pos", &GDN_TheWorld_Viewer::getMouseQuadrantHitPos);
	register_method("get_mouse_quadrant_hit_size", &GDN_TheWorld_Viewer::getMouseQuadrantHitSize);
	register_method("get_mouse_chunk_hit_name", &GDN_TheWorld_Viewer::getMouseChunkHitId);
	register_method("get_mouse_chunk_hit_pos", &GDN_TheWorld_Viewer::getMouseChunkHitPos);
	register_method("get_mouse_chunk_hit_size", &GDN_TheWorld_Viewer::getMouseChunkHitSize);
	register_method("get_mouse_chunk_hit_dist_from_cam", &GDN_TheWorld_Viewer::getMouseChunkHitDistFromCam);
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_initialized = false;
	m_desiderdLookDev = ShaderTerrainData::LookDev::NotSet;
	m_desideredLookDevChanged = false;
	m_useVisualServer = true;
	m_firstProcess = true;
	m_initialWordlViewerPosSet = false;
	m_dumpRequired = false;
	lastTrackedChunk = nullptr;
	m_refreshRequired = false;
	m_worldViewerLevel = 0;
	m_worldCamera = nullptr;
	m_cameraChunk = nullptr;
	//m_cameraQuadTree = nullptr;
	m_refreshMapQuadTree = false;
	m_globals = nullptr;
	m_editModeUIControl = nullptr;
	m_timeElapsedFromLastDump = 0;
	m_timeElapsedFromLastStatistic = 0;
	m_processDuration = 0;
	m_numProcessExecution = 0;
	m_averageProcessDuration = 0;
	m_numRefreshMapQuad = 0;
	m_RefreshMapQuadDuration = 0;
	m_averageRefreshMapQuadDuration = 0;
	m_numMouseTrackHit = 0;
	m_mouseTrackHitDuration = 0;
	m_averageMouseTrackHitDuration = 0;
	m_numUpdateQuads1 = 0;
	m_updateQuads1Duration = 0;
	m_averageUpdateQuads1Duration = 0;
	m_numUpdateQuads2 = 0;
	m_updateQuads2Duration = 0;
	m_averageUpdateQuads2Duration = 0;
	m_numUpdateQuads3 = 0;
	m_updateQuads3Duration = 0;
	m_averageUpdateQuads3Duration = 0;
	m_numUpdateChunks = 0;
	m_updateChunksDuration = 0;
	m_averageUpdateChunksDuration = 0;
	m_numUpdateMaterialParams = 0;
	m_updateMaterialParamsDuration = 0;
	m_averageUpdateMaterialParamsDuration = 0;
	m_numSplits = 0;
	m_numJoins = 0;
	m_numActiveChunks = 0;
	m_numChunks = 0;
	m_numProcessNotOwnsLock = 0;
	m_numQuadrant = 0;
	m_numinitializedQuadrant = 0;
	m_numVisibleQuadrant = 0;
	m_numEmptyQuadrant = 0;
	m_numFlushedQuadrant = 0;
	m_numinitializedVisibleQuadrant = 0;
	m_debugContentVisibility = true;
	m_trackMouse = false;
	m_editMode = false;
	m_editModeHudVisible = false;
	m_timeElapsedFromLastMouseTrack = 0;
	m_mouseQuadrantHitSize = 0;
	m_mouseHitChunk = nullptr;
	//m_mouseHitQuadTree = nullptr;
	m_updateTerrainVisibilityRequired = false;
	m_currentChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
	m_requiredChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
	m_updateDebugModeRequired = false;
	m_numWorldVerticesPerSize = 0;
	m_debugDraw = Viewport::DebugDraw::DEBUG_DRAW_DISABLED;
	m_altPressed = false;
	m_ctrlPressed = false;
	m_shiftPressed = false;
	m_streamerThreadRequiredExit = false;
	m_streamerThreadRunning = false;
	m_numVisibleQuadrantOnPerimeter = -1;
	m_numCacheQuadrantOnPerimeter = 0;
	m_recalcQuadrantsInViewNeeded = false;
}

GDN_TheWorld_Viewer::~GDN_TheWorld_Viewer()
{
	deinit();
}

bool GDN_TheWorld_Viewer::init(void)
{
	PLOGI << "TheWorld Viewer Initializing...";

	m_meshCache = make_unique<MeshCache>(this);
	m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());

	m_streamerThreadRequiredExit = false;
	m_streamerThreadRunning = true;
	m_streamerThread = std::thread(&GDN_TheWorld_Viewer::streamer, this);

	m_initialized = true;
	PLOGI << "TheWorld Viewer Initialized!";

	return true;
}

void GDN_TheWorld_Viewer::preDeinit(void)
{
	m_streamerThreadRequiredExit = true;
	//if (m_streamerThread.joinable())
	//	m_streamerThread.join();
}

bool GDN_TheWorld_Viewer::canDeinit(void)
{
	return !m_streamerThreadRunning;
}

void GDN_TheWorld_Viewer::deinit(void)
{
	if (m_initialized)
	{
		PLOGI << "TheWorld Viewer Deinitializing...";

		m_streamerThreadRequiredExit = true;
		if (m_streamerThread.joinable())
			m_streamerThread.join();

		//{
		//	godot::GDN_TheWorld_Globals::s_num = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed1 = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed2 = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed3 = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed4 = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed5 = 0;
		//}
		m_mapQuadTree.clear();
		//{
		//	size_t el1 = godot::GDN_TheWorld_Globals::s_elapsed1 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el2 = godot::GDN_TheWorld_Globals::s_elapsed2 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el3 = godot::GDN_TheWorld_Globals::s_elapsed3 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el4 = godot::GDN_TheWorld_Globals::s_elapsed4 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el5 = godot::GDN_TheWorld_Globals::s_elapsed5 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el = godot::GDN_TheWorld_Globals::s_elapsed1 / godot::GDN_TheWorld_Globals::s_num;
		//}
		m_meshCache.reset();

		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (editModeUIControl != nullptr)
		{
			editModeUIControl->deinit();
			editModeUIControl->queue_free();
		}

		m_initialized = false;
		PLOGI << "TheWorld Viewer Deinitialized!";
		
		Globals()->debugPrint("GDN_TheWorld_Viewer::deinit DONE!");
	}
}

void GDN_TheWorld_Viewer::replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply)
{
	std::string method = reply.getMethod();

	if (reply.error())
	{
		Globals()->setStatus(TheWorldStatus::error);
		Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::replyFromServer: error method ") + method + std::string(" rc=") + std::to_string(reply.getErrorCode()) + std::string(" ") + reply.getErrorMessage()).c_str());
		return;
	}

	try
	{
		if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES)
		{
			TheWorld_Utils::Profiler::addElapsedMs(std::string("WorldDeploy 1 ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES, reply.duration());
			
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2 ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES);

			//TheWorld_Viewer_Utils::TimerMs clock("GDN_TheWorld_Viewer::replyFromServer", "CLIENT SIDE", false, true);
			
			if (reply.getNumReplyParams() < 3)
			{
				std::string m = std::string("Reply MapManager::getVertices error (not enough params replied)");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}

			ClientServerVariant v = reply.getReplyParam(0);
			const auto _viewerPosX(std::get_if<float>(&v));
			if (_viewerPosX == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not return a float as first return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float viewerPosX = *_viewerPosX;

			v = reply.getReplyParam(1);
			const auto _viewerPosZ(std::get_if<float>(&v));
			if (_viewerPosZ == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not return a float as second return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float viewerPosZ = *_viewerPosZ;

			ClientServerVariant vbuffGridVertices = reply.getReplyParam(2);
			const auto _buffGridVerticesFromServer(std::get_if<std::string>(&vbuffGridVertices));
			if (_buffGridVerticesFromServer == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not return a string as third return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}

			v = reply.getInputParam(2);
			const auto _lowerXGridVertex(std::get_if<float>(&v));
			if (_lowerXGridVertex == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float lowerXGridVertex = *_lowerXGridVertex;

			v = reply.getInputParam(3);
			const auto _lowerZGridVertex(std::get_if<float>(&v));
			if (_lowerZGridVertex == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a float as forth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float lowerZGridVertex = *_lowerZGridVertex;

			v = reply.getInputParam(4);
			const auto _numVerticesPerSize(std::get_if<int>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have an int as fifth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(5);
			const auto _gridStepInWU(std::get_if<float>(&v));
			if (_gridStepInWU == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a float as sixth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepInWU;

			v = reply.getInputParam(6);
			const auto _level(std::get_if<int>(&v));
			if (_level == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have an int as seventh input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int level = *_level;

			v = reply.getInputParam(7);
			const auto _meshIdFromServer(std::get_if<std::string>(&v));
			if (_meshIdFromServer == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a string as eight input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string meshIdFromServer = *_meshIdFromServer;

			v = reply.getInputParam(8);
			const auto _setCamera(std::get_if<bool>(&v));
			if (_setCamera == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a bool as nineth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			bool setCamera = *_setCamera;

			v = reply.getInputParam(9);
			const auto _cameraDistanceFromTerrain(std::get_if<float>(&v));
			if (_cameraDistanceFromTerrain == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a float as tenth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraDistanceFromTerrain = *_cameraDistanceFromTerrain;

			QuadrantPos quadrantPos(lowerXGridVertex, lowerZGridVertex, level, numVerticesPerSize, gridStepInWU);

			m_mtxQuadTreeAndMainProcessing.lock();
			if (m_mapQuadTree.contains(quadrantPos) && !m_mapQuadTree[quadrantPos]->statusToErase())
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1 ") + __FUNCTION__, "Elab. quadrant");
				QuadTree* quadTree = m_mapQuadTree[quadrantPos].get();
				m_mtxQuadTreeAndMainProcessing.unlock();
				if (quadTree->statusGetTerrainDataInProgress() || quadTree->statusRefreshTerrainDataInProgress())
				{
					std::recursive_mutex& quadrantMutex = quadTree->getQuadrantMutex();
					std::lock_guard<std::recursive_mutex> lock(quadrantMutex);

					std::string meshIdFromBuffer;
					{
						// ATTENZIONE
						//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);	// SUPERDEBUGRIC : to remove when the mock for altitudes is removed from cache.refreshMeshCacheFromBuffer
						TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1 ") + __FUNCTION__,"quadTree->getQuadrant()->refreshGridVertices");
						quadTree->getQuadrant()->refreshGridVerticesFromServer(*_buffGridVerticesFromServer, meshIdFromServer, meshIdFromBuffer, true);
					}
					
					{
						// ATTENZIONE
						//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
						quadTree->materialParamsNeedReset(true);
						//quadTree->resetMaterialParams(true);
					}

					if (setCamera)
					{
						TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.2 ") + __FUNCTION__, "setCamera");

						forceRefreshMapQuadTree();

						quadTree->setVisible(true);

						size_t cameraInGridIndex = quadTree->getQuadrant()->getIndexFromHeighmap(viewerPosX, viewerPosZ, level);
						if (cameraInGridIndex == -1)
							throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Not found WorldViewer Pos").c_str()));

						float cameraHeight = quadTree->getQuadrant()->getAltitudeFromHeigthmap(cameraInGridIndex);

						Vector3 cameraPos(viewerPosX, cameraHeight + cameraDistanceFromTerrain, viewerPosZ);		// MapManager coordinates are local coordinates of WorldNode
						Vector3 lookAt(viewerPosX + cameraDistanceFromTerrain, cameraHeight, viewerPosZ + cameraDistanceFromTerrain);

						// Viewer stuff: set viewer position relative to world node at the first point of the bitmap and altitude 0 so that that point is at position (0,0,0) respect to the viewer
						//Transform t = get_transform();
						//t.origin = Vector3(quadTree->getQuadrant()->getGridVertices()[0].posX(), 0, quadTree->getQuadrant()->getGridVertices()[0].posZ());
						//set_transform(t);

						WorldCamera()->initCameraInWorld(cameraPos, lookAt);

						m_initialWordlViewerPosSet = true;
					}

					quadTree->setStatus(QuadrantStatus::initialized);
				}
			}
			else
				m_mtxQuadTreeAndMainProcessing.unlock();
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER)
		{
			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			if (editModeUIControl != nullptr)
				editModeUIControl->replyFromServer(reply);
		}
		else
		{
			Globals()->setStatus(TheWorldStatus::error);
			Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::replyFromServer: unknow method ") + method).c_str());
		}
	}
	catch (GDN_TheWorld_Exception ex)
	{
		Globals()->setStatus(TheWorldStatus::error);
		Globals()->errorPrint(String(ex.exceptionName()) + String(" caught - ") + ex.what());
	}
	catch (...)
	{
		Globals()->setStatus(TheWorldStatus::error);
		Globals()->errorPrint("GDN_TheWorld_Globals::replyFromServer: exception caught");
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
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	//if (globals->status() != TheWorldStatus::sessionInitialized)
	//	return;

	globals->debugPrint("GDN_TheWorld_Viewer::_ready");

	//get_tree()->get_root()->connect("size_changed", this, "viewport_resizing");
	
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

	//{
	//	// Test performance memcpy
	//	int size = 2049 * 2049 * 4;
	//	char* buffer = (char*)calloc(1, size);
	//	memset(buffer, (int)'A', size);

	//	for (int i = 0; i < 100; i++)
	//	{
	//		char* p = nullptr;
	//		{
	//			TheWorld_Utils::GuardProfiler profiler(std::string("TestReady 1 ") + std::to_string(size) + " " + __FUNCTION__, "calloc");
	//			p = (char*)calloc(1, size);
	//		}
	//		{
	//			TheWorld_Utils::GuardProfiler profiler(std::string("TestReady 2 ") + std::to_string(size) + " " + __FUNCTION__, "memcpy");
	//			memcpy(p, buffer, size);
	//		}
	//		{
	//			TheWorld_Utils::GuardProfiler profiler(std::string("TestReady 3 ") + std::to_string(size) + " " + __FUNCTION__, "free");
	//			::free(p);
	//		}
	//	}

	//	::free(buffer);
	//}
}

void GDN_TheWorld_Viewer::_input(const Ref<InputEvent> event)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	if (event->is_action_pressed("ui_select") && m_altPressed)
	{
		if (m_trackMouse)
		{
			m_mouseQuadrantHitName = "";

			QuadTree* quadTreeSel = nullptr;
			if (m_quadrantSelPos.empty())
			{
				if (!m_quadrantHitPos.empty())
				{
					m_quadrantSelPos = m_quadrantHitPos;

					{
						std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
						MapQuadTree::iterator it = m_mapQuadTree.find(m_quadrantSelPos);
						if (it != m_mapQuadTree.end() && !it->second->statusToErase())
						{
							quadTreeSel = it->second.get();
							it->second->setEditModeSel(true);
							it->second->getQuadrant()->setColorsUpdated(true);
							it->second->materialParamsNeedReset(true);
						}
					}
				}
			}
			else
			{
				{
					std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
					MapQuadTree::iterator it = m_mapQuadTree.find(m_quadrantSelPos);
					if (it != m_mapQuadTree.end() && !it->second->statusToErase())
					{
						it->second->setEditModeSel(false);
						it->second->getQuadrant()->setColorsUpdated(true);
						it->second->materialParamsNeedReset(true);
					}
				}

				m_quadrantSelPos.reset();
			}

			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			if (editModeUIControl == nullptr)
			{
				createEditModeUI();
				editModeUIControl = EditModeUIControl();
				editModeUIControl->set_visible(false);
				m_editModeHudVisible = false;

			}
			if (editModeUIControl != nullptr /* && m_editMode*/)
			{
				if (m_quadrantSelPos.empty())
				{
					editModeUIControl->setMouseQuadSelLabelText("");
					editModeUIControl->setMouseQuadSelPosLabelText("");
					editModeUIControl->setEmptyTerrainEditValues();
					editModeUIControl->setElapsed(0, false);
				}
				else
				{
					editModeUIControl->setMouseQuadSelLabelText(m_quadrantSelPos.getName() + " " + m_quadrantSelPos.getTag());
					editModeUIControl->setMouseQuadSelPosLabelText(std::string("X=") + std::to_string(m_quadrantSelPos.getLowerXGridVertex()) + " Z=" + std::to_string(m_quadrantSelPos.getLowerZGridVertex()) + " " + std::to_string(m_quadrantSelPos.getSizeInWU()));
					if (quadTreeSel != nullptr)
					{
						TheWorld_Utils::TerrainEdit* terrainEdit = quadTreeSel->getQuadrant()->getTerrainEdit();
						
						TheWorld_Utils::TerrainEdit* northSideTerrainEdit = nullptr;
						QuadrantPos pos = m_quadrantSelPos.getQuadrantPos(QuadrantPos::DirectionSlot::NorthZMinus);
						QuadTree* q = getQuadTree(pos);
						if (q != nullptr && !q->getQuadrant()->empty())
							northSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
						TheWorld_Utils::TerrainEdit* southSideTerrainEdit = nullptr;
						pos = m_quadrantSelPos.getQuadrantPos(QuadrantPos::DirectionSlot::SouthZPlus);
						q = getQuadTree(pos);
						if (q != nullptr && !q->getQuadrant()->empty())
							southSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
						TheWorld_Utils::TerrainEdit* westSideTerrainEdit = nullptr;
						pos = m_quadrantSelPos.getQuadrantPos(QuadrantPos::DirectionSlot::WestXMinus);
						q = getQuadTree(pos);
						if (q != nullptr && !q->getQuadrant()->empty())
							westSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
						TheWorld_Utils::TerrainEdit* eastSideTerrainEdit = nullptr;
						pos = m_quadrantSelPos.getQuadrantPos(QuadrantPos::DirectionSlot::EastXPlus);
						q = getQuadTree(pos);
						if (q != nullptr && !q->getQuadrant()->empty())
							eastSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
						terrainEdit->adjustValues(northSideTerrainEdit, southSideTerrainEdit, westSideTerrainEdit, eastSideTerrainEdit);

						editModeUIControl->setTerrainEditValues(*terrainEdit);
					}
				}
			}
		}
	}

	if (event->is_action_pressed("ui_toggle_track_mouse"))
	{
		if (!m_editMode)
			m_trackMouse = !m_trackMouse;
		if (m_trackMouse)
			m_mouseQuadrantHitName = "";
	}

	if (event->is_action_pressed("ui_toggle_edit_mode"))
	{
		m_editMode = !m_editMode;
		if (m_editMode)
			m_trackMouse = true;
		else
			m_trackMouse = false;
	}

	if (event->is_action_pressed("ui_toggle_debug_visibility"))
	{
		m_debugContentVisibility = !m_debugContentVisibility;
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
	Globals()->print("LEFT						==> A	MOUSE BUTTON LEFT");
	Globals()->print("RIGHT						==> D	MOUSE BUTTON LEFT");
	Globals()->print("FORWARD					==>	W	MOUSE BUTTON MID	MOUSE WHEEL UP (WITH ALT MOVE ON MAP, WITHOUT MOVE FORWARD IN THE DIRECTION OF CAMERA)");
	Globals()->print("BACKWARD					==>	S	MOUSE BUTTON MID	MOUSE WHEEL DOWN (WITH ALT MOVE ON MAP, WITHOUT MOVE BACKWARD IN THE DIRECTION OF CAMERA)");
	Globals()->print("UP						==>	Q");
	Globals()->print("DOWN						==>	E");
	Globals()->print("");
	Globals()->print("SHIFT						==>	LOW MOVEMENT");
	Globals()->print("CTRL						==> FAST MOVEMENT");
	Globals()->print("");
	Globals()->print("ROTATE CAMERA				==>	MOUSE BUTTON RIGHT");
	Globals()->print("SHIFT ORI CAMERA			==>	MOUSE BUTTON LEFT");
	Globals()->print("SHIFT VERT CAMERA			==>	MOUSE BUTTON MID");
	Globals()->print("");
	Globals()->print("TOGGLE DEBUG STATS		==>	CTRL-ALT-D");
	Globals()->print("TOGGLE EDIT				==>	ALT-E");
	Globals()->print("DUMP						==>	ALT-D");
	Globals()->print("");
	Globals()->print("TOGGLE DEBUG VISIBILITY	==>	F1");
	Globals()->print("ROTATE CHUNK DEBUG MODE	==>	F2");
	Globals()->print("ROTATE DRAWING MODE		==>	F3");
	Globals()->print("REFRESH VIEW				==>	F4");
	Globals()->print("");
	Globals()->print("EXIT						==>	ESC");
	Globals()->print("");
	Globals()->print("===========================================================================");
	Globals()->print("");
}

void GDN_TheWorld_Viewer::_notification(int p_what)
{

	//GDN_TheWorld_Globals* globals = Globals();
	//if (globals->status() != TheWorldStatus::sessionInitialized)
	//	return;

	switch (p_what)
	{
		case NOTIFICATION_PREDELETE:
		{
			GDN_TheWorld_Globals* globals = Globals();
			if (globals != nullptr)
				globals->debugPrint("GDN_TheWorld_Viewer::_notification - Destroy Viewer");
	
			deinit();
		}
		break;
		case NOTIFICATION_ENTER_WORLD:
		{
			GDN_TheWorld_Globals* globals = Globals();
			printKeyboardMapping();
			string s = "Use Visual Server: "; s += (m_useVisualServer ? "True" : "False");
			if (globals != nullptr)
			{
				globals->infoPrint(s.c_str());
				globals->debugPrint("Enter world");
			}
			//if (m_initialWordlViewerPosSet)
			//{
			std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				if (!itQuadTree->second->isValid())
					continue;

				if (!itQuadTree->second->isVisible())
					continue;

				Chunk::EnterWorldChunkAction action;
				itQuadTree->second->ForAllChunk(action);
				
				itQuadTree->second->getQuadrant()->getCollider()->enterWorld();
			}
			//}
		}
		break;
		case NOTIFICATION_EXIT_WORLD:
		{
			GDN_TheWorld_Globals* globals = Globals();
			if (globals != nullptr)
				globals->debugPrint("Exit world");
			//if (m_initialWordlViewerPosSet)
			//{
			std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				if (!itQuadTree->second->isValid())
					continue;

				if (!itQuadTree->second->isVisible())
					continue;

				Chunk::ExitWorldChunkAction action;
				itQuadTree->second->ForAllChunk(action);
				
				itQuadTree->second->getQuadrant()->getCollider()->exitWorld();
				//itQuadTree->second->getQuadrant()->getCollider()->onGlobalTransformChanged();
			}
			//}
		}
		break;
		case NOTIFICATION_VISIBILITY_CHANGED:
		{
			GDN_TheWorld_Globals* globals = Globals();
			if (globals != nullptr)
				globals->debugPrint("Visibility changed");
			//if (m_initialWordlViewerPosSet)
			//{
			std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				if (!itQuadTree->second->isValid())
					continue;

				if (!itQuadTree->second->isVisible())
					continue;

				Chunk::VisibilityChangedChunkAction action(is_visible_in_tree());
				itQuadTree->second->ForAllChunk(action);
			}
			//}
		}
		break;
		case NOTIFICATION_TRANSFORM_CHANGED:
		{
			onTransformChanged();
		}
		break;
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

void GDN_TheWorld_Viewer::recalcQuadrantsInView(void)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants");

	QuadrantPos cameraQuadrant = m_computedCameraQuadrantPos;
	
	vector<QuadrantPos> quadrantPosNeeded;
	quadrantPosNeeded.push_back(cameraQuadrant);

	//{
	//	// calculate minimum distance of camera from quad tree borders
	//	float minimunDistanceOfCameraFromBordersOfQuadrant = cameraPosGlobalCoord.x - cameraQuadrant.getLowerXGridVertex();
	//	float f = (cameraQuadrant.getLowerXGridVertex() + cameraQuadrant.getSizeInWU()) - cameraPosGlobalCoord.x;
	//	if (f < minimunDistanceOfCameraFromBordersOfQuadrant)
	//		minimunDistanceOfCameraFromBordersOfQuadrant = f;
	//	f = cameraPosGlobalCoord.z - cameraQuadrant.getLowerZGridVertex();
	//	if (f < minimunDistanceOfCameraFromBordersOfQuadrant)
	//		minimunDistanceOfCameraFromBordersOfQuadrant = f;
	//	f = (cameraQuadrant.getLowerZGridVertex() + cameraQuadrant.getSizeInWU()) - cameraPosGlobalCoord.z;
	//	if (f < minimunDistanceOfCameraFromBordersOfQuadrant)
	//		minimunDistanceOfCameraFromBordersOfQuadrant = f;

	//	// calculate num of quad needed surrounding camera-quad-tree according to desidered farHorizon
	//	m_numVisibleQuadrantOnPerimeter = 0;
	//	if (farHorizon > minimunDistanceOfCameraFromBordersOfQuadrant)
	//	{
	//		m_numVisibleQuadrantOnPerimeter = (size_t)floor((farHorizon - minimunDistanceOfCameraFromBordersOfQuadrant) / cameraQuadrant.getSizeInWU()) + 1;
	//		if (m_numVisibleQuadrantOnPerimeter > 3)
	//			m_numVisibleQuadrantOnPerimeter = 3;
	//	}
	//}
	{
		m_numVisibleQuadrantOnPerimeter = 3;
		m_numCacheQuadrantOnPerimeter = 4;

		//m_numVisibleQuadrantOnPerimeter = 0;	// SUPERDEBUGRIC only camera quadrant
		//m_numCacheQuadrantOnPerimeter = 0;		// SUPERDEBUGRIC only camera quadrant

		//m_numVisibleQuadrantOnPerimeter = 1;	// SUPERDEBUGRIC only camera quadrant & 1 surrounding
		//m_numCacheQuadrantOnPerimeter = 1;		// SUPERDEBUGRIC only camera quadrant & 1 surrounding
	}

	// add horizontal (X axis) quadrants on the left and on the right
	for (int i = 0; i < m_numCacheQuadrantOnPerimeter; i++)
	{
		QuadrantPos q = cameraQuadrant.getQuadrantPos(QuadrantPos::DirectionSlot::XPlus, 1 + i);
		q.setTag(cameraQuadrant.getTag() + " X+" + std::to_string(1 + i));
		quadrantPosNeeded.push_back(q);
		//break;	// SUPERDEBUGRIC only X+1 quadrant
		q = cameraQuadrant.getQuadrantPos(QuadrantPos::DirectionSlot::XMinus, 1 + i);
		q.setTag(cameraQuadrant.getTag() + " X-" + std::to_string(1 + i));
		quadrantPosNeeded.push_back(q);
	}

	// for each horizontal quadrant ...
	size_t size = quadrantPosNeeded.size();
	for (size_t idx = 0; idx < size; idx++)
	{
		// ... add vertical (Z axis) quadrants up and down
		for (size_t i = 0; i < m_numCacheQuadrantOnPerimeter; i++)
		{
			//break;	// SUPERDEBUGRIC only X+1 quadrant
			QuadrantPos q = quadrantPosNeeded[idx].getQuadrantPos(QuadrantPos::DirectionSlot::ZPlus, 1 + int(i));
			q.setTag(quadrantPosNeeded[idx].getTag() + " Z+" + std::to_string(1 + i));
			quadrantPosNeeded.push_back(q);
			q = quadrantPosNeeded[idx].getQuadrantPos(QuadrantPos::DirectionSlot::ZMinus, 1 + int(i));
			q.setTag(quadrantPosNeeded[idx].getTag() + " Z-" + std::to_string(1 + i));
			quadrantPosNeeded.push_back(q);
		}
	}

	vector<QuadrantPos> quadrantToDelete;

	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

		//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (map stuff)");
		// insert needed quadrants not present in map and refresh the ones which are already in it
		//vector<QuadrantPos> quadrantToAdd;
		std::timespec refreshTime;

		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.0 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (get time)");

			int ret = timespec_get(&refreshTime, TIME_UTC);
		}
		
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (create/refresh)");

			size = quadrantPosNeeded.size();
			for (int idx = 0; idx < size; idx++)
			{
				MapQuadTree::iterator it = m_mapQuadTree.find(quadrantPosNeeded[idx]);
				if (it == m_mapQuadTree.end() || it->second->statusToErase())
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.1.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (create new quads)");

					if (it != m_mapQuadTree.end() /* && it->second->statusToErase() // useless for the previous if */)
					{
						// extract element from map without destroying it than remembre to destory later (next _process)
						auto h = m_mapQuadTree.extract(it->first);
						QuadrantPos pos = h.key();
						m_toErase.push_back(pos);
						//m_mapQuadTree.erase(it->first);
					}

					m_mapQuadTree[quadrantPosNeeded[idx]] = make_unique<QuadTree>(this, quadrantPosNeeded[idx]);
					m_mapQuadTree[quadrantPosNeeded[idx]]->refreshTime(refreshTime);
					if (!m_streamingTime.counterStarted())
						m_streamingTime.tick();
					//return;	// ???
				}
				else
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.1.2 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (refresh old quads)");

					it->second->refreshTime(refreshTime);
					it->second->setTag(quadrantPosNeeded[idx].getTag());
				}
			}
		}

		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.2 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (second loop)");

			// look for the quadrant to delete (the ones not refreshed)
			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				if (itQuadTree->second->getRefreshTime().tv_nsec == refreshTime.tv_nsec && itQuadTree->second->getRefreshTime().tv_sec == refreshTime.tv_sec)
				{
					bool visibilityChanged = false;
					if (itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(cameraQuadrant) > m_numVisibleQuadrantOnPerimeter)
					{
						if (itQuadTree->second->isVisible())
						{
							visibilityChanged = true;
							itQuadTree->second->setVisible(false);
						}
					}
					else
					{
						if (!itQuadTree->second->isVisible())
						{
							visibilityChanged = true;
							itQuadTree->second->setVisible(true);
							itQuadTree->second->materialParamsNeedReset(true);
						}
					}

					if (visibilityChanged)
					{
						//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.3 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (ForAllChunk)");
						Chunk::VisibilityChangedChunkAction action(is_visible_in_tree());
						itQuadTree->second->ForAllChunk(action);
					}
				}
				else
				{
					// Quadrant too far from camera: need to be deleted
					if (itQuadTree->second->currentStatus() == QuadrantStatus::initialized || itQuadTree->second->currentStatus() == QuadrantStatus::uninitialized)
					{
						itQuadTree->second->setVisible(false);
						quadrantToDelete.push_back(itQuadTree->first);
					}
					//if (m_cameraQuadTree != nullptr && itQuadTree->first == m_cameraQuadTree->getQuadrant()->getPos())
					//{
					//	// if we delete the ones containing old camera chunk we invalidate it
					//	m_cameraChunk = nullptr;
					//	m_cameraQuadTree = nullptr;
					//}
				}
			}
		}
		
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.3 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (erase all quads)");

			for (int idx = 0; idx < quadrantToDelete.size(); idx++)
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.3.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (erase quad)");
				m_mapQuadTree[quadrantToDelete[idx]]->setStatus(QuadrantStatus::toErase);
				//m_mapQuadTree.erase(quadrantToDelete[idx]);
			}
		}
	}
	
	quadrantToDelete.clear();
}

void GDN_TheWorld_Viewer::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_TheWorld_Viewer::_process");

	TheWorld_Utils::GuardProfiler profiler(std::string("_process 1 ") + __FUNCTION__, "ALL");

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	if (!WorldCamera() || !m_initialWordlViewerPosSet)
		return;

	GDN_TheWorld_Camera* activeCamera = WorldCamera()->getActiveCamera();
	if (!activeCamera)
		return;

	//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
	std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing, std::try_to_lock);
	if (!lock.owns_lock())
	{
		m_numProcessNotOwnsLock++;
		return;
	}

	TheWorld_Viewer_Utils::TimerMcs clock;
	clock.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock, this] { clock.tock(); this->m_numProcessExecution++; this->m_processDuration += clock.duration().count(); });

	_process_impl(_delta, activeCamera);
}

void GDN_TheWorld_Viewer::_process_impl(float _delta, GDN_TheWorld_Camera* activeCamera)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.1 ") + __FUNCTION__, "Lock owned");

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

	if (m_editMode)
	{
		if (!m_editModeHudVisible)
		{
			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			if (editModeUIControl == nullptr)
			{
				createEditModeUI();
				editModeUIControl = EditModeUIControl();
			}
			editModeUIControl->set_visible(true);
			m_editModeHudVisible = true;
		}
	}
	else
		if (m_editModeHudVisible)
		{
			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			editModeUIControl->set_visible(false);
			m_editModeHudVisible = false;
		}
	
	// erasing out-of-view quadrants
	{
		for (auto& pos : m_toErase)
		{
			if (m_mapQuadTree.contains(pos))
			{
				size_t numErased = m_mapQuadTree.erase(pos);
			}
		}
		m_toErase.clear();
	}
	
	Vector3 cameraPosGlobalCoord = activeCamera->get_global_transform().get_origin();
	//Transform globalTransform = internalTransformGlobalCoord();
	//Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node (grid) local coordinates of the camera pos

	// ADJUST QUADTREEs NEEDED ACCORDING TO CAMERA POS - START
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.2 ") + __FUNCTION__, "Adjust Quadtrees");

		float farHorizon = cameraPosGlobalCoord.y * FAR_HORIZON_MULTIPLIER;
		if (globals->heightmapSizeInWUs() > farHorizon)
			farHorizon = globals->heightmapSizeInWUs();

		QuadrantPos cameraQuadrant;

		// Look for Camera QuadTree: put it as first element of quadrantPosNeeded
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			if (itQuadTree->second->statusToErase())
				continue;

			float quadrantSizeInWU = itQuadTree->second->getQuadrant()->getPos().getSizeInWU();
			if (cameraPosGlobalCoord.x >= itQuadTree->second->getQuadrant()->getPos().getLowerXGridVertex() && cameraPosGlobalCoord.x < (itQuadTree->second->getQuadrant()->getPos().getLowerXGridVertex() + quadrantSizeInWU)
				&& cameraPosGlobalCoord.z >= itQuadTree->second->getQuadrant()->getPos().getLowerZGridVertex() && cameraPosGlobalCoord.z < (itQuadTree->second->getQuadrant()->getPos().getLowerZGridVertex() + quadrantSizeInWU))
			{
				cameraQuadrant = itQuadTree->second->getQuadrant()->getPos();
				cameraQuadrant.setTag("Camera");
				break;
			}
		}

		if (cameraQuadrant.empty())	// boh not found camera-quad-tree (it seems impossible) ...
		{
			// ... btw we create new quad tree containing camera
			float x = cameraPosGlobalCoord.x, z = cameraPosGlobalCoord.z;
			float _gridStepInWU = globals->gridStepInWU();
			QuadrantPos q(x, z, m_worldViewerLevel, m_numWorldVerticesPerSize, _gridStepInWU);
			cameraQuadrant = q;
			cameraQuadrant.setTag("Camera");
		}

		//
		// now quadrantPosNeeded has 1 element and quadrantPosNeeded[0] is the camera 
		//

		// if forced or the camera has changed quadrant the cache is repopulated
		//m_computedCameraQuadrantPos = quadrantPosNeeded[0];	// DEBUG
		if (m_refreshMapQuadTree || m_computedCameraQuadrantPos != cameraQuadrant)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.2.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants");

			TheWorld_Viewer_Utils::TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Recalc in memory map of quadrants", false, false);
			clock1.tick();
			auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
				clock1.tock();
				this->m_numRefreshMapQuad++;
				this->m_RefreshMapQuadDuration += clock1.duration().count();
				});

			m_refreshMapQuadTree = false;
			m_computedCameraQuadrantPos = cameraQuadrant;

			m_recalcQuadrantsInViewNeeded = true;
			//recalcQuadrantsInView();
		}
	}
	// ADJUST QUADTREEs NEEDED ACCORDING TO CAMERA POS - END

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.3 ") + __FUNCTION__, "Init new quadrants");
		streamingQuadrantStuff();
	}

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.4 ") + __FUNCTION__, "Track mouse");

		godot::Node* collider = nullptr;
		if (m_trackMouse)
		{
			int64_t timeElapsed = OS::get_singleton()->get_ticks_msec();
			if (timeElapsed - m_timeElapsedFromLastMouseTrack > TIME_INTERVAL_BETWEEN_MOUSE_TRACK)
			{
				TheWorld_Viewer_Utils::TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Track Mouse Chunk", false, false);
				clock1.tick();
				auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
					clock1.tock();
				this->m_numMouseTrackHit++;
				this->m_mouseTrackHitDuration += clock1.duration().count();
					});

				godot::PhysicsDirectSpaceState* spaceState = get_world()->get_direct_space_state();
				godot::Vector2 mousePosInVieport = get_viewport()->get_mouse_position();
				godot::Vector3 rayOrigin = activeCamera->project_ray_origin(mousePosInVieport);
				godot::Vector3 rayEnd = rayOrigin + activeCamera->project_ray_normal(mousePosInVieport) * activeCamera->get_zfar() * 1.5;
				godot::Dictionary rayArray = spaceState->intersect_ray(rayOrigin, rayEnd);

				GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();

				if (rayArray.has("position"))
				{
					m_mouseHit = rayArray["position"];
					if (editModeUIControl && m_editMode)
						editModeUIControl->setMouseHitLabelText(std::string("X=") + std::to_string(m_mouseHit.x) + " Y=" + std::to_string(m_mouseHit.y) + " Z=" + std::to_string(m_mouseHit.z));
				}

				if (rayArray.has("collider"))
				{
					collider = rayArray["collider"];
					if (collider->has_meta("QuadrantName"))
					{
						godot::String s = collider->get_meta("QuadrantName", "");
						char* str = s.alloc_c_string();
						std::string mouseQuadrantHitName = str;
						godot::api->godot_free(str);
						if (mouseQuadrantHitName != m_mouseQuadrantHitName)
						{
							/*s = collider->get_meta("QuadrantTag", "");
							str = s.alloc_c_string();
							std::string mouseQuadrantHitTag = str;
							godot::api->godot_free(str);*/
							godot::Vector3 mouseQuadrantHitPos = collider->get_meta("QuadrantOrig", Vector3());
							float mouseQuadrantHitSize = collider->get_meta("QuadrantSize", 0.0);
							float gridStepInWu = collider->get_meta("QuadrantStep");
							int level = collider->get_meta("QuadrantLevel");
							int numVerticesPerSize = collider->get_meta("QuadrantNumVert");
							QuadrantPos quadrantHitPos(mouseQuadrantHitPos.x, mouseQuadrantHitPos.z, level, numVerticesPerSize, gridStepInWu);
							//quadrantHitPos.setTag(mouseQuadrantHitTag);

							// Get current Tag
							std::string mouseQuadrantHitTag;
							MapQuadTree::iterator it = m_mapQuadTree.find(quadrantHitPos);
							if (it != m_mapQuadTree.end() && !it->second->statusToErase())
								mouseQuadrantHitTag = it->second->getTag();

							if (editModeUIControl != nullptr && m_editMode)
							{

								editModeUIControl->setMouseQuadHitLabelText(mouseQuadrantHitName + " " + mouseQuadrantHitTag);
								editModeUIControl->setMouseQuadHitPosLabelText(std::string("X=") + std::to_string(mouseQuadrantHitPos.x) + " Z=" + std::to_string(mouseQuadrantHitPos.z) + " " + std::to_string(mouseQuadrantHitSize));

								//// Get TerrainEdit Data from hit quadrant
								//MapQuadTree::iterator it = m_mapQuadTree.find(quadrantHitPos);
								//if (it != m_mapQuadTree.end() && !it->second->statusToErase())
								//{
								//	TerrainEdit* terrainEdit = it->second->getQuadrant()->getTerrainEdit();
								//	editModeUIControl->setSeed(terrainEdit->noiseSeed);
								//	editModeUIControl->setFrequency(terrainEdit->frequency);
								//	editModeUIControl->setOctaves(terrainEdit->fractalOctaves);
								//	editModeUIControl->setLacunarity(terrainEdit->fractalLacunarity);
								//	editModeUIControl->setGain(terrainEdit->fractalGain);
								//	editModeUIControl->setWeightedStrength(terrainEdit->fractalWeightedStrength);
								//	editModeUIControl->setPingPongStrength(terrainEdit->fractalPingPongStrength);
								//	editModeUIControl->setAmplitude(terrainEdit->amplitude);
								//	editModeUIControl->setMinHeight(terrainEdit->minHeight);
								//	editModeUIControl->setMaxHeight(terrainEdit->maxHeight);
								//	//editModeUIControl->setElapsed(0);
								//}
							}

							m_mouseQuadrantHitName = mouseQuadrantHitName;
							m_mouseQuadrantHitTag = mouseQuadrantHitTag;
							m_mouseQuadrantHitPos = mouseQuadrantHitPos;
							m_mouseQuadrantHitSize = mouseQuadrantHitSize;
							m_quadrantHitPos = quadrantHitPos;
						}
					}
					//godot::PoolStringArray metas = collider->get_meta_list();
					//for (int i = 0; i < metas.size(); i++)
					//{
					//	godot::String s = metas[i];
					//	char* str = s.alloc_c_string();
					//	std::string ss = str;
					//	godot::api->godot_free(str);
					//	godot::Variant v = collider->get_meta(s);
					//}
					//godot::String s = collider->get_meta("QuadrantName");
				}
				m_timeElapsedFromLastMouseTrack = timeElapsed;
			}
		}
	}
	
	// UPDATE QUADS - Stage 1
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.5 ") + __FUNCTION__, "Update Quads Stage 1");

		TheWorld_Viewer_Utils::TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Quads Stage 1", false, false);
		clock1.tick();
		auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
			clock1.tock();
			this->m_numUpdateQuads1++;
			this->m_updateQuads1Duration += clock1.duration().count();
			});

		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			int numSplitRequired = 0;
			itQuadTree->second->update(cameraPosGlobalCoord, QuadTree::UpdateStage::Stage1, numSplitRequired);
		}
	}
	
	while (true)
	{
		int numSplitRequired = 0;

		// UPDATE QUADS - Stage 2
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.6 ") + __FUNCTION__, "Update Quads Stage 2");

			TheWorld_Viewer_Utils::TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Quads Stage 2", false, false);
			clock1.tick();
			auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
				clock1.tock();
			this->m_numUpdateQuads2++;
			this->m_updateQuads2Duration += clock1.duration().count();
				});


			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				itQuadTree->second->update(Vector3(), QuadTree::UpdateStage::Stage2, numSplitRequired);
			}
		}

		if (numSplitRequired == 0)
			break;
		else
		{
			// UPDATE QUADS - Stage 3
			TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.7 ") + __FUNCTION__, "Update Quads Stage 3");

			TheWorld_Viewer_Utils::TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Quads Stage 3", false, false);
			clock1.tick();
			auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
				clock1.tock();
			this->m_numUpdateQuads3++;
			this->m_updateQuads3Duration += clock1.duration().count();
				});

			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				int numSplitRequired = 0;
				itQuadTree->second->update(cameraPosGlobalCoord, QuadTree::UpdateStage::Stage3, numSplitRequired);
			}
		}
	}

	if (m_refreshRequired)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.8 ") + __FUNCTION__, "Refresh required");

		Chunk::RefreshChunkAction action(is_visible_in_tree());
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			itQuadTree->second->ForAllChunk(action);
		}
		m_refreshRequired = false;
	}

	// UPDATE CHUNKS
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.9 ") + __FUNCTION__, "Update Chunks");

		TheWorld_Viewer_Utils::TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Chunks", false, false);
		clock1.tick();
		auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
			clock1.tock();
			this->m_numUpdateChunks++;
			this->m_updateChunksDuration += clock1.duration().count();
			});

		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			if (!itQuadTree->second->isValid())
				continue;
			
			if (!itQuadTree->second->isVisible())
				continue;

			// All chunk that need an update (those are chunk that got split or joined)
			std::vector<Chunk*> vectChunkUpdate = itQuadTree->second->getChunkUpdate();

			// Forcing update to all neighboring chunks to readjust the seams (by populating vectAdditionalUpdateChunk)
			// chunks affected are neighboring ones with same lod or less (more definded) ==> neighboring chunks with greater lod (less defined) are not selcted
			// as the seams are calculated on chunks with lessere lof (more defined chunks) if the neighborings have different lod
			std::vector<Chunk*> vectAdditionalUpdateChunk;
			for (std::vector<Chunk*>::iterator itChunk = vectChunkUpdate.begin(); itChunk != vectChunkUpdate.end(); itChunk++)
			{
				if (!(*itChunk)->isActive())
					continue;

				Chunk::ChunkPos pos = (*itChunk)->getPos();
				int lod = pos.getLod();
				int numChunksPerSideAtLowerLod = 0;
				Chunk::ChunkPos posFirstInternalChunkAtLowerLod(0, 0, 0);
				if (lod > 0)
				{
					posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
					numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);
				}

				{
					// In case the chunk got split/joined forcing update to left, right, bottom, top chunks with same lod
					Chunk* chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::XMinusChunk);	// x lessening
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
					chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::ZMinusChunk);			// z lessening
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//   -
						//   o
						//
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::ZPlusChunk);			// z growing
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//
						//   o
						//   -
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
				}

				// In case the chunk got joined: we have to look for adjacent chunks with lower lod (more defined)
				if (lod > 0 && (*itChunk)->gotJustJoined())
				{
					//(*itChunk)->setJustJoined(false);

					//	o =
					//	= =
					//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

					//	o =
					//	= =
					Chunk* chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XMinusQuadSecondChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//  
						//	- o =
						//	  = =
						//  
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					//	o =
					//	= =
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XMinusQuadForthChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//  
						//	  o =
						//	- = =
						//  
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					//	o =
					//	= =
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XPlusQuadFirstChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//  
						//	  o = -
						//	  = =
						//  
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					//	o =
					//	= =
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XPlusQuadThirdChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//  
						//	  o = 
						//	  = = -
						//  
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					//	o =
					//	= =
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZMinusQuadThirdChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//      
						//    -
						//	  o =
						//	  = =
						//    
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					//	o =
					//	= =
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZMinusQuadForthChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//      
						//      -
						//	  o =
						//	  = =
						//    
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					//	o =
					//	= =
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZPlusQuadFirstChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//    
						//	  o =
						//	  = =
						//    -
						//      
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
					//	o =
					//	= =
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZPlusQuadSecondChunk);
					if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
					{
						//      
						//	  o =
						//	  = =
						//      -
						//      
						chunk->setPendingUpdate(true);
						vectAdditionalUpdateChunk.push_back(chunk);
					}
				}

				// if the current chunk is on a border of the quadrant we need to look for adjacent chunks (with same lod or lesser if the chunk was just joined) in Adjacent quadrant
				QuadrantPos XMinusQuadrantPos, XPlusQuadrantPos, ZMinusQuadrantPos, ZPlusQuadrantPos;
				if (itQuadTree->second->isChunkOnBorderOfQuadrant(pos, XMinusQuadrantPos, XPlusQuadrantPos, ZMinusQuadrantPos, ZPlusQuadrantPos))
				{
					int numChunksPerSide = Globals()->numChunksPerHeightmapSide(pos.getLod());

					// if the chunks is adjacent to another quadrant
					if (XMinusQuadrantPos.isInitialized())
					{
						// take the adjacent quadrant if it exists
						auto iter = m_mapQuadTree.find(XMinusQuadrantPos);
						if (iter != m_mapQuadTree.end() && !iter->second->statusToErase())
						{
							QuadTree* quadTreeXMinus = iter->second.get();
							if (quadTreeXMinus->isValid())
							{
								Chunk::ChunkPos posSameLodOnAdjacentQuadrant(numChunksPerSide - 1, pos.getSlotPosZ(), pos.getLod());
								Chunk* chunkSameLodOnAdjacentQuadrant = quadTreeXMinus->getChunkAt(posSameLodOnAdjacentQuadrant);
								if (chunkSameLodOnAdjacentQuadrant != nullptr && chunkSameLodOnAdjacentQuadrant->isActive() && !chunkSameLodOnAdjacentQuadrant->isPendingUpdate())
								{
									//
									// - | o
									//
									chunkSameLodOnAdjacentQuadrant->setPendingUpdate(true);
									vectAdditionalUpdateChunk.push_back(chunkSameLodOnAdjacentQuadrant);
								}

								if (lod > 0 && ((*itChunk)->gotJustJoined() || lod == Globals()->lodMaxDepth()))
								{
									//	o =
									//	= =
									//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

									//int numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);

									{
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(numChunksPerSideAtLowerLod - 1, posFirstInternalChunkAtLowerLod.getSlotPosZ(), posFirstInternalChunkAtLowerLod.getLod());
										Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeXMinus->getChunkAt(posLesserLodOnAdjacentQuadrant);
										if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
										{
											//  
											//	- | o =
											//	  | = =
											//  
											chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
											vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
										}
									}
									{
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(numChunksPerSideAtLowerLod - 1, posFirstInternalChunkAtLowerLod.getSlotPosZ() + 1, posFirstInternalChunkAtLowerLod.getLod());
										Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeXMinus->getChunkAt(posLesserLodOnAdjacentQuadrant);
										if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
										{
											//  
											//	  | o =
											//	- | = =
											//  
											chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
											vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
										}
									}
								}
							}
						}
					}

					if (XPlusQuadrantPos.isInitialized())
					{
						// take the adjacent quadrant if it exists
						auto iter = m_mapQuadTree.find(XPlusQuadrantPos);
						if (iter != m_mapQuadTree.end() && !iter->second->statusToErase())
						{
							QuadTree* quadTreeXPlus = iter->second.get();
							if (quadTreeXPlus->isValid())
							{
								Chunk::ChunkPos posSameLodOnAdjacentQuadrant(0, pos.getSlotPosZ(), pos.getLod());
								Chunk* chunkSameLodOnAdjacentQuadrant = quadTreeXPlus->getChunkAt(posSameLodOnAdjacentQuadrant);
								if (chunkSameLodOnAdjacentQuadrant != nullptr && chunkSameLodOnAdjacentQuadrant->isActive() && !chunkSameLodOnAdjacentQuadrant->isPendingUpdate())
								{
									//
									//   o | -
									//
									chunkSameLodOnAdjacentQuadrant->setPendingUpdate(true);
									vectAdditionalUpdateChunk.push_back(chunkSameLodOnAdjacentQuadrant);
								}

								if (lod > 0 && (*itChunk)->gotJustJoined())
								{
									//	o =
									//	= =
									//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

									//int numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);

									{
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(0, posFirstInternalChunkAtLowerLod.getSlotPosZ(), posFirstInternalChunkAtLowerLod.getLod());
										Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeXPlus->getChunkAt(posLesserLodOnAdjacentQuadrant);
										if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
										{
											//  
											//	o = | -
											//	= = |
											//  
											chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
											vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
										}
									}
									{
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(0, posFirstInternalChunkAtLowerLod.getSlotPosZ() + 1, posFirstInternalChunkAtLowerLod.getLod());
										Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeXPlus->getChunkAt(posLesserLodOnAdjacentQuadrant);
										if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
										{
											//  
											//	o = |
											//	= = | -
											//  
											chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
											vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
										}
									}
								}
							}
						}
					}

					if (ZMinusQuadrantPos.isInitialized())
					{
						// take the adjacent quadrant if it exists
						auto iter = m_mapQuadTree.find(ZMinusQuadrantPos);
						if (iter != m_mapQuadTree.end() && !iter->second->statusToErase())
						{
							QuadTree* quadTreeZMinus = iter->second.get();
							if (quadTreeZMinus->isValid())
							{
								Chunk::ChunkPos posSameLodOnAdjacentQuadrant(pos.getSlotPosX(), numChunksPerSide - 1, pos.getLod());
								Chunk* chunkSameLodOnAdjacentQuadrant = quadTreeZMinus->getChunkAt(posSameLodOnAdjacentQuadrant);
								if (chunkSameLodOnAdjacentQuadrant != nullptr && chunkSameLodOnAdjacentQuadrant->isActive() && !chunkSameLodOnAdjacentQuadrant->isPendingUpdate())
								{
									//   -
									//   =
									//   o
									//
									chunkSameLodOnAdjacentQuadrant->setPendingUpdate(true);
									vectAdditionalUpdateChunk.push_back(chunkSameLodOnAdjacentQuadrant);
								}

								if (lod > 0 && (*itChunk)->gotJustJoined())
								{
									//	o =
									//	= =
									//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

									//int numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);

									{
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunkAtLowerLod.getSlotPosX(), numChunksPerSideAtLowerLod - 1, posFirstInternalChunkAtLowerLod.getLod());
										Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeZMinus->getChunkAt(posLesserLodOnAdjacentQuadrant);
										if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
										{
											//  
											//  -
											//  = =
											//	o =
											//	= =
											//  
											chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
											vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
										}
									}
									{
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunkAtLowerLod.getSlotPosX() + 1, numChunksPerSideAtLowerLod - 1, posFirstInternalChunkAtLowerLod.getLod());
										Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeZMinus->getChunkAt(posLesserLodOnAdjacentQuadrant);
										if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
										{
											//  
											//    -
											//  = =
											//	o =
											//	= =
											//  
											chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
											vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
										}
									}
								}
							}
						}
					}

					if (ZPlusQuadrantPos.isInitialized())
					{
						// take the adjacent quadrant if it exists
						auto iter = m_mapQuadTree.find(ZPlusQuadrantPos);
						if (iter != m_mapQuadTree.end() && !iter->second->statusToErase())
						{
							QuadTree* quadTreeZPlus = iter->second.get();
							if (quadTreeZPlus->isValid())
							{
								//
								//   o
								//   =
								//   -
								Chunk::ChunkPos posSameLodOnAdjacentQuadrant(pos.getSlotPosX(), 0, pos.getLod());
								Chunk* chunkSameLodOnAdjacentQuadrant = quadTreeZPlus->getChunkAt(posSameLodOnAdjacentQuadrant);
								if (chunkSameLodOnAdjacentQuadrant != nullptr && chunkSameLodOnAdjacentQuadrant->isActive() && !chunkSameLodOnAdjacentQuadrant->isPendingUpdate())
								{
									chunkSameLodOnAdjacentQuadrant->setPendingUpdate(true);
									vectAdditionalUpdateChunk.push_back(chunkSameLodOnAdjacentQuadrant);
								}

								if (lod > 0 && (*itChunk)->gotJustJoined())
								{
									//	o =
									//	= =
									//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

									//int numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);

									{
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunkAtLowerLod.getSlotPosX(), 0, posFirstInternalChunkAtLowerLod.getLod());
										Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeZPlus->getChunkAt(posLesserLodOnAdjacentQuadrant);
										if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
										{
											//  
											//	o =
											//	= =
											//  = =
											//  -
											//  
											chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
											vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
										}
									}
									{
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunkAtLowerLod.getSlotPosX() + 1, 0, posFirstInternalChunkAtLowerLod.getLod());
										Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeZPlus->getChunkAt(posLesserLodOnAdjacentQuadrant);
										if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
										{
											//  
											//	o =
											//	= =
											//  = =
											//    -
											//  
											chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
											vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
										}
									}
								}
							}
						}
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

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.10 ") + __FUNCTION__, "Other stuff");

		if (m_updateTerrainVisibilityRequired)
		{
			if (m_initialWordlViewerPosSet)
			{
				for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
				{
					if (!itQuadTree->second->isValid())
						continue;
					//if (!itQuadTree->second->isVisible())
					//	continue;
					Chunk::DebugVisibilityChangedChunkAction action(m_debugContentVisibility);
					itQuadTree->second->ForAllChunk(action);
				}
			}
			m_updateTerrainVisibilityRequired = false;
		}

		if (m_updateDebugModeRequired)
		{
			if (m_initialWordlViewerPosSet)
			{
				for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
				{
					Chunk::SwitchDebugModeAction action(m_requiredChunkDebugMode);
					itQuadTree->second->ForAllChunk(action);
				}
				m_currentChunkDebugMode = m_requiredChunkDebugMode;
			}
			m_updateDebugModeRequired = false;
		}
	}

	{
		if (m_desideredLookDevChanged)
		{
			enum class ShaderTerrainData::LookDev desideredLookDev = getDesideredLookDev();

			for (auto& it : m_mapQuadTree)
			{
				if (desideredLookDev == ShaderTerrainData::LookDev::NotSet)
					it.second->setRegularMaterial();
				else
					it.second->setLookDevMaterial(desideredLookDev);
			}
		}
	}
	
	m_desideredLookDevChanged = false;

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.11 ") + __FUNCTION__, "Material params stuff");

		TheWorld_Viewer_Utils::TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Material Params", false, false);
		clock1.tick();
		auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
			clock1.tock();
		this->m_numUpdateMaterialParams++;
		this->m_updateMaterialParamsDuration += clock1.duration().count();
			});

		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			bool reset = false;
			reset = itQuadTree->second->resetMaterialParams();
			bool updated = itQuadTree->second->updateMaterialParams();
				
			if ((reset || updated) && !m_desideredLookDevChanged)
				break;
		}
	}

	m_desideredLookDevChanged = false;

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.12 ") + __FUNCTION__, "Dump stuff");

		// Check for Dump
		if (TIME_INTERVAL_BETWEEN_DUMP != 0 && globals->isDebugEnabled())
		{
			int64_t timeElapsed = OS::get_singleton()->get_ticks_msec();
			if (timeElapsed - m_timeElapsedFromLastDump > TIME_INTERVAL_BETWEEN_DUMP * 1000)
			{
				m_dumpRequired = true;
				m_timeElapsedFromLastDump = timeElapsed;
			}
		}
		//if (m_dumpRequired)
		//{
		//	TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.12.1 ") + __FUNCTION__, "Dump stuff");

		//	m_dumpRequired = false;
		//	dump();
		//}
	}

	// Check for Statistics
	if (TIME_INTERVAL_BETWEEN_STATISTICS != 0)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.13 ") + __FUNCTION__, "Refresh statistics");

		int64_t timeElapsed = OS::get_singleton()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastStatistic > TIME_INTERVAL_BETWEEN_STATISTICS)
		{
			//m_numProcessNotOwnsLock = 0;
			
			refreshQuadTreeStatistics();
			
			if (m_numProcessExecution > 0)
			{
				m_averageProcessDuration = int(m_processDuration / m_numProcessExecution);
				//globals->debugPrint(String("m_numProcessExecution=") + std::to_string(m_numProcessExecution).c_str() + ", m_processDuration=" + std::to_string(m_processDuration).c_str() + ", m_averageProcessDuration=" + std::to_string(m_averageProcessDuration).c_str());
				m_processDuration = 0;
				m_numProcessExecution = 0;
			}
			else
				m_averageProcessDuration = 0;

			if (m_numRefreshMapQuad > 10)
			{
				m_averageRefreshMapQuadDuration = int(m_RefreshMapQuadDuration / m_numRefreshMapQuad);
				m_RefreshMapQuadDuration = 0;
				m_numRefreshMapQuad = 0;
			}

			if (m_numMouseTrackHit > 10)
			{
				m_averageMouseTrackHitDuration = int(m_mouseTrackHitDuration / m_numMouseTrackHit);
				m_mouseTrackHitDuration = 0;
				m_numMouseTrackHit = 0;
			}

			if (m_numUpdateQuads1 > 0)
			{
				m_averageUpdateQuads1Duration = int(m_updateQuads1Duration / m_numUpdateQuads1);
				m_updateQuads1Duration = 0;
				m_numUpdateQuads1 = 0;
			}
			else
				m_averageUpdateQuads1Duration = 0;

			if (m_numUpdateQuads2 > 0)
			{
				m_averageUpdateQuads2Duration = int(m_updateQuads2Duration / m_numUpdateQuads2);
				m_updateQuads2Duration = 0;
				m_numUpdateQuads2 = 0;
			}
			else
				m_averageUpdateQuads2Duration = 0;

			if (m_numUpdateQuads3 > 0)
			{
				m_averageUpdateQuads3Duration = int(m_updateQuads3Duration / m_numUpdateQuads3);
				m_updateQuads3Duration = 0;
				m_numUpdateQuads3 = 0;
			}
			else
				m_averageUpdateQuads3Duration = 0;

			if (m_numUpdateChunks > 0)
			{
				m_averageUpdateChunksDuration = int(m_updateChunksDuration / m_numUpdateChunks);
				m_updateChunksDuration = 0;
				m_numUpdateChunks = 0;
			}
			else
				m_averageUpdateChunksDuration = 0;

			if (m_numUpdateMaterialParams > 0)
			{
				m_averageUpdateMaterialParamsDuration = int(m_updateMaterialParamsDuration / m_numUpdateMaterialParams);
				m_updateMaterialParamsDuration = 0;
				m_numUpdateMaterialParams = 0;
			}
			else
				m_averageUpdateMaterialParamsDuration = 0;

			m_timeElapsedFromLastStatistic = timeElapsed;
		}
	}
}

void GDN_TheWorld_Viewer::_physics_process(float _delta)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	Input* input = Input::get_singleton();

	if (input->is_action_pressed("ui_shift"))
		m_shiftPressed = true;
	else
		m_shiftPressed = false;

	if (input->is_action_pressed("ui_ctrl"))
		m_ctrlPressed = true;
	else
		m_ctrlPressed = false;

	if (input->is_action_pressed("ui_alt"))
		m_altPressed = true;
	else
		m_altPressed = false;
}

void GDN_TheWorld_Viewer::getAllQuadrantPos(std::vector<QuadrantPos>& allQuandrantPos)
{
	allQuandrantPos.clear();

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	size_t size = m_mapQuadTree.size();
	allQuandrantPos.reserve(size);
	for (auto const& item : m_mapQuadTree)
		allQuandrantPos.push_back(item.first);
}

QuadTree* GDN_TheWorld_Viewer::getQuadTree(QuadrantPos pos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	auto iter = m_mapQuadTree.find(pos);
	if (iter == m_mapQuadTree.end() || iter->second->statusToErase())
		return nullptr;

	return iter->second.get();
}

Chunk* GDN_TheWorld_Viewer::getActiveChunkAt(QuadrantPos pos, Chunk::ChunkPos chunkPos, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter)
{
	QuadTree* quadTree = getQuadTree(pos);
	if (quadTree == nullptr)
		return nullptr;

	Chunk* chunk = quadTree->getChunkAt(chunkPos);
	if (chunk == nullptr)
		return nullptr;

	return getActiveChunkAt(chunk, dir, filter);
}

Chunk* GDN_TheWorld_Viewer::getActiveChunkAt(Chunk* chunk, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter)
{
	// A T T E N Z I O N E : DA TESTARE
	
	if (dir == Chunk::DirectionSlot::Center)
		return chunk;

	enum PosInQuad posInQuad = chunk->getPosInQuad();
	if (posInQuad == PosInQuad::NotSet)
		return nullptr;

	Chunk::ChunkPos pos = chunk->getPos();
	QuadTree* quadTree = chunk->getQuadTree();
		
	Chunk* retChunk = nullptr;

	//int idx = 0;
	switch (dir)
	{
	case Chunk::DirectionSlot::XMinusChunk:
	{
		if (posInQuad == PosInQuad::Second || posInQuad == PosInQuad::Forth)
		{
			if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::XMinusChunk);
			if (retChunk != nullptr && !retChunk->isActive())
				retChunk = nullptr;
		}
		else
		{
			int lod = pos.getLod();
			int maxLod = Globals()->lodMaxDepth();
			int numChunksPerHeightmapSide = Globals()->numChunksPerHeightmapSide(lod);

			if (chunk->getSlotPosX() == 0)
			{
				QuadTree* quadTreeXMinus = getQuadTree(quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XMinus));
				if (quadTreeXMinus != nullptr && quadTreeXMinus->isValid() && quadTreeXMinus->isVisible())
				{
					Chunk::ChunkPos posLastChunkAtSameLod(numChunksPerHeightmapSide - 1, pos.getSlotPosZ(), lod);
					if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
					{
						retChunk = quadTreeXMinus->getChunkAt(posLastChunkAtSameLod);
						if (retChunk != nullptr && !retChunk->isActive())
							retChunk = nullptr;
					}
					if (retChunk == nullptr)
					{
						Chunk::ChunkPos posSecondInternalChunkAtLowerLod = Chunk::ChunkPos(posLastChunkAtSameLod.getSlotPosX() * 2 + 1, posLastChunkAtSameLod.getSlotPosZ() * 2, lod - 1);
						Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(posLastChunkAtSameLod.getSlotPosX() / 2, posLastChunkAtSameLod.getSlotPosZ() / 2, lod + 1);
						while (true)
						{
							if (retChunk != nullptr || (posSecondInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
								break;

							if (retChunk == nullptr && posSecondInternalChunkAtLowerLod.getLod() >= 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
							{
								// we have not found a chunk on the X minus quad at the same lod: looking for a chunk at lower lod (most defined)
								retChunk = quadTreeXMinus->getChunkAt(posSecondInternalChunkAtLowerLod);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}
							if (retChunk == nullptr && posChunkAtHigherLod.getLod() <= maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
							{
								// we have not found a chunk on the X minus quad nor at the same nor at lower lod: looking for a chunk at higher lod (less defined)
								retChunk = quadTreeXMinus->getChunkAt(posChunkAtHigherLod);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}

							posSecondInternalChunkAtLowerLod = Chunk::ChunkPos(posSecondInternalChunkAtLowerLod.getSlotPosX() * 2 + 1, posSecondInternalChunkAtLowerLod.getSlotPosZ() * 2,
								posSecondInternalChunkAtLowerLod.getLod() - 1);
							posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
							//idx++;
							//if (idx >= 2)
							//	Godot::print("msg");
						}
					}
				}
			}
			else
			{
				if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				{
					retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::XMinusChunk);
					if (retChunk != nullptr && !retChunk->isActive())
						retChunk = nullptr;
				}
				if (retChunk == nullptr)
				{
					Chunk::ChunkPos posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
					Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(pos.getSlotPosX() / 2, pos.getSlotPosZ() / 2, lod + 1);
					while (true)
					{
						if (retChunk != nullptr || (posFirstInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
							break;

						if (retChunk == nullptr && posFirstInternalChunkAtLowerLod.getLod() > 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
						{
							// we have not found a chunk on the X minus direction at the same lod: looking for a chunk at lower lod (most defined)
							retChunk = quadTree->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XMinusChunk);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}
						if (retChunk == nullptr && posChunkAtHigherLod.getLod() < maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
						{
							// we have not found a chunk on the X minus direction nor at same nor at lower lod: looking for a chunk at higher lod (less defined)
							retChunk = quadTree->getChunkAt(posChunkAtHigherLod, Chunk::DirectionSlot::XMinusChunk);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}

						posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstInternalChunkAtLowerLod.getSlotPosX() * 2, posFirstInternalChunkAtLowerLod.getSlotPosZ() * 2, posFirstInternalChunkAtLowerLod.getLod() - 1);
						posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
						//idx++;
						//if (idx >= 2)
						//	Godot::print("msg");
					}
				}
			}
		}
	}
	break;
	case Chunk::DirectionSlot::XPlusChunk:
	{
		if (posInQuad == PosInQuad::First || posInQuad == PosInQuad::Third)
		{
			if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::XPlusChunk);
			if (retChunk != nullptr && !retChunk->isActive())
				retChunk = nullptr;
		}
		else
		{
			int lod = pos.getLod();
			int maxLod = Globals()->lodMaxDepth();
			int numChunksPerHeightmapSide = Globals()->numChunksPerHeightmapSide(lod);

			if (chunk->getSlotPosX() == numChunksPerHeightmapSide - 1)
			{
				QuadTree* quadTreeXPlus = getQuadTree(quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XPlus));
				if (quadTreeXPlus != nullptr && quadTreeXPlus->isValid() && quadTreeXPlus->isVisible())
				{
					Chunk::ChunkPos posFirstChunkAtSameLod(0, pos.getSlotPosZ(), lod);
					if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
					{
						retChunk = quadTreeXPlus->getChunkAt(posFirstChunkAtSameLod);
						if (retChunk != nullptr && !retChunk->isActive())
							retChunk = nullptr;
					}
					if (retChunk == nullptr)
					{
						Chunk::ChunkPos posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstChunkAtSameLod.getSlotPosX() * 2, posFirstChunkAtSameLod.getSlotPosZ() * 2, lod - 1);
						Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(posFirstChunkAtSameLod.getSlotPosX() / 2, posFirstChunkAtSameLod.getSlotPosZ() / 2, lod + 1);
						while (true)
						{
							if (retChunk != nullptr || (posFirstInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
								break;

							if (retChunk == nullptr && posFirstInternalChunkAtLowerLod.getLod() >= 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
							{
								// we have not found a chunk on the X minus quad at the same lod: looking for a chunk at lower lod (most defined)
								retChunk = quadTreeXPlus->getChunkAt(posFirstInternalChunkAtLowerLod);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}
							if (retChunk == nullptr && posChunkAtHigherLod.getLod() <= maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
							{
								// we have not found a chunk on the X minus quad nor at the same nor at lower lod: looking for a chunk at higher lod (less defined)
								retChunk = quadTreeXPlus->getChunkAt(posChunkAtHigherLod);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}

							posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstInternalChunkAtLowerLod.getSlotPosX() * 2 + 1, posFirstInternalChunkAtLowerLod.getSlotPosZ() * 2,
								posFirstInternalChunkAtLowerLod.getLod() - 1);
							posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
							//idx++;
							//if (idx >= 2)
							//	Godot::print("msg");
						}
					}
				}
			}
			else
			{
				if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				{
					retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::XPlusChunk);
					if (retChunk != nullptr && !retChunk->isActive())
						retChunk = nullptr;
				}
				if (retChunk == nullptr)
				{
					Chunk::ChunkPos posSecondInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2 + 1, pos.getSlotPosZ() * 2, lod - 1);
					Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(pos.getSlotPosX() / 2, pos.getSlotPosZ() / 2, lod + 1);
					while (true)
					{
						if (retChunk != nullptr || (posSecondInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
							break;

						if (retChunk == nullptr && posSecondInternalChunkAtLowerLod.getLod() > 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
						{
							// we have not found a chunk on the X minus direction at the same lod: looking for a chunk at lower lod (most defined)
							retChunk = quadTree->getChunkAt(posSecondInternalChunkAtLowerLod, Chunk::DirectionSlot::XPlusChunk);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}
						if (retChunk == nullptr && posChunkAtHigherLod.getLod() < maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
						{
							// we have not found a chunk on the X minus direction nor at same nor at lower lod: looking for a chunk at higher lod (less defined)
							retChunk = quadTree->getChunkAt(posChunkAtHigherLod, Chunk::DirectionSlot::XPlusChunk);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}

						posSecondInternalChunkAtLowerLod = Chunk::ChunkPos(posSecondInternalChunkAtLowerLod.getSlotPosX() * 2, posSecondInternalChunkAtLowerLod.getSlotPosZ() * 2, posSecondInternalChunkAtLowerLod.getLod() - 1);
						posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
						//idx++;
						//if (idx >= 2)
						//	Godot::print("msg");
					}
				}
			}
		}
	}
	break;
	case Chunk::DirectionSlot::ZMinusChunk:
	{
		if (posInQuad == PosInQuad::Third || posInQuad == PosInQuad::Forth)
		{
			if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::ZMinusChunk);
			if (retChunk != nullptr && !retChunk->isActive())
				retChunk = nullptr;
		}
		else
		{
			int lod = pos.getLod();
			int maxLod = Globals()->lodMaxDepth();
			int numChunksPerHeightmapSide = Globals()->numChunksPerHeightmapSide(lod);

			if (chunk->getSlotPosZ() == 0)
			{
				QuadTree* quadTreeZMinus = getQuadTree(quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZMinus));
				if (quadTreeZMinus != nullptr && quadTreeZMinus->isValid() && quadTreeZMinus->isVisible())
				{
					Chunk::ChunkPos posLastChunkAtSameLod(pos.getSlotPosX(), numChunksPerHeightmapSide - 1, lod);
					if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
					{
						retChunk = quadTreeZMinus->getChunkAt(posLastChunkAtSameLod);
						if (retChunk != nullptr && !retChunk->isActive())
							retChunk = nullptr;
					}
					if (retChunk == nullptr)
					{
						Chunk::ChunkPos posThirdInternalChunkAtLowerLod = Chunk::ChunkPos(posLastChunkAtSameLod.getSlotPosX() * 2, posLastChunkAtSameLod.getSlotPosZ() * 2 + 1, lod - 1);
						Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(posLastChunkAtSameLod.getSlotPosX() / 2, posLastChunkAtSameLod.getSlotPosZ() / 2, lod + 1);
						while (true)
						{
							if (retChunk != nullptr || (posThirdInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
								break;

							if (retChunk == nullptr && posThirdInternalChunkAtLowerLod.getLod() >= 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
							{
								// we have not found a chunk on the X minus quad at the same lod: looking for a chunk at lower lod (most defined)
								retChunk = quadTreeZMinus->getChunkAt(posThirdInternalChunkAtLowerLod);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}
							if (retChunk == nullptr && posChunkAtHigherLod.getLod() <= maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
							{
								// we have not found a chunk on the X minus quad nor at the same nor at lower lod: looking for a chunk at higher lod (less defined)
								retChunk = quadTreeZMinus->getChunkAt(posChunkAtHigherLod);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}

							posThirdInternalChunkAtLowerLod = Chunk::ChunkPos(posThirdInternalChunkAtLowerLod.getSlotPosX() * 2 + 1, posThirdInternalChunkAtLowerLod.getSlotPosZ() * 2,
								posThirdInternalChunkAtLowerLod.getLod() - 1);
							posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
							//idx++;
							//if (idx >= 2)
							//	Godot::print("msg");
						}
					}
				}
			}
			else
			{
				if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				{
					retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::ZMinusChunk);
					if (retChunk != nullptr && !retChunk->isActive())
						retChunk = nullptr;
				}
				if (retChunk == nullptr)
				{
					Chunk::ChunkPos posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
					Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(pos.getSlotPosX() / 2, pos.getSlotPosZ() / 2, lod + 1);
					while (true)
					{
						if (retChunk != nullptr || (posFirstInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
							break;

						if (retChunk == nullptr && posFirstInternalChunkAtLowerLod.getLod() > 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
						{
							// we have not found a chunk on the X minus direction at the same lod: looking for a chunk at lower lod (most defined)
							retChunk = quadTree->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZMinusChunk);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}
						if (retChunk == nullptr && posChunkAtHigherLod.getLod() < maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
						{
							// we have not found a chunk on the X minus direction nor at same nor at lower lod: looking for a chunk at higher lod (less defined)
							retChunk = quadTree->getChunkAt(posChunkAtHigherLod, Chunk::DirectionSlot::ZMinusChunk);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}

						posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstInternalChunkAtLowerLod.getSlotPosX() * 2, posFirstInternalChunkAtLowerLod.getSlotPosZ() * 2, posFirstInternalChunkAtLowerLod.getLod() - 1);
						posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
						//idx++;
						//if (idx >= 2)
						//	Godot::print("msg");
					}
				}
			}
		}
	}
	break;
	case Chunk::DirectionSlot::ZPlusChunk:
	{
		if (posInQuad == PosInQuad::First || posInQuad == PosInQuad::Second)
		{
			if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::ZPlusChunk);
			if (retChunk != nullptr && !retChunk->isActive())
				retChunk = nullptr;
		}
		else
		{
			int lod = pos.getLod();
			int maxLod = Globals()->lodMaxDepth();
			int numChunksPerHeightmapSide = Globals()->numChunksPerHeightmapSide(lod);

			if (chunk->getSlotPosZ() == numChunksPerHeightmapSide - 1)
			{
				QuadTree* quadTreeZPlus = getQuadTree(quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZPlus));
				if (quadTreeZPlus != nullptr && quadTreeZPlus->isValid() && quadTreeZPlus->isVisible())
				{
					Chunk::ChunkPos posFirstChunkAtSameLod(pos.getSlotPosX(), 0, lod);
					if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
					{
						retChunk = quadTreeZPlus->getChunkAt(posFirstChunkAtSameLod);
						if (retChunk != nullptr && !retChunk->isActive())
							retChunk = nullptr;
					}
					if (retChunk == nullptr)
					{
						Chunk::ChunkPos posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstChunkAtSameLod.getSlotPosX() * 2, posFirstChunkAtSameLod.getSlotPosZ() * 2, lod - 1);
						Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(posFirstChunkAtSameLod.getSlotPosX() / 2, posFirstChunkAtSameLod.getSlotPosZ() / 2, lod + 1);
						while (true)
						{
							if (retChunk != nullptr || (posFirstInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
								break;

							if (retChunk == nullptr && posFirstInternalChunkAtLowerLod.getLod() >= 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
							{
								// we have not found a chunk on the X minus quad at the same lod: looking for a chunk at lower lod (most defined)
								retChunk = quadTreeZPlus->getChunkAt(posFirstInternalChunkAtLowerLod);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}
							if (retChunk == nullptr && posChunkAtHigherLod.getLod() <= maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
							{
								// we have not found a chunk on the X minus quad nor at the same nor at lower lod: looking for a chunk at higher lod (less defined)
								retChunk = quadTreeZPlus->getChunkAt(posChunkAtHigherLod);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}

							posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstInternalChunkAtLowerLod.getSlotPosX() * 2 + 1, posFirstInternalChunkAtLowerLod.getSlotPosZ() * 2,
								posFirstInternalChunkAtLowerLod.getLod() - 1);
							posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
							//idx++;
							//if (idx >= 2)
							//	Godot::print("msg");
						}
					}
				}
			}
			else
			{
				if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				{
					retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::ZPlusChunk);
					if (retChunk != nullptr && !retChunk->isActive())
						retChunk = nullptr;
				}
				if (retChunk == nullptr)
				{
					Chunk::ChunkPos posThirdInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2 + 1, lod - 1);
					Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(pos.getSlotPosX() / 2, pos.getSlotPosZ() / 2, lod + 1);
					while (true)
					{
						if (retChunk != nullptr || (posThirdInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
							break;

						if (retChunk == nullptr && posThirdInternalChunkAtLowerLod.getLod() > 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
						{
							// we have not found a chunk on the X minus direction at the same lod: looking for a chunk at lower lod (most defined)
							retChunk = quadTree->getChunkAt(posThirdInternalChunkAtLowerLod, Chunk::DirectionSlot::ZPlusChunk);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}
						if (retChunk == nullptr && posChunkAtHigherLod.getLod() < maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
						{
							// we have not found a chunk on the X minus direction nor at same nor at lower lod: looking for a chunk at higher lod (less defined)
							retChunk = quadTree->getChunkAt(posChunkAtHigherLod, Chunk::DirectionSlot::ZPlusChunk);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}

						posThirdInternalChunkAtLowerLod = Chunk::ChunkPos(posThirdInternalChunkAtLowerLod.getSlotPosX() * 2, posThirdInternalChunkAtLowerLod.getSlotPosZ() * 2, posThirdInternalChunkAtLowerLod.getLod() - 1);
						posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
						//idx++;
						//if (idx >= 2)
						//	Godot::print("msg");
					}
				}
			}
		}
	}
	break;
	}

	return retChunk;
}

void GDN_TheWorld_Viewer::createEditModeUI(void)
{
	GDN_TheWorld_Edit* _editModeUIControl = EditModeUIControl();
	if (_editModeUIControl != nullptr)
		return;

	m_mtxQuadTreeAndMainProcessing.lock();
	GDN_TheWorld_Edit* editModeUIControl = GDN_TheWorld_Edit::_new();
	m_mtxQuadTreeAndMainProcessing.unlock();
	if (editModeUIControl == nullptr)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	editModeUIControl->init(this);
}

QuadrantPos GDN_TheWorld_Viewer::getQuadrantSelForEdit(QuadTree** quadTreeSel)
{
	*quadTreeSel = nullptr;
	QuadrantPos ret = m_quadrantSelPos;

	if (ret.empty())
		return ret;

	{
		*quadTreeSel = getQuadTree(ret);
		if (*quadTreeSel == nullptr)
			return QuadrantPos();
		else
			return ret;

		//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
		//MapQuadTree::iterator it = m_mapQuadTree.find(ret);
		//if (it == m_mapQuadTree.end() || it->second->statusToErase())
		//	return QuadrantPos();
		//else
		//{
		//	*quadTreeSel = it->second.get();
		//	return ret;
		//}
	}
}

godot::GDN_TheWorld_Edit* GDN_TheWorld_Viewer::EditModeUIControl(bool useCache)
{
	if (m_editModeUIControl == nullptr || !useCache)
	{
		SceneTree* scene = get_tree();
		if (!scene)
			return NULL;
		Viewport* root = scene->get_root();
		if (!root)
			return NULL;
		m_editModeUIControl = Object::cast_to<GDN_TheWorld_Edit>(root->find_node(THEWORLD_EDIT_MODE_UI_CONTROL_NAME, true, false));
	}

	return m_editModeUIControl;
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

bool GDN_TheWorld_Viewer::terrainShiftPermitted(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();

	if (editModeUIControl == nullptr)
		return true;
	else
	{
		if (editModeUIControl->UIAcceptingFocus())
			return false;
		else
			return true;
		//if (!editMode() && !editModeUIControl->actionInProgress())
		//	return true;
		//else
		//	return false;
	}

	return true;
	return m_numinitializedQuadrant >= m_numQuadrant;
}

void GDN_TheWorld_Viewer::resetInitialWordlViewerPos(float x, float z, float cameraDistanceFromTerrain, int level, int chunkSizeShift, int heightmapResolutionShift)
{
	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System

	enum class TheWorldStatus status = Globals()->status();
	if (status < TheWorldStatus::sessionInitialized)
		DebugBreak();

	assert(status >= TheWorldStatus::sessionInitialized);
	if (status < TheWorldStatus::sessionInitialized)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Incorrect Status for resetInitialWordlViewerPos").c_str()));

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
		
		float _gridStepInWU = Globals()->gridStepInWU();
		QuadrantPos quadrantPos(x, z, level, m_numWorldVerticesPerSize, _gridStepInWU);
		quadrantPos.setTag("Camera");

		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

		m_mapQuadTree.clear();
		m_mapQuadTree[quadrantPos] = make_unique<QuadTree>(this, quadrantPos);
		m_mapQuadTree[quadrantPos]->init(x, z, true, cameraDistanceFromTerrain);

		Globals()->setStatus(TheWorldStatus::worldDeployInProgress);
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
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("Transform changed");
	
	//The transformand other properties can be set by the scene loader, before we enter the tree
	if (!is_inside_tree())
		return;

	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (itQuadTree->second->status() != QuadrantStatus::uninitialized && itQuadTree->second->status() != QuadrantStatus::toErase)
			itQuadTree->second->onGlobalTransformChanged();

		if (!itQuadTree->second->isValid())
			continue;
		
		if (!itQuadTree->second->isVisible())
			continue;
		
		Chunk::TransformChangedChunkAction action;
		itQuadTree->second->ForAllChunk(action);

		itQuadTree->second->materialParamsNeedUpdate(true);

		itQuadTree->second->getQuadrant()->getCollider()->onGlobalTransformChanged();
	}
}

//void GDN_TheWorld_Viewer::setMapScale(Vector3 mapScaleVector)
//{
//	if (m_mapScaleVector == mapScaleVector)
//		return;
//
//	mapScaleVector.x = Utils::max2(mapScaleVector.x, MIN_MAP_SCALE);
//	mapScaleVector.y = Utils::max2(mapScaleVector.y, MIN_MAP_SCALE);
//	mapScaleVector.z = Utils::max2(mapScaleVector.z, MIN_MAP_SCALE);
//
//	m_mapScaleVector = mapScaleVector;
//
//	onTransformChanged();
//}

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

Transform GDN_TheWorld_Viewer::getCameraChunkGlobalTransformApplied(void)
{
	if (m_cameraChunk && !m_cameraChunk->isMeshNull())
		return m_cameraChunk->getGlobalTransformApplied();
	else
		return Transform();
}

Transform GDN_TheWorld_Viewer::getCameraChunkDebugGlobalTransformApplied(void)
{
	if (m_cameraChunk && !m_cameraChunk->isDebugMeshNull())
		return m_cameraChunk->getDebugGlobalTransformApplied();
	else
		return Transform();
}


String GDN_TheWorld_Viewer::getCameraChunkId(void)
{
	if (m_cameraChunk)
		return m_cameraChunk->getPos().getIdStr().c_str();
	else
		return "";
}

String GDN_TheWorld_Viewer::getCameraQuadrantName(void)
{
	if (m_cameraChunk)
		return m_cameraChunk->getQuadTree()->getQuadrant()->getPos().getName().c_str();
	else
		return "";
}

void GDN_TheWorld_Viewer::refreshQuadTreeStatistics()
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	int numSplits = 0;
	int numJoins = 0;
	int numChunks = 0;
	int numActiveChunks = 0;
	int numQuadrant = (int)m_mapQuadTree.size();
	int numinitializedQuadrant = 0;
	int numVisibleQuadrant = 0;
	int numEmptyQuadrant = 0;
	int numFlushedQuadrant = 0;
	int numinitializedVisibleQuadrant = 0;

	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (itQuadTree->second->isVisible())
		{
			numVisibleQuadrant++;
			if (itQuadTree->second->statusInitialized())
				numinitializedVisibleQuadrant++;
		}
		
		if (itQuadTree->second->statusInitialized())
		{
			numSplits += itQuadTree->second->getNumSplits();
			numJoins += itQuadTree->second->getNumJoins();
			numChunks += itQuadTree->second->getNumChunks();
			numActiveChunks += itQuadTree->second->getNumActiveChunks();
			numinitializedQuadrant++;
		}

		if (itQuadTree->second->getQuadrant()->empty())
			numEmptyQuadrant++;

		if (itQuadTree->second->getQuadrant()->getFloat16HeightsBuffer(false).size() == 0 || itQuadTree->second->getQuadrant()->getFloat32HeightsBuffer(false).size() == 0)
			numFlushedQuadrant++;
	}

	m_numSplits = numSplits;
	m_numJoins = numJoins;
	m_numChunks = numChunks;
	m_numActiveChunks = numActiveChunks;
	m_numQuadrant = numQuadrant;
	m_numinitializedQuadrant = numinitializedQuadrant;
	m_numVisibleQuadrant = numVisibleQuadrant;
	m_numEmptyQuadrant = numEmptyQuadrant;
	m_numFlushedQuadrant = numFlushedQuadrant;
	m_numinitializedVisibleQuadrant = numinitializedVisibleQuadrant;
}

int GDN_TheWorld_Viewer::getProcessNotOwnsLock(void)
{
	return m_numProcessNotOwnsLock;
}

int GDN_TheWorld_Viewer::getNumChunks(void)
{
	return m_numChunks;
}

int GDN_TheWorld_Viewer::getNumActiveChunks(void)
{
	return m_numActiveChunks;
}


int GDN_TheWorld_Viewer::getNumSplits(void)
{
	return m_numSplits;
}

int GDN_TheWorld_Viewer::getNumJoins(void)
{
	return m_numJoins;
}

int GDN_TheWorld_Viewer::getNumQuadrant(void)
{
	return m_numQuadrant;
}

int GDN_TheWorld_Viewer::getNuminitializedQuadrant(void)
{
	return m_numinitializedQuadrant;
}

int GDN_TheWorld_Viewer::getNumVisibleQuadrant(void)
{
	return m_numVisibleQuadrant;
}

int GDN_TheWorld_Viewer::getNumEmptyQuadrant(void)
{
	return m_numEmptyQuadrant;
}

int GDN_TheWorld_Viewer::getNumFlushedQuadrant(void)
{
	return m_numFlushedQuadrant;
}

int GDN_TheWorld_Viewer::getNuminitializedVisibleQuadrant(void)
{
	return m_numinitializedVisibleQuadrant;
}

int GDN_TheWorld_Viewer::getProcessDuration(void)
{
	return m_averageProcessDuration;
}

int GDN_TheWorld_Viewer::getRefreshMapQuadDuration(void)
{
	return m_averageRefreshMapQuadDuration;
}

int GDN_TheWorld_Viewer::getUpdateQuads1Duration(void)
{
	return m_averageUpdateQuads1Duration;
}

int GDN_TheWorld_Viewer::getUpdateQuads2Duration(void)
{
	return m_averageUpdateQuads2Duration;
}

int GDN_TheWorld_Viewer::getUpdateQuads3Duration(void)
{
	return m_averageUpdateQuads3Duration;
}

int GDN_TheWorld_Viewer::getUpdateChunksDuration(void)
{
	return m_averageUpdateChunksDuration;
}

int GDN_TheWorld_Viewer::getUpdateMaterialParamsDuration(void)
{
	return m_averageUpdateMaterialParamsDuration;
}

int GDN_TheWorld_Viewer::getMouseTrackHitDuration(void)
{
	return m_averageMouseTrackHitDuration;
}

void GDN_TheWorld_Viewer::dump()
{
	Globals()->debugPrint("*************");
	Globals()->debugPrint("STARTING DUMP");

	float f = Engine::get_singleton()->get_frames_per_second();
	Globals()->debugPrint("FPS: " + String(std::to_string(f).c_str()));

	{
		size_t memoryOccupation = 0;
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
		for (auto& quadTree : m_mapQuadTree)
			memoryOccupation += quadTree.second->getQuadrant()->calcMemoryOccupation();
		Globals()->debugPrint("Quadrant busy memory : " + String(std::to_string(memoryOccupation).c_str()));
	}

	//Node* node = get_node(NodePath("/root"));
	//if (node != nullptr)
	//{
	//	Globals()->debugPrint("=== NODES ===========================================");
	//	Globals()->debugPrint("");
	//	Globals()->debugPrint("@@2 = res://native/GDN_TheWorld_Viewer.gdns");
	//	Globals()->debugPrint("");
	//	Globals()->debugPrint(node->get_name());
	//	Array nodes = node->get_children();
	//	dumpRecurseIntoChildrenNodes(nodes, 1);
	//	Globals()->debugPrint("=== NODES ===========================================");
	//}
	
	//{
	//	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
	//	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	//	{
	//		itQuadTree->second->dump();
	//	}
	//}

	std::vector<std::string> names = TheWorld_Utils::Profiler::names("");
	if (names.size() > 0)
	{
		Globals()->debugPrint("== PROFILE DATA ==========================================");
		for (auto& name : names)
		{
			size_t num = 0;
			size_t elapsed = TheWorld_Utils::Profiler::elapsedMs("", name, num);
			size_t avgElapsed = TheWorld_Utils::Profiler::averageElapsedMs("", name, num);
			size_t minElapsed = TheWorld_Utils::Profiler::minElapsedMs("", name);
			size_t maxElapsed = TheWorld_Utils::Profiler::maxElapsedMs("", name);
			if (num > 0)
			{
				Globals()->debugPrint(String(name.c_str()) + " num=" + std::to_string(num).c_str() + " Elapsed=" + std::to_string(elapsed).c_str() + " MinElapsed=" + std::to_string(minElapsed).c_str() + " MaxElapsed=" + std::to_string(maxElapsed).c_str() + " AvgElapsed=" + std::to_string(avgElapsed).c_str());
				TheWorld_Utils::Profiler::reset("", name);
			}
		}
		Globals()->debugPrint("== PROFILE DATA ==========================================");
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
	//m_cameraQuadTree = quadTree;
}

void GDN_TheWorld_Viewer::streamer(void)
{
	m_streamerThreadRunning = true;

	while (!m_streamerThreadRequiredExit)
	{
		try
		{
			try
			{
				{
					//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
					std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing, std::try_to_lock);
					if (lock.owns_lock())
					{
						for (MapQuadTree::iterator it = m_mapQuadTree.begin(); it != m_mapQuadTree.end(); it++)
						{
							bool reset = false;
							//reset = it->second->resetMaterialParams();

							if (it->second->statusToErase() && !it->second->getQuadrant()->internalDataLocked())
							{
								m_toErase.push_back(it->first);
								break;
							}
						}

						lock.unlock();
					}
				}

				// Optimization: calc here getHeightsForCollider, getFloat16HeightsBuffer, getFloat32HeightsBuffer, getNormalsBuffer if they are empty and if we know they will be loaded in next _process
				// with this optimization we want to do the work outside _process to have more fluidity
				
				if (m_recalcQuadrantsInViewNeeded)
				{
					recalcQuadrantsInView();
					m_recalcQuadrantsInViewNeeded = false;
				}
				if (m_dumpRequired)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("streamer 1 ") + __FUNCTION__, "Dump stuff");

					m_dumpRequired = false;
					dump();
				}

			}
			catch (std::exception& e)
			{
				Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::streamer - std::exception caught - ") + e.what()).c_str());
				throw(e);
			}
			catch (...)
			{
				Globals()->errorPrint("GDN_TheWorld_Viewer::streamer - Exception caught");
				throw(new exception("exception caught"));
			}
		}
		catch (std::exception& e)
		{
			Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, string("GDN_TheWorld_Viewer::streamer - std::exception caught - ") + e.what());
		}
		catch (...)
		{
			Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "GDN_TheWorld_Viewer::streamer - Exception caught");
		}

		Sleep(STREAMER_SLEEP_TIME);
	}

	m_streamerThreadRunning = false;
}

void GDN_TheWorld_Viewer::streamingQuadrantStuff(void)
{
	bool allQuadrantInitialized = true;

	std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing, std::try_to_lock);
	if (lock.owns_lock())
	{
		bool exitForNow = false;
		for (size_t distance = 0; distance <= m_numCacheQuadrantOnPerimeter; distance++)
		{
			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				//if (itQuadTree->second->status() != QuadrantStatus::initialized)
				//if (itQuadTree->second->status() != QuadrantStatus::initialized && itQuadTree->second->status() != QuadrantStatus::toErase)
				if (itQuadTree->second->status() < QuadrantStatus::initialized)
					allQuadrantInitialized = false;

				if (itQuadTree->second->statusUninitialized() || itQuadTree->second->statusRefreshTerrainDataNeeded())
				{
					size_t distanceInPerimeterFromCameraQuadrant = itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(m_computedCameraQuadrantPos);
					if (distanceInPerimeterFromCameraQuadrant == distance)
					{
						float x = 0, z = 0;
						if (itQuadTree->second->statusUninitialized())
							itQuadTree->second->init(x, z);
						else
							itQuadTree->second->refreshTerrainData(x, z);
						exitForNow = true;
						break;
					}
				}
			}

			if (exitForNow)
				break;
		}

		lock.unlock();

		if (allQuadrantInitialized)
		{
			if (Globals()->status() == TheWorldStatus::worldDeployInProgress && ((m_numVisibleQuadrantOnPerimeter == 0 && m_mapQuadTree.size() == 1) || m_mapQuadTree.size() > 1))
				Globals()->setStatus(TheWorldStatus::worldDeployed);

			if (m_streamingTime.counterStarted())
			{
				m_streamingTime.tock();
				PLOG_DEBUG << "Initialization of last stream of quadrant in " << m_streamingTime.duration().count() << " ms";
			}
		}
	}
}

Transform GDN_TheWorld_Viewer::getInternalGlobalTransform(void)
{
	Transform gt = get_global_transform();
	return gt;
}
