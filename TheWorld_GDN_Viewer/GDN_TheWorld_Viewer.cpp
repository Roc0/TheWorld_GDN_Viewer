//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Camera.h"
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
#include <filesystem>

#include "MeshCache.h"
#include "QuadTree.h"

#include <algorithm>

using namespace godot;
using namespace std;

namespace fs = std::filesystem;

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
	register_method("get_num_process_not_owns_lock", &GDN_TheWorld_Viewer::getProcessNotOwnsLock);
	register_method("get_process_duration", &GDN_TheWorld_Viewer::getProcessDuration);
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
	m_cameraQuadTree = nullptr;
	m_refreshMapQuadTree = false;
	m_globals = nullptr;
	m_timeElapsedFromLastDump = 0;
	m_timeElapsedFromLastStatistic = 0;
	m_duration = 0;
	m_numProcessExecution = 0;
	m_averageProcessDuration = 0;
	m_numSplits = 0;
	m_numJoins = 0;
	m_numActiveChunks = 0;
	m_numChunks = 0;
	m_numProcessNotOwnsLock = 0;
	m_numQuadrant = 0;
	m_numinitializedQuadrant = 0;
	m_numVisibleQuadrant = 0;
	m_numinitializedVisibleQuadrant = 0;
	m_debugContentVisibility = true;
	m_updateTerrainVisibilityRequired = false;
	m_currentChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
	m_requiredChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
	m_updateDebugModeRequired = false;
	m_numWorldVerticesPerSize = 0;
	m_debugDraw = Viewport::DebugDraw::DEBUG_DRAW_DISABLED;
	m_ctrlPressed = false;
	m_streamerThreadRequiredExit = false;
	m_streamerThreadRunning = false;
	m_numVisibleQuadrantOnPerimeter = 0;
	m_numCacheQuadrantOnPerimeter = 0;
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

		m_mapQuadTree.clear();
		m_meshCache.reset();
		
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
			TheWorld_Utils::TimerMs clock("GDN_TheWorld_Viewer::replyFromServer", "CLIENT SIDE", false, true);
			
			//clock.headerMsg("MapManager::getVertices - Acq input");
			//clock.tick();

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
				std::string m = std::string("Reply MapManager::getVertices did not have a string as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float lowerXGridVertex = *_lowerXGridVertex;

			v = reply.getInputParam(3);
			const auto _lowerZGridVertex(std::get_if<float>(&v));
			if (_lowerZGridVertex == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a string as forth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float lowerZGridVertex = *_lowerZGridVertex;

			v = reply.getInputParam(4);
			const auto _numVerticesPerSize(std::get_if<int>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a string as fifth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(5);
			const auto _gridStepinWU(std::get_if<float>(&v));
			if (_gridStepinWU == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a string as sixth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepinWU = *_gridStepinWU;

			v = reply.getInputParam(6);
			const auto _level(std::get_if<int>(&v));
			if (_level == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a string as seventh input param");
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
				std::string m = std::string("Reply MapManager::getVertices did not have a string as nineth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			bool setCamera = *_setCamera;

			v = reply.getInputParam(9);
			const auto _cameraDistanceFromTerrain(std::get_if<float>(&v));
			if (_cameraDistanceFromTerrain == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a string as tenth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraDistanceFromTerrain = *_cameraDistanceFromTerrain;

			QuadrantPos quadrantPos(lowerXGridVertex, lowerZGridVertex, level, numVerticesPerSize, gridStepinWU);

			//clock.tock();

			//clock.headerMsg("MapManager::getVertices - Lock m_mtxQuadTree");
			//clock.tick();
			//std::lock_guard lock(m_mtxQuadTree);
			m_mtxQuadTree.lock();
			//clock.tock();
			if (m_mapQuadTree.contains(quadrantPos))
			{
				QuadTree* quadTree = m_mapQuadTree[quadrantPos].get();
				m_mtxQuadTree.unlock();
				if (quadTree->status() == QuadrantStatus::getVerticesInProgress)
				{
					//clock.headerMsg("MapManager::getVertices - Lock quadrantMutex");
					//clock.tick();
					std::recursive_mutex& quadrantMutex = quadTree->getQuadrantMutex();
					std::lock_guard lock(quadrantMutex);
					//clock.tock();

					std::string meshIdFromBuffer;
					{
						// ATTENZIONE
						//std::lock_guard lock(m_mtxQuadTree);	// SUPER DEBUGRIC : to remove when the mock for altitudes is removed from cache.refreshMeshCacheFromBuffer
						quadTree->getQuadrant()->refreshGridVertices(*_buffGridVerticesFromServer, meshIdFromServer, meshIdFromBuffer);
					}
					
					std::vector<TheWorld_Utils::GridVertex>& vectGridVertices = quadTree->getQuadrant()->getGridVertices();

					//clock.headerMsg("MapManager::getVertices - resetMaterialParams");
					//clock.tick();
					{
						// ATTENZIONE
						//std::lock_guard lock(m_mtxQuadTree);
						quadTree->materialParamsNeedReset(true);
						//quadTree->resetMaterialParams();
					}
					//clock.tock();
					//Globals()->debugPrint(String("ELAPSED - QUADRANT ") + quadTree->getQuadrant()->getPos().getId().c_str() + " TAG=" + quadTree->getQuadrant()->getPos().getTag().c_str() + " - GDN_TheWorld_Viewer::replyFromServer MapManager::getVertices (resetMaterialParams) " + std::to_string(clock2.duration().count()).c_str() + " ms");

					if (setCamera)
					{
						quadTree->setVisible(true);

						forceRefreshMapQuadTree();

						TheWorld_Utils::GridVertex viewerPos(viewerPosX, 0, viewerPosZ, level);
						std::vector<TheWorld_Utils::GridVertex>::iterator it = std::find(vectGridVertices.begin(), vectGridVertices.end(), viewerPos);
						if (it == vectGridVertices.end())
						{
							bool found = false;
							for (it = vectGridVertices.begin(); it != vectGridVertices.end(); it++)
							{
								if (it->equalApartFromAltitude(viewerPos))
								{
									found = true;
									break;
								}
							}
							if (!found)
								throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Not found WorldViewer Pos").c_str()));
						}
						Vector3 cameraPos(viewerPosX, it->altitude() + cameraDistanceFromTerrain, viewerPosZ);		// MapManager coordinates are local coordinates of WorldNode
						Vector3 lookAt(viewerPosX + cameraDistanceFromTerrain, it->altitude(), viewerPosZ + cameraDistanceFromTerrain);

						// Viewer stuff: set viewer position relative to world node at the first point of the bitmap and altitude 0 so that that point is at position (0,0,0) respect to the viewer
						//Transform t = get_transform();
						//t.origin = Vector3(quadTree->getQuadrant()->getGridVertices()[0].posX(), 0, quadTree->getQuadrant()->getGridVertices()[0].posZ());
						//set_transform(t);

						WorldCamera()->initCameraInWorld(cameraPos, lookAt);

						m_initialWordlViewerPosSet = true;
					}

					quadTree->setStatus(QuadrantStatus::initialized);
					
					m_refreshMapQuadTree = true;
				}
			}
			else
				m_mtxQuadTree.unlock();
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
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if (globals->status() != TheWorldStatus::sessionInitialized)
		return;

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
			std::lock_guard lock(m_mtxQuadTree);
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
			std::lock_guard lock(m_mtxQuadTree);
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
			std::lock_guard lock(m_mtxQuadTree);
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

void GDN_TheWorld_Viewer::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_TheWorld_Viewer::_process");

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if (globals->status() != TheWorldStatus::sessionInitialized)
		return;

	if (!WorldCamera() || !m_initialWordlViewerPosSet)
		return;

	GDN_TheWorld_Camera* activeCamera = WorldCamera()->getActiveCamera();
	if (!activeCamera)
		return;

	//std::lock_guard lock(m_mtxQuadTree);
	std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTree, std::try_to_lock);
	if (!lock.owns_lock())
	{
		m_numProcessNotOwnsLock++;
		return;
	}

	TheWorld_Utils::TimerMcs clock;
	clock.tick();
	auto save_duration = TheWorld_Utils::finally([&clock, this] {
		clock.tock(); 
		this->m_numProcessExecution++; 
		this->m_duration += clock.duration().count(); 
		});

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
	//Transform globalTransform = internalTransformGlobalCoord();
	//Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node (grid) local coordinates of the camera pos

	// ADJUST QUADTREEs NEEDED ACCORDING TO CAMERA POS
	{
		float farHorizon = cameraPosGlobalCoord.y * FAR_HORIZON_MULTIPLIER;
		if (globals->heightmapSizeInWUs() > farHorizon)
			farHorizon = globals->heightmapSizeInWUs();

		// Look for Camera QuadTree: put it as first element of quadrantPosNeeded
		vector<QuadrantPos> quadrantPosNeeded;
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			float quadrantSizeInWU = itQuadTree->second->getQuadrant()->getPos().getSizeInWU();
			if (cameraPosGlobalCoord.x >= itQuadTree->second->getQuadrant()->getPos().getLowerXGridVertex() && cameraPosGlobalCoord.x <= (itQuadTree->second->getQuadrant()->getPos().getLowerXGridVertex() + quadrantSizeInWU)
				&& cameraPosGlobalCoord.z >= itQuadTree->second->getQuadrant()->getPos().getLowerZGridVertex() && cameraPosGlobalCoord.z <= (itQuadTree->second->getQuadrant()->getPos().getLowerZGridVertex() + quadrantSizeInWU))
			{
				quadrantPosNeeded.push_back(itQuadTree->second->getQuadrant()->getPos());
				quadrantPosNeeded[0].setTag("Camera");
				break;
			}
		}

		if (quadrantPosNeeded.size() == 0)	// boh not found camera-quad-tree (it seems impossible) ...
		{
			// ... btw we create new quad tree containing camera
			float x = cameraPosGlobalCoord.x, z = cameraPosGlobalCoord.z;
			float _gridStepInWU = globals->gridStepInWU();
			QuadrantPos q(x, z, m_worldViewerLevel, m_numWorldVerticesPerSize, _gridStepInWU);
			quadrantPosNeeded.push_back(q);
			quadrantPosNeeded[0].setTag("Camera");
		}

		//
		// now quadrantPosNeeded has 1 element and quadrantPosNeeded[0] is the camera 
		//

		// if forced or the camera has changed quadrant the cache is repopulated
		if (m_refreshMapQuadTree || m_computedCameraQuadrantPos != quadrantPosNeeded[0])
		{
			TheWorld_Utils::TimerMs clock1("GDN_TheWorld_Viewer::_process", "Recalc in memory map of quadrants", false, true);
			clock1.tick();

			// Da Rimuovere (???)
			//Transform t = get_global_transform();
			//t.origin = Vector3(quadrantPosNeeded[0].getLowerXGridVertex(), 0, quadrantPosNeeded[0].getLowerZGridVertex());
			//set_global_transform(t);

			m_refreshMapQuadTree = false;
			m_computedCameraQuadrantPos = quadrantPosNeeded[0];

			// calculate minimum distance of camera from quad tree borders
			float minimunDistanceOfCameraFromBordersOfQuadrant = cameraPosGlobalCoord.x - quadrantPosNeeded[0].getLowerXGridVertex();
			float f = (quadrantPosNeeded[0].getLowerXGridVertex() + quadrantPosNeeded[0].getSizeInWU()) - cameraPosGlobalCoord.x;
			if (f < minimunDistanceOfCameraFromBordersOfQuadrant)
				minimunDistanceOfCameraFromBordersOfQuadrant = f;
			f = cameraPosGlobalCoord.z - quadrantPosNeeded[0].getLowerZGridVertex();
			if (f < minimunDistanceOfCameraFromBordersOfQuadrant)
				minimunDistanceOfCameraFromBordersOfQuadrant = f;
			f = (quadrantPosNeeded[0].getLowerZGridVertex() + quadrantPosNeeded[0].getSizeInWU()) - cameraPosGlobalCoord.z;
			if (f < minimunDistanceOfCameraFromBordersOfQuadrant)
				minimunDistanceOfCameraFromBordersOfQuadrant = f;

			// calculate num of quad needed surrounding camera-quad-tree according to desidered farHorizon
			m_numVisibleQuadrantOnPerimeter = 0;
			if (farHorizon > minimunDistanceOfCameraFromBordersOfQuadrant)
			{
				m_numVisibleQuadrantOnPerimeter = (size_t)floor((farHorizon - minimunDistanceOfCameraFromBordersOfQuadrant) / quadrantPosNeeded[0].getSizeInWU()) + 1;
				if (m_numVisibleQuadrantOnPerimeter > 3)
					m_numVisibleQuadrantOnPerimeter = 3;
				m_numVisibleQuadrantOnPerimeter = 0;	// SUPER DEBUGRIC only camera quadrant
			}

			m_numCacheQuadrantOnPerimeter = m_numVisibleQuadrantOnPerimeter * 2;

			// add horizontal (X axis) quadrants on the left and on the right
			for (int i = 0; i < m_numCacheQuadrantOnPerimeter; i++)
			{
				QuadrantPos q = quadrantPosNeeded[0].getQuadrantPos(QuadrantPos::DirectionSlot::XPlus, 1 + i);
				q.setTag(quadrantPosNeeded[0].getTag() + " X+" + std::to_string(1 + i));
				quadrantPosNeeded.push_back(q);
				//break;	// SUPER DEBUGRIC only X+1 quadrant
				q = quadrantPosNeeded[0].getQuadrantPos(QuadrantPos::DirectionSlot::XMinus, 1 + i);
				q.setTag(quadrantPosNeeded[0].getTag() + " X-" + std::to_string(1 + i));
				quadrantPosNeeded.push_back(q);
			}

			// for each horizontal quadrant ...
			size_t size = quadrantPosNeeded.size();
			for (size_t idx = 0; idx < size; idx++)
			{
				// ... add vertical (Z axis) quadrants up and down
				for (size_t i = 0; i < m_numCacheQuadrantOnPerimeter; i++)
				{
					//break;	// SUPER DEBUGRIC only X+1 quadrant
					QuadrantPos q = quadrantPosNeeded[idx].getQuadrantPos(QuadrantPos::DirectionSlot::ZPlus, 1 + int(i));
					q.setTag(quadrantPosNeeded[idx].getTag() + " Z+" + std::to_string(1 + i));
					quadrantPosNeeded.push_back(q);
					q = quadrantPosNeeded[idx].getQuadrantPos(QuadrantPos::DirectionSlot::ZMinus, 1 + int(i));
					q.setTag(quadrantPosNeeded[idx].getTag() + " Z-" + std::to_string(1 + i));
					quadrantPosNeeded.push_back(q);
				}
			}

			// insert needed quadrants not present in map and refresh the ones which are already in it
			//vector<QuadrantPos> quadrantToAdd;
			std::timespec refreshTime;
			int ret = timespec_get(&refreshTime, TIME_UTC);
			size = quadrantPosNeeded.size();
			for (int idx = 0; idx < size; idx++)
			{
				MapQuadTree::iterator it = m_mapQuadTree.find(quadrantPosNeeded[idx]);
				if (it == m_mapQuadTree.end())
				{
					m_mapQuadTree[quadrantPosNeeded[idx]] = make_unique<QuadTree>(this, quadrantPosNeeded[idx]);
					m_mapQuadTree[quadrantPosNeeded[idx]]->refreshTime(refreshTime);
				}
				else
					it->second->refreshTime(refreshTime);
			}

			// look for the quadrant to delete (the ones not refreshed)
			vector<QuadrantPos> quadrantToDelete;
			for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			{
				if (itQuadTree->second->getRefreshTime().tv_nsec == refreshTime.tv_nsec && itQuadTree->second->getRefreshTime().tv_sec == refreshTime.tv_sec)
				{
					// 
					bool visibilityChanged = false;
					if (itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(quadrantPosNeeded[0]) > m_numVisibleQuadrantOnPerimeter)
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
						}
					}

					if (visibilityChanged)
					{
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
					if (m_cameraQuadTree != nullptr /* && m_cameraQuadTree->isValid()*/ && itQuadTree->first == m_cameraQuadTree->getQuadrant()->getPos())
					{
						// if we delete the ones containing old camera chunk we invalidate it
						m_cameraChunk = nullptr;
						m_cameraQuadTree = nullptr;
					}
				}
			}
			for (int idx = 0; idx < quadrantToDelete.size(); idx++)
				m_mapQuadTree.erase(quadrantToDelete[idx]);
			quadrantToDelete.clear();
		}
	}
	// ADJUST QUADTREEs NEEDED ACCORDING TO CAMERA POS

	// UPDATE QUADS
	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		itQuadTree->second->update(cameraPosGlobalCoord);
	}

	if (m_refreshRequired)
	{
		Chunk::RefreshChunkAction action(is_visible_in_tree());
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			itQuadTree->second->ForAllChunk(action);
		}
		m_refreshRequired = false;
	}

	// UPDATE CHUNKS
	{
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
				Chunk::ChunkPos posFirstInternalChunk(0, 0, 0); 
				if (lod > 0)
				{
					posFirstInternalChunk = Chunk::ChunkPos(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
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
					Chunk* chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::XMinusQuadSecondChunk);
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
					//	o =
					//	= =
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
					//	o =
					//	= =
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
					//	o =
					//	= =
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::ZMinusQuadThirdChunk);
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
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::ZMinusQuadForthChunk);
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
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::ZPlusQuadFirstChunk);
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
					chunk = itQuadTree->second->getChunkAt(posFirstInternalChunk, Chunk::DirectionSlot::ZPlusQuadSecondChunk);
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
						if (iter != m_mapQuadTree.end())
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
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(numChunksPerSideAtLowerLod - 1, posFirstInternalChunk.getSlotPosZ(), posFirstInternalChunk.getLod());
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
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(numChunksPerSideAtLowerLod - 1, posFirstInternalChunk.getSlotPosZ() + 1, posFirstInternalChunk.getLod());
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
						if (iter != m_mapQuadTree.end())
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
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(0, posFirstInternalChunk.getSlotPosZ(), posFirstInternalChunk.getLod());
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
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(0, posFirstInternalChunk.getSlotPosZ() + 1, posFirstInternalChunk.getLod());
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
						if (iter != m_mapQuadTree.end())
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
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunk.getSlotPosX(), numChunksPerSideAtLowerLod - 1, posFirstInternalChunk.getLod());
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
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunk.getSlotPosX() + 1, numChunksPerSideAtLowerLod - 1, posFirstInternalChunk.getLod());
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
						if (iter != m_mapQuadTree.end())
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
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunk.getSlotPosX(), 0, posFirstInternalChunk.getLod());
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
										Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunk.getSlotPosX() + 1, 0, posFirstInternalChunk.getLod());
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

	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		itQuadTree->second->resetMaterialParams();
		itQuadTree->second->updateMaterialParams();
	}

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
	if (m_dumpRequired)
	{
		m_dumpRequired = false;
		dump();
	}

	// Check for Statistics
	if (TIME_INTERVAL_BETWEEN_STATISTICS != 0)
	{
		int64_t timeElapsed = OS::get_singleton()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastStatistic > TIME_INTERVAL_BETWEEN_STATISTICS)
		{
			m_numProcessNotOwnsLock = 0;
			
			refreshQuadTreeStatistics();
			
			if (m_numProcessExecution > 0)
			{
				m_averageProcessDuration = int(m_duration / m_numProcessExecution);
				//globals->debugPrint(String("m_numProcessExecution=") + std::to_string(m_numProcessExecution).c_str() + ", m_duration=" + std::to_string(m_duration).c_str() + ", m_averageProcessDuration=" + std::to_string(m_averageProcessDuration).c_str());
				m_duration = 0;
				m_numProcessExecution = 0;
			}
			else
				m_averageProcessDuration = 0;
			
			m_timeElapsedFromLastStatistic = timeElapsed;
		}
	}
}

void GDN_TheWorld_Viewer::_physics_process(float _delta)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if (globals->status() != TheWorldStatus::sessionInitialized)
		return;

	Input* input = Input::get_singleton();

	if (input->is_action_pressed("ui_ctrl"))
		m_ctrlPressed = true;
	else
		m_ctrlPressed = false;
}

QuadTree* GDN_TheWorld_Viewer::getQuadTreeFromInternalMap(QuadrantPos pos)
{
	auto iter = m_mapQuadTree.find(pos);
	if (iter == m_mapQuadTree.end())
		return nullptr;

	return iter->second.get();
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
		
		float _gridStepInWU = Globals()->gridStepInWU();
		QuadrantPos quadrantPos(x, z, level, m_numWorldVerticesPerSize, _gridStepInWU);
		quadrantPos.setTag("Camera");

		std::lock_guard lock(m_mtxQuadTree);

		m_mapQuadTree.clear();
		m_mapQuadTree[quadrantPos] = make_unique<QuadTree>(this, quadrantPos);
		m_mapQuadTree[quadrantPos]->init(x, z, true, cameraDistanceFromTerrain);
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
		if (itQuadTree->second->status() != QuadrantStatus::uninitialized)
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
	if (m_cameraQuadTree)
		return m_cameraQuadTree->getQuadrant()->getPos().getName().c_str();
	else
		return "";
}

void GDN_TheWorld_Viewer::refreshQuadTreeStatistics()
{
	std::lock_guard lock(m_mtxQuadTree);

	int numSplits = 0;
	int numJoins = 0;
	int numChunks = 0;
	int numActiveChunks = 0;
	int numQuadrant = (int)m_mapQuadTree.size();
	int numinitializedQuadrant = 0;
	int numVisibleQuadrant = 0;
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
	}

	m_numSplits = numSplits;
	m_numJoins = numJoins;
	m_numChunks = numChunks;
	m_numActiveChunks = numActiveChunks;
	m_numQuadrant = numQuadrant;
	m_numinitializedQuadrant = numinitializedQuadrant;
	m_numVisibleQuadrant = numVisibleQuadrant;
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

int GDN_TheWorld_Viewer::getNuminitializedVisibleQuadrant(void)
{
	return m_numinitializedVisibleQuadrant;
}

int GDN_TheWorld_Viewer::getProcessDuration(void)
{
	return m_averageProcessDuration;
}

void GDN_TheWorld_Viewer::dump()
{
	Globals()->debugPrint("*************");
	Globals()->debugPrint("STARTING DUMP");

	float f = Engine::get_singleton()->get_frames_per_second();
	Globals()->debugPrint("FPS: " + String(std::to_string(f).c_str()));

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
	
	std::lock_guard lock(m_mtxQuadTree);

	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		itQuadTree->second->dump();

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
	m_cameraQuadTree = quadTree;
}

void GDN_TheWorld_Viewer::streamer(void)
{
	m_streamerThreadRunning = true;

	while (!m_streamerThreadRequiredExit)
	{
		try
		{
			//std::lock_guard lock(m_mtxQuadTree);
			std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTree, std::try_to_lock);
			if (lock.owns_lock())
			{
				try
				{
					if (m_computedCameraQuadrantPos.isInitialized())
					{
						bool exitForNow = false;
						for (size_t distance = 0; distance <= m_numCacheQuadrantOnPerimeter; distance++)
						{
							for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
							{
								if (itQuadTree->second->status() == QuadrantStatus::uninitialized)
								{
									size_t distanceInPerimeterFromCameraQuadrant = itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(m_computedCameraQuadrantPos);
									if (distanceInPerimeterFromCameraQuadrant == distance)
									{
										float x = 0, z = 0;
										itQuadTree->second->init(x, z);
										exitForNow = true;
										break;
									}
								}
							}

							if (exitForNow)
								break;
						}
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

Transform GDN_TheWorld_Viewer::getInternalGlobalTransform(void)
{
	Transform gt = get_global_transform();
	return gt;
}
