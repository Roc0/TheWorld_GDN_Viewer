//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Camera.h"
#include "GDN_TheWorld_Edit.h"

#pragma warning(push, 0)
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/config_file.hpp>
#pragma warning(pop)

#include <filesystem>
#include <limits>

#include "MeshCache.h"
#include "QuadTree.h"
#include "Profiler.h"
#include "FastNoiseLite.h"
#include "Tools.h"

#include <algorithm>

using namespace godot;
//using namespace std;

namespace fs = std::filesystem;

// TODO
// . passare allo shader (nuova texture ?):
//		- il numero dei vertici di un quad (u_num_vertices_per_chunk_side=32)
//		- le 4 altezze nelle 4 direzioni adiacenti per: 
//			- calcolare le normali dinamicamente se la normal map presenta un vettore nullo
//			- eseguire lo streching verso l'esterno dei vertici sul perimetro in modo da sovrapporsi con i chunk adiacenti ed evitare l'effetto "Fireflies along seams" --SE CI LIMITIAMO A QUESTO BASTA UNA SOLA DIREZIONE PER I VERTICI SUL BORDO(NEI 4 ANGOLI PREVALE UNO DEI DUE BORDI)--
// . nello shader calcolare il LOD in base alla distanza dalla camera
// . in base al LOD corrente verificare se il VERTEX � sul perimetro del chunk: si verifica se le coordinate sono 0 o vicine ad esso oppure se sono pari allo step in WU (u_grid_step) * il numero di vertici della grid 
//		che separano vertici aidacenti della mesh al lod corrente (da calcolare in base al lod) per il numero dei vertici di una mesh (u_num_vertices_per_chunk_side)
// . eseguire lo streching verso l'esterno dei vertici sul perimetro in modo da sovrapporsi con i chunk adiacenti ed evitare l'effetto "Fireflies along seams" --SE CI LIMITIAMO A QUESTO BASTA UNA SOLA DIREZIONE PER I VERTICI SUL BORDO(NEI 4 ANGOLI PREVALE UNO DEI DUE BORDI)--
//		la quantit� dello stretching dipende dal lod corrente (maggiore per lod minori ovvero per risoluzioni maggiori)
// . calcolare le normali tramite uno shader applicato ad uno sprite 2D per sfruttare le capacit� di calcolo della GPU (vedi: normalmap_baker.gd e bump2normal_tex.shader)
// TODO

// World Node Local Coordinate System is the same as MapManager coordinate system
// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
// Chunk and QuadTree coordinates are in Viewer Node local coordinate System

void GDN_TheWorld_Viewer::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("debug_print", "p_msg"), &GDN_TheWorld_Viewer::debugPrint);
	ClassDB::bind_method(D_METHOD("error_print", "p_msg"), &GDN_TheWorld_Viewer::errorPrint);
	ClassDB::bind_method(D_METHOD("warning_print", "p_msg"), &GDN_TheWorld_Viewer::warningPrint);
	ClassDB::bind_method(D_METHOD("info_print", "p_msg"), &GDN_TheWorld_Viewer::infoPrint);
	ClassDB::bind_method(D_METHOD("print", "p_msg"), &GDN_TheWorld_Viewer::print);

	ClassDB::bind_method(D_METHOD("set_editor_interface"), &GDN_TheWorld_Viewer::setEditorInterface);
	ClassDB::bind_method(D_METHOD("set_editor_3d_overlay"), &GDN_TheWorld_Viewer::setEditor3dOverlay);
	ClassDB::bind_method(D_METHOD("set_editor_camera"), &GDN_TheWorld_Viewer::setEditorCamera);
	ClassDB::bind_method(D_METHOD("get_camera"), &GDN_TheWorld_Viewer::getCamera);
	ClassDB::bind_method(D_METHOD("get_info_camera"), &GDN_TheWorld_Viewer::getInfoCamera);
	ClassDB::bind_method(D_METHOD("get_camera_projection_mode"), &GDN_TheWorld_Viewer::getCameraProjectionMode);
	ClassDB::bind_method(D_METHOD("get_or_create_edit_mode_ui_control"), &GDN_TheWorld_Viewer::getOrCreateEditModeUIControl);
	ClassDB::bind_method(D_METHOD("toggle_track_mouse"), &GDN_TheWorld_Viewer::toggleTrackMouse);
	ClassDB::bind_method(D_METHOD("get_track_mouse_state"), &GDN_TheWorld_Viewer::getTrackMouseState);
	ClassDB::bind_method(D_METHOD("toggle_edit_mode"), &GDN_TheWorld_Viewer::toggleEditMode);
	ClassDB::bind_method(D_METHOD("toggle_debug_visibility"), &GDN_TheWorld_Viewer::toggleDebugVisibility);
	ClassDB::bind_method(D_METHOD("rotate_chunk_debug_mode"), &GDN_TheWorld_Viewer::rotateChunkDebugMode);
	ClassDB::bind_method(D_METHOD("rotate_drawing_mode"), &GDN_TheWorld_Viewer::rotateDrawingMode);
	ClassDB::bind_method(D_METHOD("toggle_quadrant_selected"), &GDN_TheWorld_Viewer::toggleQuadrantSelected);
	ClassDB::bind_method(D_METHOD("set_depth_quad"), &GDN_TheWorld_Viewer::setDepthQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("get_depth_quad"), &GDN_TheWorld_Viewer::getDepthQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("set_cache_quad"), &GDN_TheWorld_Viewer::setCacheQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("get_cache_quad"), &GDN_TheWorld_Viewer::getCacheQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("deploy_world"), &GDN_TheWorld_Viewer::deployWorld);
	ClassDB::bind_method(D_METHOD("undeploy_world"), &GDN_TheWorld_Viewer::undeployWorld);
	ClassDB::bind_method(D_METHOD("initial_world_viewer_pos_set"), &GDN_TheWorld_Viewer::initialWordlViewerPosSet);
	ClassDB::bind_method(D_METHOD("dump_required"), &GDN_TheWorld_Viewer::setDumpRequired);
	//ClassDB::bind_method(D_METHOD("get_camera_chunk_global_transform_of_aabb"), &GDN_TheWorld_Viewer::getCameraChunkGlobalTransformOfAABB);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_local_aabb"), &GDN_TheWorld_Viewer::getCameraChunkLocalAABB);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_local_debug_aabb"), &GDN_TheWorld_Viewer::getCameraChunkLocalDebugAABB);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_global_transform_applied"), &GDN_TheWorld_Viewer::getCameraChunkGlobalTransformApplied);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_debug_global_transform_applied"), &GDN_TheWorld_Viewer::getCameraChunkDebugGlobalTransformApplied);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_id"), &GDN_TheWorld_Viewer::getCameraChunkId);
	ClassDB::bind_method(D_METHOD("get_camera_quadrant_name"), &GDN_TheWorld_Viewer::getCameraQuadrantName);
	ClassDB::bind_method(D_METHOD("get_num_splits"), &GDN_TheWorld_Viewer::getNumSplits);
	ClassDB::bind_method(D_METHOD("get_num_joins"), &GDN_TheWorld_Viewer::getNumJoins);
	ClassDB::bind_method(D_METHOD("get_num_chunks"), &GDN_TheWorld_Viewer::getNumChunks);
	ClassDB::bind_method(D_METHOD("get_num_active_chunks"), &GDN_TheWorld_Viewer::getNumActiveChunks);
	ClassDB::bind_method(D_METHOD("get_num_quadrant"), &GDN_TheWorld_Viewer::getNumQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_initialized_quadrant"), &GDN_TheWorld_Viewer::getNuminitializedQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_visible_quadrant"), &GDN_TheWorld_Viewer::getNumVisibleQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_initialized_visible_quadrant"), &GDN_TheWorld_Viewer::getNuminitializedVisibleQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_empty_quadrant"), &GDN_TheWorld_Viewer::getNumEmptyQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_flushed_quadrant"), &GDN_TheWorld_Viewer::getNumFlushedQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_process_not_owns_lock"), &GDN_TheWorld_Viewer::getProcessNotOwnsLock);
	ClassDB::bind_method(D_METHOD("get_process_duration"), &GDN_TheWorld_Viewer::getProcessDuration);
	ClassDB::bind_method(D_METHOD("get_refresh_quads_duration"), &GDN_TheWorld_Viewer::getRefreshMapQuadDuration);
	ClassDB::bind_method(D_METHOD("get_update_quads1_duration"), &GDN_TheWorld_Viewer::getUpdateQuads1Duration);
	ClassDB::bind_method(D_METHOD("get_update_quads2_duration"), &GDN_TheWorld_Viewer::getUpdateQuads2Duration);
	ClassDB::bind_method(D_METHOD("get_update_quads3_duration"), &GDN_TheWorld_Viewer::getUpdateQuads3Duration);
	ClassDB::bind_method(D_METHOD("get_update_chunks_duration"), &GDN_TheWorld_Viewer::getUpdateChunksDuration);
	ClassDB::bind_method(D_METHOD("get_update_material_params_duration"), &GDN_TheWorld_Viewer::getUpdateMaterialParamsDuration);
	ClassDB::bind_method(D_METHOD("get_mouse_track_hit_duration"), &GDN_TheWorld_Viewer::getMouseTrackHitDuration);
	ClassDB::bind_method(D_METHOD("get_debug_draw_mode"), &GDN_TheWorld_Viewer::getDebugDrawMode);
	ClassDB::bind_method(D_METHOD("get_chunk_debug_mode"), &GDN_TheWorld_Viewer::getChunkDebugModeStr);
	ClassDB::bind_method(D_METHOD("get_mouse_hit"), &GDN_TheWorld_Viewer::getMouseHit);
	ClassDB::bind_method(D_METHOD("get_mouse_hit_distance_from_camera"), &GDN_TheWorld_Viewer::getMouseHitDistanceFromCamera);
	ClassDB::bind_method(D_METHOD("get_mouse_quadrant_hit_name"), &GDN_TheWorld_Viewer::_getMouseQuadrantHitName);
	ClassDB::bind_method(D_METHOD("get_mouse_quadrant_hit_tag"), &GDN_TheWorld_Viewer::_getMouseQuadrantHitTag);
	ClassDB::bind_method(D_METHOD("get_mouse_quadrant_hit_pos"), &GDN_TheWorld_Viewer::getMouseQuadrantHitPos);
	ClassDB::bind_method(D_METHOD("get_mouse_quadrant_hit_size"), &GDN_TheWorld_Viewer::getMouseQuadrantHitSize);
	ClassDB::bind_method(D_METHOD("get_mouse_chunk_hit_name"), &GDN_TheWorld_Viewer::getMouseChunkHitId);
	ClassDB::bind_method(D_METHOD("get_mouse_chunk_hit_pos"), &GDN_TheWorld_Viewer::getMouseChunkHitPos);
	ClassDB::bind_method(D_METHOD("get_mouse_chunk_hit_size"), &GDN_TheWorld_Viewer::getMouseChunkHitSize);
	ClassDB::bind_method(D_METHOD("get_mouse_chunk_hit_dist_from_cam"), &GDN_TheWorld_Viewer::getMouseChunkHitDistFromCam);
	ClassDB::bind_method(D_METHOD("update_material_parmas_for_every_quadrant"), &GDN_TheWorld_Viewer::updateMaterialParamsForEveryQuadrant);
	ClassDB::bind_method(D_METHOD("set_shader_parameter"), &GDN_TheWorld_Viewer::setShaderParam);
	ClassDB::bind_method(D_METHOD("get_viewport_size"), &GDN_TheWorld_Viewer::getViewportSize);
	ClassDB::bind_method(D_METHOD("viewport_size_changed"), &GDN_TheWorld_Viewer::viewportSizeChanged);

	ADD_SIGNAL(MethodInfo("tw_quadrant_selected_for_edit", PropertyInfo(Variant::BOOL, "value")));
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_initialized = false;
	m_quitting = false;
	m_visibleInTree = false;
	m_initRequired = false;
	m_isInEditor = false;
	m_isInEditorInitialized = false;
	m_ready = false;
	m_desiderdLookDev = ShaderTerrainData::LookDev::NotSet;
	m_desideredLookDevChanged = false;
	m_stopDeploy = false;
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
	m_timeElapsedFromLastMouseTrack = 0;
	m_mouseQuadrantHitSize = 0;
	m_mouseHitChunk = nullptr;
	m_mouseTrackedOnTerrain = false;
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
	m_editorInterface = nullptr;
	m_editor3dOverlay = nullptr;
	m_editorCamera = nullptr;
	m_depthQuadOnPerimeter = 3;
	m_cacheQuadOnPerimeter = 1;
	m_shaderParam_depthBlending = false;
	m_shaderParam_triplanar = false;
	m_shaderParam_globalmapBlendStart = 0.0f;
	m_shaderParam_globalmapBlendDistance = 0.0f;
	m_shaderParamChanged = false;

	_init();
}

GDN_TheWorld_Viewer::~GDN_TheWorld_Viewer()
{
	deinit();
}

void GDN_TheWorld_Viewer::setShaderParam(godot::String name, godot::Variant value)
{
	if (name == "ground_uv_scale")
	{
		m_shaderParam_groundUVScale.set_normal(godot::Vector3(value, value, value));
		m_shaderParam_groundUVScale.d = value;
		m_shaderParamChanged = true;
	}
	else if (name == "depth_blending")
	{
		m_shaderParam_depthBlending = value;
		m_shaderParamChanged = true;
	}
	else if (name == "triplanar")
	{
		m_shaderParam_triplanar = value;
		m_shaderParamChanged = true;
	}
	else if (name == "tile_reduction")
	{
		bool tileReduction = value;
		if (tileReduction)
		{
			m_shaderParam_tileReduction.set_normal(godot::Vector3(1.0f, 1.0f, 1.0f));
			m_shaderParam_tileReduction.d = 1.0f;
		}
		else
		{
			m_shaderParam_tileReduction.set_normal(godot::Vector3(0.0f, 0.0f, 0.0f));
			m_shaderParam_tileReduction.d = 0.0f;
		}
		m_shaderParamChanged = true;
	}
	else if (name == "globalmap_blend_start")
	{
		m_shaderParam_globalmapBlendStart = value;
		m_shaderParamChanged = true;
	}
	else if (name == "globalmap_blend_distance")
	{
		m_shaderParam_globalmapBlendDistance = value;
		m_shaderParamChanged = true;
	}
	else if (name == "colormap_opacity")
	{
		m_shaderParam_colormapOpacity.set_normal(godot::Vector3(value, value, value));
		m_shaderParam_colormapOpacity.d = value;
		m_shaderParamChanged = true;
	}
}

//Ref<Material> GDN_TheWorld_Viewer::getRegularMaterial(void)
//{
//	if (m_regularMaterial == nullptr)
//	{
//		getMainProcessingMutex().lock();
//		Ref<ShaderMaterial> mat = ShaderMaterial::_new();
//		getMainProcessingMutex().unlock();
//		ResourceLoader* resLoader = ResourceLoader::get_singleton();
//		//String shaderPath = "res://addons/twviewer/shaders/regularChunk.shader";
//		String shaderPath = "res://addons/twviewer/shaders/simple4.shader";
//		Ref<Shader> shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
//		mat->set_shader(shader);
//		m_regularMaterial = mat;
//	}
//
//	return m_regularMaterial;
//}

//Ref<Material> GDN_TheWorld_Viewer::getLookDevMaterial(void)
//{
//	if (m_lookDevMaterial == nullptr)
//	{
//		getMainProcessingMutex().lock();
//		Ref<ShaderMaterial> mat = ShaderMaterial::_new();
//		getMainProcessingMutex().unlock();
//		ResourceLoader* resLoader = ResourceLoader::get_singleton();
//		String shaderPath = "res://addons/twviewer/shaders/lookdevChunk.shader";
//		Ref<Shader> shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
//		mat->set_shader(shader);
//		m_lookDevMaterial = mat;
//	}
//
//	return m_lookDevMaterial;
//}

bool GDN_TheWorld_Viewer::init(void)
{
	PLOG_INFO << "TheWorld Viewer Initializing...";

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return false;

	globals->addElementToInitialize();
	
	//m_isInEditor = IS_EDITOR_HINT();
	
	// From _notification ENTER_WORLD
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

	// From _ready
	{
		if (IS_EDITOR_HINT())
		{
			m_depthQuadOnPerimeter = 1;
			m_cacheQuadOnPerimeter = 1;
		}
		else
		{
			m_depthQuadOnPerimeter = 3;
			m_cacheQuadOnPerimeter = 1;
		}

		m_shaderParam_groundUVScale.set_normal(godot::Vector3(0.0f, 0.0f, 0.0f));	// if every coord is 0.0 the parameter is not passed to the shader
		m_shaderParam_groundUVScale.d = 0.0f;
		m_shaderParam_tileReduction.set_normal(godot::Vector3(0.0f, 0.0f, 0.0f));
		m_shaderParam_tileReduction.d = 0.0f;
		m_shaderParam_colormapOpacity.set_normal(godot::Vector3(1.0f, 1.0f, 1.0f));
		m_shaderParam_colormapOpacity.d = 1.0f;

		set_notify_transform(true);

		//if (globals->status() != TheWorldStatus::sessionInitialized)
		//	return;

		//get_tree()->get_root()->connect("size_changed", this, "viewport_resizing");

		//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");

		// Camera stuff
		//if (!IS_EDITOR_HINT())
		{
			//if (WorldCamera())
			//{
			//	WorldCamera()->deactivateCamera();
			//	WorldCamera()->queue_free();
			//}
			//assignWorldCamera(GDN_TheWorld_Camera::_new());

			GDN_TheWorld_Camera* camera = CameraNode(false);
			if (camera == nullptr)
				camera = memnew(GDN_TheWorld_Camera);
			camera->set_name(THEWORLD_CAMERA_NODE_NAME);
			assignWorldCamera(camera);

			SceneTree* scene = get_tree();
			Node* sceneRoot = nullptr;
			if (IS_EDITOR_HINT())
				sceneRoot = scene->get_edited_scene_root();
			else
				sceneRoot = scene->get_root();

			int64_t worldMainNodeId = getWorldNode()->get_instance_id();

			Node* parent = camera->get_parent();
			if (parent != nullptr)
			{
				int64_t parentId = parent->get_instance_id();
				if (parentId != worldMainNodeId)
				{
					parent->remove_child(camera);
					//getWorldNode()->call_deferred("add_child", camera);
					getWorldNode()->add_child(camera);
				}
			}
			else
			{
				//getWorldNode()->call_deferred("add_child", camera);	// with deferred we risk that we call get_tree in Globals() (activateCamera) before camera is added to the scene 
				getWorldNode()->add_child(camera);
			}

			camera->call_deferred("set_owner", sceneRoot);

			godot::Control* control = getOrCreateEditModeUIControl();
		}
	}

	m_meshCache = make_unique<MeshCache>(this);
	m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());

	m_streamerThreadRequiredExit = false;
	m_streamerThreadRunning = true;
	m_streamerThread = std::thread(&GDN_TheWorld_Viewer::streamer, this);

	ShaderTerrainData::ShaderTerrainData::getGroundTextures(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()), this);

	std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>>& groundTextures = ShaderTerrainData::getGroundTextures();
	globals->addElementToInitialize((int)groundTextures.size());
	
	m_normalDrawer = memnew(GDN_TheWorld_Drawer);
	this->add_child(m_normalDrawer);
	const float radius = 1;
	m_normalLineIdx = m_normalDrawer->addLine(godot::Vector3(), godot::Vector3(), "", false, true, GDN_TheWorld_Drawer::Drawing::LabelPos::end, 15.0f, godot::Vector2i(-10, -25), GDN_TheWorld_Globals::g_color_black);
	m_normalSphereIdx = m_normalDrawer->addSphere(godot::Vector3(), radius, GDN_TheWorld_Globals::g_color_cyan);
	m_normalDrawer->set_visible(false);

	m_positionDrawer = memnew(GDN_TheWorld_Drawer);
	this->add_child(m_positionDrawer);
	m_positionLabelXIdx = m_positionDrawer->addLabel2d(godot::Vector3(), godot::Vector2i(-10, 20), "", GDN_TheWorld_Globals::g_color_white);
	m_positionLabelZIdx = m_positionDrawer->addLabel2d(godot::Vector3(), godot::Vector2i(-10, 40), "", GDN_TheWorld_Globals::g_color_white);
	m_positionLabelYIdx = m_positionDrawer->addLabel2d(godot::Vector3(), godot::Vector2i(-10, 60), "", GDN_TheWorld_Globals::g_color_white);
	m_positionLineIdx = m_positionDrawer->addLine(godot::Vector3(), godot::Vector3(), "", false, true, GDN_TheWorld_Drawer::Drawing::LabelPos::center, -1.0f, godot::Vector2i(-10, -10), GDN_TheWorld_Globals::g_color_black);
	m_positionSphereStartIdx = m_positionDrawer->addSphere(godot::Vector3(), 5, GDN_TheWorld_Globals::g_color_cyan);
	m_positionSphereEndIdx = m_positionDrawer->addSphere(godot::Vector3(), 5, GDN_TheWorld_Globals::g_color_cyan);

	m_positionDrawer->set_visible(false);

	m_gizmo = memnew(GDN_TheWorld_Gizmo3d);
	this->add_child(m_gizmo);
	m_gizmo->set_visible(false);
	m_gizmo->set_viewer(this);

	globals->addNumElementInitialized();

	m_initialized = true;
	m_initRequired = false;
	PLOG_INFO << "TheWorld Viewer Initialized!";

	return true;
}

void GDN_TheWorld_Viewer::preDeinit(void)
{
	enum class TheWorldStatus status = Globals()->status();
	if ((int)status >= (int)TheWorldStatus::worldDeployInProgress)
	{
		while ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
		{
			status = Globals()->status();
			if ((int)status < (int)TheWorldStatus::worldUnDeployInProgress)
				undeployWorld();

			status = Globals()->status();
			if (status == TheWorldStatus::worldDeployInProgress)
				Globals()->setStatus(TheWorldStatus::worldDeployed);

			Sleep(5);
		}
	}

	m_streamerThreadRequiredExit = true;
}

bool GDN_TheWorld_Viewer::canDeinit(void)
{
	return !m_streamerThreadRunning;
}

void GDN_TheWorld_Viewer::deinit(void)
{
	if (m_initialized)
	{
		PLOG_INFO << "TheWorld Viewer Deinitializing...";

		// from _notification EXIT_WORLD
		GDN_TheWorld_Globals* globals = Globals();
		if (globals != nullptr)
			globals->debugPrint("Exit world");

		if (m_normalDrawer != nullptr)
		{
			Node* parent = m_normalDrawer->get_parent();
			if (parent != nullptr)
				parent->remove_child(m_normalDrawer);
			m_normalDrawer->queue_free();
			m_normalDrawer = nullptr;
		}
		
		if (m_positionDrawer != nullptr)
		{
			Node* parent = m_positionDrawer->get_parent();
			if (parent != nullptr)
				parent->remove_child(m_positionDrawer);
			m_positionDrawer->queue_free();
			m_positionDrawer = nullptr;
		}

		if (m_gizmo != nullptr)
		{
			Node* parent = m_gizmo->get_parent();
			if (parent != nullptr)
				parent->remove_child(m_gizmo);
			m_gizmo->queue_free();
			m_gizmo = nullptr;
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

			Chunk::ExitWorldChunkAction action;
			itQuadTree->second->ForAllChunk(action);

			itQuadTree->second->getQuadrant()->getCollider()->exitWorld();
			//itQuadTree->second->getQuadrant()->getCollider()->onGlobalTransformChanged();
		}
		//}

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
		clearMapQuadTree();
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
			
			Node* parent = editModeUIControl->get_parent();
			if (parent != nullptr)
				parent->call_deferred("remove_child", editModeUIControl);
			editModeUIControl->queue_free();
			m_editModeUIControl = nullptr;
		}

		GDN_TheWorld_Camera* camera = CameraNode(true);
		if (camera != nullptr)
		{
			//Node* parent = camera->get_parent();
			//if (parent != nullptr)									// TESTING
			//	parent->call_deferred("remove_child", camera);			// TESTING
			//camera->queue_free();										// TESTING
		}
		
		ShaderTerrainData::releaseGlobals();

		m_initialized = false;
		PLOG_INFO << "TheWorld Viewer Deinitialized!";
		
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

			enum class TheWorldStatus status = Globals()->status();
			if (status == TheWorldStatus::worldUnDeployInProgress || status < TheWorldStatus::worldDeployInProgress)
			{
				GDN_TheWorld_Globals* globals = Globals();
				if (globals != nullptr)
				{
					std::string msg = std::string("GDN_TheWorld_Viewer::replyFromServer - ") + THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES + " - Status " + std::to_string(int(status)) + ": cannot proceed deploying quadrant";
					globals->debugPrint(msg.c_str());
				}
			}
			else
			{
				//TheWorld_Viewer_Utils::TimerMs clock("GDN_TheWorld_Viewer::replyFromServer", "CLIENT SIDE", false, true);

				if (reply.getNumReplyParams() < 3)
				{
					std::string m = std::string("Reply MapManager::getVertices error (not enough params replied)");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}

				ClientServerVariant v = reply.getReplyParam(0);
				const auto _cameraPosX(std::get_if<float>(&v));
				if (_cameraPosX == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not return a float as first return param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraPosX = *_cameraPosX;

				v = reply.getReplyParam(1);
				const auto _cameraPosZ(std::get_if<float>(&v));
				if (_cameraPosZ == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not return a float as second return param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraPosZ = *_cameraPosZ;

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
				const auto _cameraDistanceFromTerrainForced(std::get_if<float>(&v));
				if (_cameraDistanceFromTerrainForced == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as tenth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraDistanceFromTerrainForced = *_cameraDistanceFromTerrainForced;

				v = reply.getInputParam(10);
				const auto _cameraPosY(std::get_if<float>(&v));
				if (_cameraPosY == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as eleventh input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraPosY = *_cameraPosY;

				v = reply.getInputParam(11);
				const auto _cameraYaw(std::get_if<float>(&v));
				if (_cameraYaw == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as twelveth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraYaw = *_cameraYaw;

				v = reply.getInputParam(12);
				const auto _cameraPitch(std::get_if<float>(&v));
				if (_cameraPitch == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as thirteenth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraPitch = *_cameraPitch;

				v = reply.getInputParam(13);
				const auto _cameraRoll(std::get_if<float>(&v));
				if (_cameraRoll == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as fourteenth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraRoll = *_cameraRoll;

				QuadrantPos quadrantPos(lowerXGridVertex, lowerZGridVertex, level, numVerticesPerSize, gridStepInWU);

				m_mtxQuadTreeAndMainProcessing.lock();
				if (m_mapQuadTree.contains(quadrantPos) && !m_mapQuadTree[quadrantPos]->statusToErase())
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1 ") + __FUNCTION__, "Elab. quadrant");
					QuadTree* quadTree = m_mapQuadTree[quadrantPos].get();
					m_mtxQuadTreeAndMainProcessing.unlock();

					if (m_stopDeploy)
					{
						//quadTree->setStatus(QuadrantStatus::initialized);
						quadTree->setStatus(QuadrantStatus::initializationStopped);
					}
					else
					{
						if (quadTree->statusGetTerrainDataInProgress() || quadTree->statusRefreshTerrainDataInProgress())
						{
							std::recursive_mutex& quadrantMutex = quadTree->getQuadrantMutex();
							std::lock_guard<std::recursive_mutex> lock(quadrantMutex);

							std::string meshIdFromBuffer;
							{
								// ATTENZIONE
								//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);	// SUPERDEBUGRIC : to remove when the mock for altitudes is removed from cache.refreshMeshCacheFromBuffer
								TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1 ") + __FUNCTION__, "quadTree->getQuadrant()->refreshGridVertices");
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

								size_t cameraInGridIndex = quadTree->getQuadrant()->getIndexFromMap(cameraPosX, cameraPosZ);
								if (cameraInGridIndex == -1)
									throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Not found WorldViewer Pos").c_str()));

								float cameraHeight = quadTree->getQuadrant()->getAltitudeFromHeigthmap(cameraInGridIndex);

								Vector3 cameraPos(cameraPosX, cameraPosY, cameraPosZ);		// MapManager coordinates are local coordinates of WorldNode
								if (cameraDistanceFromTerrainForced != 0.0f)
									cameraPos.y = cameraHeight + cameraDistanceFromTerrainForced;
								float offset = cameraDistanceFromTerrainForced == 0.0f ? 300 : cameraDistanceFromTerrainForced;
								//Vector3 lookAt(cameraPosX + offset, cameraHeight, cameraPosZ + offset);
								Vector3 lookAt(cameraPosX, cameraHeight, cameraPosZ - abs(offset));		// camera look at terrain along north direction

								// Viewer stuff: set viewer position relative to world node at the first point of the bitmap and altitude 0 so that that point is at position (0,0,0) respect to the viewer
								//Transform t = get_transform();
								//t.origin = Vector3(quadTree->getQuadrant()->getGridVertices()[0].posX(), 0, quadTree->getQuadrant()->getGridVertices()[0].posZ());
								//set_transform(t);

								//WorldCamera()->initCameraInWorld(cameraPos, lookAt, cameraYaw, cameraPitch, cameraRoll);
								WorldCamera()->call_deferred("init_camera_in_world", cameraPos, lookAt, cameraYaw, cameraPitch, cameraRoll);

								m_initialWordlViewerPosSet = true;
							}

							quadTree->setStatus(QuadrantStatus::initialized);
						}
					}
				}
				else
					m_mtxQuadTreeAndMainProcessing.unlock();
			}
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER)
		{
			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			if (editModeUIControl != nullptr && editModeUIControl->initilized())
				editModeUIControl->replyFromServer(reply);
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 0.2 ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL);

			ClientServerVariant v = reply.getInputParam(0);
			const auto _level(std::get_if<int>(&v));
			if (_level == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a int as first input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int level = *_level;

			v = reply.getInputParam(1);
			const auto _numVerticesPerSize(std::get_if<size_t>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a size_t as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			size_t numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(2);
			const auto _gridStepInWU(std::get_if<float>(&v));
			if (_gridStepInWU == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepInWU;

			v = reply.getInputParam(3);
			const auto _setCamera(std::get_if<bool>(&v));
			if (_setCamera == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a bool as forth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			bool setCamera = *_setCamera;

			v = reply.getInputParam(4);
			const auto _cameraPosX(std::get_if<float>(&v));
			if (_cameraPosX == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as fifth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosX = *_cameraPosX;

			v = reply.getInputParam(5);
			const auto _cameraPosY(std::get_if<float>(&v));
			if (_cameraPosY == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as sixth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosY = *_cameraPosY;

			v = reply.getInputParam(6);
			const auto _cameraPosZ(std::get_if<float>(&v));
			if (_cameraPosZ == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as seventh input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosZ = *_cameraPosZ;

			v = reply.getInputParam(7);
			const auto _cameraDistanceFromTerrainForced(std::get_if<float>(&v));
			if (_cameraDistanceFromTerrainForced == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as eight input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraDistanceFromTerrainForced = *_cameraDistanceFromTerrainForced;

			v = reply.getInputParam(8);
			const auto _cameraYaw(std::get_if<float>(&v));
			if (_cameraYaw == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as ninth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraYaw = *_cameraYaw;

			v = reply.getInputParam(9);
			const auto _cameraPitch(std::get_if<float>(&v));
			if (_cameraPitch == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as tenth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPitch = *_cameraPitch;

			v = reply.getInputParam(10);
			const auto _cameraRoll(std::get_if<float>(&v));
			if (_cameraRoll == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as eleventh input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraRoll = *_cameraRoll;

			QuadrantPos quadrantPos(cameraPosX, cameraPosZ, level, (int)numVerticesPerSize, gridStepInWU);
			quadrantPos.setTag("Camera");

			std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

			clearMapQuadTree();
			m_mapQuadTree[quadrantPos] = make_unique<QuadTree>(this, quadrantPos);
			m_mapQuadTree[quadrantPos]->init(cameraPosX, cameraPosY, cameraPosZ, cameraYaw, cameraPitch, cameraRoll, true, cameraDistanceFromTerrainForced);
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 0.1 ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD);

			ClientServerVariant v = reply.getInputParam(0);
			const auto _level(std::get_if<int>(&v));
			if (_level == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a int as first input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int level = *_level;

			v = reply.getInputParam(1);
			const auto _numVerticesPerSize(std::get_if<size_t>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a size_t as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			size_t numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(2);
			const auto _gridStepInWU(std::get_if<float>(&v));
			if (_gridStepInWU == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepInWU;

			v = reply.getInputParam(3);
			const auto _setCamera(std::get_if<bool>(&v));
			if (_setCamera == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a bool as forth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			bool setCamera = *_setCamera;

			v = reply.getInputParam(4);
			const auto _cameraPosX(std::get_if<float>(&v));
			if (_cameraPosX == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as fifth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosX = *_cameraPosX;

			v = reply.getInputParam(5);
			const auto _cameraPosY(std::get_if<float>(&v));
			if (_cameraPosY == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as sixth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosY = *_cameraPosY;

			v = reply.getInputParam(6);
			const auto _cameraPosZ(std::get_if<float>(&v));
			if (_cameraPosZ == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as seventh input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosZ = *_cameraPosZ;

			v = reply.getInputParam(7);
			const auto _cameraDistanceFromTerrainForced(std::get_if<float>(&v));
			if (_cameraDistanceFromTerrainForced == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as eight input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraDistanceFromTerrainForced = *_cameraDistanceFromTerrainForced;

			v = reply.getInputParam(8);
			const auto _cameraYaw(std::get_if<float>(&v));
			if (_cameraYaw == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as ninth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraYaw = *_cameraYaw;

			v = reply.getInputParam(9);
			const auto _cameraPitch(std::get_if<float>(&v));
			if (_cameraPitch == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as tenth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPitch = *_cameraPitch;

			v = reply.getInputParam(10);
			const auto _cameraRoll(std::get_if<float>(&v));
			if (_cameraRoll == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as eleventh input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraRoll = *_cameraRoll;

			Globals()->Client()->MapManagerDeployLevel(isInEditor(), level, numVerticesPerSize, gridStepInWU, setCamera, cameraPosX, cameraPosY, cameraPosZ, cameraDistanceFromTerrainForced, cameraYaw, cameraPitch, cameraRoll);
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD)
		{
			{
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
			}

			enum class TheWorldStatus status = Globals()->status();

			while (Globals()->Client()->getNumCallbacksRunning() > 1)
			{
				Sleep(1);
			}

			while (m_editModeUIControl != nullptr && m_editModeUIControl->getNumWorkingThread() > 0)
			{
				m_editModeUIControl->requestedQuit();
				Sleep(1);
			}

			{
				std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing); 
				clearMapQuadTree();
			}

			if (Globals()->connectedToServer() && m_initialized)
				Globals()->setStatus(TheWorldStatus::sessionInitialized);
			else if (Globals()->connectedToServer())
				Globals()->setStatus(TheWorldStatus::connectedToServer);
			else if (Globals()->initialized())
				Globals()->setStatus(TheWorldStatus::initialized);
			else
				Globals()->setStatus(TheWorldStatus::uninitialized);

		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC)
		{
			ClientServerVariant v = reply.getInputParam(1);
			const auto _numVerticesPerSize(std::get_if<size_t>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a size_t as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			size_t numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(2);
			const auto _gridStepinWU(std::get_if<float>(&v));
			if (_gridStepinWU == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepinWU = *_gridStepinWU;

			v = reply.getReplyParam(0);
			const auto _foundUpdatedQuadrant(std::get_if<bool>(&v));
			if (_foundUpdatedQuadrant == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not return a bool as first return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			bool foundUpdatedQuadrant = *_foundUpdatedQuadrant;

			if (foundUpdatedQuadrant)
			{
				v = reply.getReplyParam(1);
				const auto _level(std::get_if<int>(&v));
				if (_level == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not return a int as second return param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				int level = *_level;

				v = reply.getReplyParam(2);
				const auto _lowerXGridVertex(std::get_if<float>(&v));
				if (_lowerXGridVertex == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not return a float as third return param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float lowerXGridVertex = *_lowerXGridVertex;

				v = reply.getReplyParam(3);
				const auto _lowerZGridVertex(std::get_if<float>(&v));
				if (_lowerZGridVertex == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not return a float as forth return param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float lowerZGridVertex = *_lowerZGridVertex;

				std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

				for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
				{
					if (!itQuadTree->second->isValid())
						continue;

					//if (!itQuadTree->second->isVisible())
					//	continue;

					QuadrantPos pos = itQuadTree->second->getQuadrant()->getPos();
					if (pos.isInitialized()
						&& pos.getNumVerticesPerSize() == numVerticesPerSize
						&& pos.getGridStepInWU() == gridStepinWU
						&& pos.getLevel() == level
						&& pos.getLowerXGridVertex() == lowerXGridVertex
						&& pos.getLowerZGridVertex() == lowerZGridVertex)
					{
						itQuadTree->second->setStatus(QuadrantStatus::refreshTerrainDataNeeded);
						break;
					}
				}
			}
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
		godot::RenderingServer::get_singleton()->set_debug_generate_wireframes(true);

	set_name(THEWORLD_VIEWER_NODE_NAME);
}

void GDN_TheWorld_Viewer::_ready(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("GDN_TheWorld_Viewer::_ready");

	m_ready = true;
}

void GDN_TheWorld_Viewer::toggleQuadrantSelected(void)
{
	if (trackMouse())
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
						it->second->setEditModeQuadrantSelected(true);
						it->second->materialParamsNeedReset(true);
						call_deferred("emit_signal", "tw_quadrant_selected_for_edit", true);
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
					it->second->setEditModeQuadrantSelected(false);
					it->second->materialParamsNeedReset(true);
					call_deferred("emit_signal", "tw_quadrant_selected_for_edit", false);
				}
			}

			m_quadrantSelPos.reset();
		}

		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (editModeUIControl == nullptr || !editModeUIControl->initilized())
		{
			createEditModeUI();
			editModeUIControl = EditModeUIControl();
			editModeUIControl->set_visible(false);
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

void GDN_TheWorld_Viewer::_input(const Ref<InputEvent>& event)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
	if (eventMouseButton != nullptr)
	{
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT && eventMouseButton->is_pressed())
			m_trackedInputEvents.push_back(TrackedInputEvent::MOUSE_BUTTON_LEFT);
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT && eventMouseButton->is_double_click())
			m_trackedInputEvents.push_back(TrackedInputEvent::MOUSE_DOUBLE_BUTTON_LEFT);
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_RIGHT && eventMouseButton->is_pressed())
			m_trackedInputEvents.push_back(TrackedInputEvent::MOUSE_BUTTON_RIGHT);
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_RIGHT && eventMouseButton->is_double_click())
			m_trackedInputEvents.push_back(TrackedInputEvent::MOUSE_DOUBLE_BUTTON_RIGHT);
	}

	if (event->is_action_pressed("ui_select") && m_altPressed)
	{
		toggleQuadrantSelected();
	}

	if (event->is_action_pressed("ui_toggle_track_mouse"))
	{
		toggleTrackMouse();
	}

	if (event->is_action_pressed("ui_toggle_edit_mode"))
	{
		toggleEditMode();
	}

	if (event->is_action_pressed("ui_toggle_debug_visibility"))
	{
		toggleDebugVisibility();
	}
	
	if (event->is_action_pressed("ui_rotate_chunk_debug_mode"))
	{
		rotateChunkDebugMode();
	}
	
	if (event->is_action_pressed("ui_rotate_drawing_mode"))
	{
		rotateDrawingMode();
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

void GDN_TheWorld_Viewer::toggleTrackMouse(void)
{
	if (!m_editMode)
	{
		if (trackMouse())
			trackMouse(false);
		else
			trackMouse(true);
	}
	else
		m_previousTrackedMouseHit = godot::Vector3();

	if (trackMouse())
		m_mouseQuadrantHitName = "";
}

bool GDN_TheWorld_Viewer::trackMouse(void)
{
	return m_trackMouse;
}

void GDN_TheWorld_Viewer::trackMouse(bool b)
{
	m_trackMouse = b;
	if (!m_trackMouse)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

		for (auto& it : m_mapQuadTree)
		{
			it.second->mouseHitChanged(godot::Vector3(), false);
		}
		m_normalDrawer->set_visible(false);
		m_positionDrawer->set_visible(false);
		m_gizmo->set_visible(false);

		m_mouseQuadrantHitName = "";
		m_mouseQuadrantHitTag = "";
		m_mouseQuadrantHitPos = godot::Vector3();
		m_mouseQuadrantHitSize = 0.0f;
		m_quadrantHitPos = QuadrantPos();
		m_quadrantHit = nullptr;
		m_adjacentQuadrantsHit.clear();
		m_previousTrackedMouseHit = godot::Vector3();
	}
}

void GDN_TheWorld_Viewer::toggleEditMode(void)
{
	m_previousTrackedMouseHit = godot::Vector3();
	m_editMode = !m_editMode;
	if (m_editMode)
		trackMouse(true);
	else
		trackMouse(false);
}

void GDN_TheWorld_Viewer::toggleDebugVisibility(void)
{
	m_debugContentVisibility = !m_debugContentVisibility;
	m_updateTerrainVisibilityRequired = true;
}

void GDN_TheWorld_Viewer::rotateChunkDebugMode(void)
{
	m_requiredChunkDebugMode = GDN_TheWorld_Globals::rotateChunkDebugMode(m_currentChunkDebugMode);
	m_updateDebugModeRequired = true;
}

void GDN_TheWorld_Viewer::rotateDrawingMode(void)
{
	godot::Viewport* vp = get_viewport();
	godot::Viewport::DebugDraw dd = vp->get_debug_draw();
	vp->set_debug_draw( (godot::Viewport::DebugDraw)((dd + 1) % 26));
	m_debugDraw = vp->get_debug_draw();
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
		case NOTIFICATION_WM_CLOSE_REQUEST:
		{
			m_quitting = true;
		}
		break;
		case NOTIFICATION_PREDELETE:
		{
			if (m_ready)
			{
				GDN_TheWorld_Globals* globals = Globals();
				if (globals != nullptr)
					if (m_quitting)
						globals->debugPrint("GDN_TheWorld_Viewer::_notification (NOTIFICATION_PREDELETE) - Destroy Viewer", false);
					else
						globals->debugPrint("GDN_TheWorld_Viewer::_notification (NOTIFICATION_PREDELETE) - Destroy Viewer");
			}
	
			deinit();
		}
		break;
		case NOTIFICATION_ENTER_WORLD:
		{
			//GDN_TheWorld_Globals* globals = Globals();
			//printKeyboardMapping();
			//string s = "Use Visual Server: "; s += (m_useVisualServer ? "True" : "False");
			//if (globals != nullptr)
			//{
			//	globals->infoPrint(s.c_str());
			//	globals->debugPrint("Enter world");
			//}
			////if (m_initialWordlViewerPosSet)
			////{
			//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
			//for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			//{
			//	if (!itQuadTree->second->isValid())
			//		continue;

			//	if (!itQuadTree->second->isVisible())
			//		continue;

			//	Chunk::EnterWorldChunkAction action;
			//	itQuadTree->second->ForAllChunk(action);
			//	
			//	itQuadTree->second->getQuadrant()->getCollider()->enterWorld();
			//}
			////}
		}
		break;
		case NOTIFICATION_EXIT_WORLD:
		{
			//GDN_TheWorld_Globals* globals = Globals();
			//if (globals != nullptr)
			//	globals->debugPrint("Exit world");
			////if (m_initialWordlViewerPosSet)
			////{
			//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
			//for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
			//{
			//	if (!itQuadTree->second->isValid())
			//		continue;

			//	if (!itQuadTree->second->isVisible())
			//		continue;

			//	Chunk::ExitWorldChunkAction action;
			//	itQuadTree->second->ForAllChunk(action);
			//	
			//	itQuadTree->second->getQuadrant()->getCollider()->exitWorld();
			//	//itQuadTree->second->getQuadrant()->getCollider()->onGlobalTransformChanged();
			//}
			////}
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
		//case NOTIFICATION_WM_SIZE_CHANGED:
		//{
		//	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		//	if (editModeUIControl != nullptr && editModeUIControl->initilized())
		//		editModeUIControl->controlNeedResize();
		//}
		//break;
	}
}

void GDN_TheWorld_Viewer::viewportSizeChanged(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr && editModeUIControl->initilized())
		editModeUIControl->controlNeedResize();
}

String GDN_TheWorld_Viewer::getDebugDrawMode(void)
{
	if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_DISABLED)
		return "DEBUG_DRAW_DISABLED";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_UNSHADED)
		return "DEBUG_DRAW_UNSHADED";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_LIGHTING)
		return "DEBUG_DRAW_LIGHTING";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_OVERDRAW)
		return "DEBUG_DRAW_OVERDRAW";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_WIREFRAME)
		return "DEBUG_DRAW_WIREFRAME";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_NORMAL_BUFFER)
		return "DEBUG_DRAW_NORMAL_BUFFER";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_VOXEL_GI_ALBEDO)
		return "DEBUG_DRAW_VOXEL_GI_ALBEDO";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_VOXEL_GI_LIGHTING)
		return "DEBUG_DRAW_VOXEL_GI_LIGHTING";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_VOXEL_GI_EMISSION)
		return "DEBUG_DRAW_VOXEL_GI_EMISSION";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SHADOW_ATLAS)
		return "DEBUG_DRAW_SHADOW_ATLAS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_DIRECTIONAL_SHADOW_ATLAS)
		return "DEBUG_DRAW_DIRECTIONAL_SHADOW_ATLAS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SCENE_LUMINANCE)
		return "DEBUG_DRAW_SCENE_LUMINANCE";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SSAO)
		return "DEBUG_DRAW_SSAO";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SSIL)
		return "DEBUG_DRAW_SSIL";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_PSSM_SPLITS)
		return "DEBUG_DRAW_PSSM_SPLITS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_DECAL_ATLAS)
		return "DEBUG_DRAW_DECAL_ATLAS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SDFGI)
		return "DEBUG_DRAW_SDFGI";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SDFGI_PROBES)
		return "DEBUG_DRAW_SDFGI_PROBES";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_GI_BUFFER)
		return "DEBUG_DRAW_GI_BUFFER";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_DISABLE_LOD)
		return "DEBUG_DRAW_DISABLE_LOD";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_CLUSTER_OMNI_LIGHTS)
		return "DEBUG_DRAW_CLUSTER_OMNI_LIGHTS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_CLUSTER_SPOT_LIGHTS)
		return "DEBUG_DRAW_CLUSTER_SPOT_LIGHTS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_CLUSTER_DECALS)
		return "DEBUG_DRAW_CLUSTER_DECALS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_CLUSTER_REFLECTION_PROBES)
		return "DEBUG_DRAW_CLUSTER_REFLECTION_PROBES";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_OCCLUDERS)
		return "DEBUG_DRAW_OCCLUDERS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_MOTION_VECTORS)
		return "DEBUG_DRAW_MOTION_VECTORS";
	else
		return std::to_string(int(m_debugDraw)).c_str();
}

String GDN_TheWorld_Viewer::getChunkDebugModeStr(void)
{
	if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::DoNotSet)
		return "CHUNK_DEBUG_MODE_DO_NOT_SET";
	else if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
		return "CHUNK_DEBUG_MODE_NO_DEBUG";
	else if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB)
		return "CHUNK_DEBUG_MODE_WIREFRAME_AABB";
	else if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
		return "CHUNK_DEBUG_MODE_WIREFRAME_SQUARE";
	else
		return "";
}

void GDN_TheWorld_Viewer::setDepthQuadOnPerimeter(int depth)
{
	if (depth >= 0 && depth <= MAX_DEPTH_ON_PERIMETER)
	{
		m_depthQuadOnPerimeter = depth;
		if ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
			m_recalcQuadrantsInViewNeeded = true;
	}
}

int GDN_TheWorld_Viewer::getDepthQuadOnPerimeter(void)
{
	return (int)m_depthQuadOnPerimeter;
}

void GDN_TheWorld_Viewer::setCacheQuadOnPerimeter(int cache)
{
	if (cache >= 0 && cache <= MAX_CACHE_ON_PERIMETER)
	{
		m_cacheQuadOnPerimeter = cache;
		if ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
			m_recalcQuadrantsInViewNeeded = true;
	}
}

int GDN_TheWorld_Viewer::getCacheQuadOnPerimeter(void)
{
	return (int)m_cacheQuadOnPerimeter;
}

String GDN_TheWorld_Viewer::getInfoCamera(void)
{
	godot::Camera3D* camera = getCamera();

	if (camera == nullptr)
		return "";

	double z_near = camera->get_near();
	double z_far = camera->get_far();
	double size = camera->get_size();
	Vector2 offset = camera->get_frustum_offset();
	double fov = camera->get_fov();
	double h_offset = camera->get_h_offset();
	double v_offset = camera->get_v_offset();
	godot::Camera3D::KeepAspect keepAspect = camera->get_keep_aspect_mode();

	std::string info;

	info = "znear=" + std::to_string(z_near) + " zfar=" + std::to_string(z_far) + " size=" + std::to_string(size) + " offset=" + std::to_string(h_offset) + "," + std::to_string(v_offset) + " fov=" + std::to_string(fov) + " keep_aspect=" + std::to_string(int(keepAspect));

	return info.c_str();
}

int GDN_TheWorld_Viewer::getCameraProjectionMode(void)
{
	godot::Camera3D* camera = getCamera();

	if (camera == nullptr)
		return -1;

	godot::Camera3D::ProjectionType projection = camera->get_projection();

	return (int)projection;
}

void GDN_TheWorld_Viewer::recalcQuadrantsInView(void)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants");

	QuadrantPos cameraQuadrant = m_computedCameraQuadrantPos;
	if (!cameraQuadrant.isInitialized())
		return;
	
	vector<QuadrantPos> quadrantPosNeeded;
	quadrantPosNeeded.push_back(cameraQuadrant);

	if (IS_EDITOR_HINT())
	{
		m_numVisibleQuadrantOnPerimeter = getDepthQuadOnPerimeter();
		m_numCacheQuadrantOnPerimeter = m_numVisibleQuadrantOnPerimeter + getCacheQuadOnPerimeter();

		//m_numVisibleQuadrantOnPerimeter = 0;	// SUPERDEBUGRIC only camera quadrant
		//m_numCacheQuadrantOnPerimeter = 0;		// SUPERDEBUGRIC only camera quadrant

		//m_numVisibleQuadrantOnPerimeter = 1;	// SUPERDEBUGRIC only camera quadrant & 1 surrounding
		//m_numCacheQuadrantOnPerimeter = 1;		// SUPERDEBUGRIC only camera quadrant & 1 surrounding
	}
	
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
	if (!IS_EDITOR_HINT())
	{
		m_numVisibleQuadrantOnPerimeter = getDepthQuadOnPerimeter();
		m_numCacheQuadrantOnPerimeter = m_numVisibleQuadrantOnPerimeter + getCacheQuadOnPerimeter();

		//m_numVisibleQuadrantOnPerimeter = 3;
		//m_numCacheQuadrantOnPerimeter = m_numVisibleQuadrantOnPerimeter + 1;

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
						Chunk::VisibilityChangedChunkAction action(m_visibleInTree);
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

void GDN_TheWorld_Viewer::_process(double _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_TheWorld_Viewer::_process");

	TheWorld_Utils::GuardProfiler profiler(std::string("_process 1 ") + __FUNCTION__, "ALL");

	m_visibleInTree = is_visible_in_tree();
	
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	setTrackedInputEvents();

	if (initRequired())
	{
		init();
		if (!ShaderTerrainData::groundTexturesNeedProcessing())
			globals->setStatus(TheWorldStatus::sessionInitialized);
	}
	
	//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
	std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing, std::try_to_lock);
	if (!lock.owns_lock())
	{
		m_numProcessNotOwnsLock++;
		return;
	}

	if (ShaderTerrainData::groundTexturesNeedProcessing())
	{
		std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>>& groundTextures = ShaderTerrainData::getGroundTextures();

		bool resetProcessing = true;

		for (const auto& groundTexture : groundTextures)
		{
			if (!groundTexture.second->initialized)
			{
				ShaderTerrainData::getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + groundTexture.first.c_str(), groundTexture.second.get(), this);
				globals->addNumElementInitialized();
				resetProcessing = false;
				break;
			}
		}

		if (resetProcessing)
		{
			ShaderTerrainData::groundTexturesNeedProcessing(false);
			globals->setStatus(TheWorldStatus::sessionInitialized);
		}
	}

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	if (!WorldCamera() || !m_initialWordlViewerPosSet)
		return;

	godot::Camera3D* activeCamera = nullptr;
	if (IS_EDITOR_HINT())
	{
		activeCamera = getCamera();
	}
	else
	{
		activeCamera = WorldCamera()->getActiveCamera();
	}
	if (!activeCamera)
		return;
	
	if (m_gizmo != nullptr)
		m_gizmo->set_camera(activeCamera);

	if (m_normalDrawer != nullptr)
		m_normalDrawer->set_camera(activeCamera, this);

	if (m_positionDrawer != nullptr)
		m_positionDrawer->set_camera(activeCamera, this);

	//// DEBUG
	//{
	//	godot::Camera3D::ProjectionType type = activeCamera->get_projection();
	//	int i = activeCamera->get_cull_mask();
	//	bool b = activeCamera->is_current();
	//	godot::Camera3D::DopplerTracking d = activeCamera->get_doppler_tracking();
	//	double far1 = activeCamera->get_far();
	//	double fov = activeCamera->get_fov();
	//	godot::Vector2 frustumOffset = activeCamera->get_frustum_offset();
	//	double hOffset = activeCamera->get_h_offset();
	//	double vOffset = activeCamera->get_v_offset();
	//	double near1 = activeCamera->get_near();
	//	double size1 = activeCamera->get_size();
	//	godot::Camera3D::KeepAspect k = activeCamera->get_keep_aspect_mode();
	//	type = (godot::Camera3D::ProjectionType)0;
	//}
	//// DEBUG

	TheWorld_Viewer_Utils::TimerMcs clock;
	clock.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock, this] { clock.tock(); this->m_numProcessExecution++; this->m_processDuration += clock.duration().count(); });

	process_impl(_delta, activeCamera);

	resetTrackedInputEvents();
}

godot::Control* GDN_TheWorld_Viewer::getOrCreateEditModeUIControl(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl == nullptr || !editModeUIControl->initilized())
	{
		createEditModeUI();
		editModeUIControl = EditModeUIControl();

		if (m_editMode)
			editModeUIControl->set_visible(true);
		else
			editModeUIControl->set_visible(false);
	}

	return editModeUIControl;
}

void GDN_TheWorld_Viewer::process_impl(double _delta, godot::Camera3D* activeCamera)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.1 ") + __FUNCTION__, "Lock owned");

	if (m_firstProcess)
	{
		m_debugDraw = get_viewport()->get_debug_draw();

		// TODORIC view frustum
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

	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr && editModeUIControl->initilized())
	{
		if (m_editMode)
		{
			if (!editModeUIControl->is_visible())
				editModeUIControl->set_visible(true);
		}
		else
		{
			if (editModeUIControl->is_visible())
				editModeUIControl->set_visible(false);
		}
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

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.2 ") + __FUNCTION__, "Adjust Quadtrees");
		process_adjustQuadTreeByCamera(globals, cameraPosGlobalCoord);
	}

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.3 ") + __FUNCTION__, "Init new quadrants");
		process_streamingQuadrantStuff();
	}

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.4 ") + __FUNCTION__, "Track mouse");
		process_trackMouse(activeCamera);
	}

	process_updateQuads(cameraPosGlobalCoord);

	process_updateChunks();

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

	process_lookDev();
	
	process_materialParam();

	//m_desideredLookDevChanged = false;

	process_dump(globals);

	process_statistics(globals);
}

void GDN_TheWorld_Viewer::process_adjustQuadTreeByCamera(GDN_TheWorld_Globals* globals, Vector3& cameraPosGlobalCoord)
{
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
	if ((m_refreshMapQuadTree || m_computedCameraQuadrantPos != cameraQuadrant) && (int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
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

void GDN_TheWorld_Viewer::process_streamingQuadrantStuff(void)
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
						float x = 0, y = 0, z = 0, yaw = 0, pitch = 0, roll = 0;
						if (itQuadTree->second->statusUninitialized())
							itQuadTree->second->init(x, y, z, yaw, pitch, roll, false, 0.0f);
						else
							itQuadTree->second->refreshTerrainData(x, y, z, yaw, pitch, roll, false, 0.0f);
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
			// m_numVisibleQuadrantOnPerimeter == 1
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

void GDN_TheWorld_Viewer::process_trackMouse(godot::Camera3D* activeCamera)
{

	godot::Node* collider = nullptr;
	if (trackMouse())
	{
		setTrackedInputEvents(&m_trackedInputEventsForTrackMouse);

		int64_t timeElapsed = TIME()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastMouseTrack > TIME_INTERVAL_BETWEEN_MOUSE_TRACK)
		{
			TheWorld_Viewer_Utils::TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Track Mouse Chunk", false, false);
			clock1.tick();
			auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
				clock1.tock();
				this->m_numMouseTrackHit++;
				this->m_mouseTrackHitDuration += clock1.duration().count();
				});

			godot::PhysicsDirectSpaceState3D* spaceState = get_world_3d()->get_direct_space_state();
			godot::Dictionary rayArray;
			godot::Vector2 mousePosInVieport = getMousPosInViewport();
			godot::Vector3 rayOrigin = activeCamera->project_ray_origin(mousePosInVieport);
			godot::Vector3 rayEnd = rayOrigin + activeCamera->project_ray_normal(mousePosInVieport) * (real_t)activeCamera->get_far() * 1.5;
			godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd);
			rayArray = spaceState->intersect_ray(query);

			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();

			if (rayArray.has("position"))
			{
				m_trackedMouseHit = rayArray["position"];
				m_mouseTrackedOnTerrain = true;
			}
			else
				m_mouseTrackedOnTerrain = false;

			if (m_mouseTrackedOnTerrain)
			{
				if (m_editMode && editModeUIControl && editModeUIControl->initilized())
					editModeUIControl->setMouseHitLabelText(std::string("X=") + std::to_string(m_trackedMouseHit.x) + " Y=" + std::to_string(m_trackedMouseHit.y) + " Z=" + std::to_string(m_trackedMouseHit.z));

				if (rayArray.has("collider"))
				{
					collider = godot::Object::cast_to<godot::Node>(rayArray["collider"]);
					if (collider->has_meta("QuadrantName"))
					{
						godot::String s = collider->get_meta("QuadrantName", "");
						godot::PackedByteArray array = s.to_ascii_buffer();
						std::string mouseQuadrantHitName((char*)array.ptr(), array.size());
						//const char* str = s.utf8().get_data();
						//std::string mouseQuadrantHitName = str;
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

							trackedMouseHitChanged(m_quadrantHit, m_adjacentQuadrantsHit, false, godot::Vector3());

							// Get current quadrant
							std::string mouseQuadrantHitTag;
							QuadTree* quadrantHit = nullptr;
							std::list<QuadTree*> adjacentQuadrantsHit;
							MapQuadTree::iterator it = m_mapQuadTree.find(quadrantHitPos);
							if (it != m_mapQuadTree.end() && !it->second->statusToErase())
							{
								mouseQuadrantHitTag = it->second->getTag();

								quadrantHit = it->second.get();
								quadrantHit->getAdjacentQuadrants(adjacentQuadrantsHit);
								trackedMouseHitChanged(quadrantHit, adjacentQuadrantsHit, true, m_trackedMouseHit);
							}

							if (m_editMode && editModeUIControl && editModeUIControl->initilized())
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
							m_quadrantHit = quadrantHit;
							m_adjacentQuadrantsHit.clear();
							for (auto& item : adjacentQuadrantsHit)
								m_adjacentQuadrantsHit.push_back(item);
						}
						else
						{
							trackedMouseHitChanged(m_quadrantHit, m_adjacentQuadrantsHit, true, m_trackedMouseHit);
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
			}
			else
			{
				trackedMouseHitChanged(m_quadrantHit, m_adjacentQuadrantsHit, false, godot::Vector3());

				m_mouseQuadrantHitName = "";
				m_mouseQuadrantHitTag = "";
				m_mouseQuadrantHitPos = godot::Vector3();
				m_mouseQuadrantHitSize = 0.0f;
				m_quadrantHitPos = QuadrantPos();
				m_quadrantHit = nullptr;
				m_adjacentQuadrantsHit.clear();
				m_previousTrackedMouseHit = godot::Vector3();
			}

			resetTrackedInputEvents(&m_trackedInputEventsForTrackMouse);
			
			m_timeElapsedFromLastMouseTrack = timeElapsed;
		}
	}
	else
		resetTrackedInputEvents(&m_trackedInputEventsForTrackMouse);
}

void GDN_TheWorld_Viewer::trackedMouseHitChanged(QuadTree* quadrantHit, std::list<QuadTree*>& adjacentQuadrantsHit, bool hit, godot::Vector3 mouseHit)
{

	if (quadrantHit != nullptr)
	{
		if (hit)
		{
			if (positionVisible())
			{
				if (trackedInputEvent(TrackedInputEvent::MOUSE_DOUBLE_BUTTON_LEFT, &m_trackedInputEventsForTrackMouse))
					trackingDistance(true, mouseHit);
				if (trackedInputEvent(TrackedInputEvent::MOUSE_DOUBLE_BUTTON_RIGHT, &m_trackedInputEventsForTrackMouse))
					trackingDistance(false);
			}

			if (m_previousTrackedMouseHit != mouseHit)
			{
				quadrantHit->mouseHitChanged(mouseHit, true);
				for (auto& item : adjacentQuadrantsHit)
					item->mouseHitChanged(mouseHit, true);

				bool _normalVisible = false;
				if (normalVisible())
				{
					size_t index = quadrantHit->getQuadrant()->getIndexFromMap(mouseHit.x, mouseHit.z);
					if (index != -1)
					{
						bool ok;
						godot::Vector3 normal = quadrantHit->getQuadrant()->getNormalFromNormalmap(index, ok);
						if (ok)
						{
							const float lineSize = 50;
							godot::Vector3 end = mouseHit + normal * lineSize;
							float radAngleWithVertical = acos(normal.dot(Vector3Up));
							float slope = (radAngleWithVertical * 180) / TheWorld_Utils::kPi;
							

							if (m_normalLineIdx != -1 && m_normalSphereIdx != -1)
							{
								_normalVisible = true;
								m_normalDrawer->updateLine(m_normalLineIdx, mouseHit, end, std::to_string(int32_t(slope)) + "�");
								m_normalDrawer->updateSphere(m_normalSphereIdx, end);
							}
						}
					}
				}

				if (_normalVisible)
					m_normalDrawer->set_visible(true);
				else
					m_normalDrawer->set_visible(false);

				bool _positionVisible = false;
				if (positionVisible())
				{
					if (m_positionLabelXIdx != -1 && m_positionLabelYIdx != -1 && m_positionLabelZIdx != -1)
					{
						_positionVisible = true;
						m_positionDrawer->updateLabel2d(m_positionLabelXIdx, mouseHit, std::string("X=") + std::to_string(mouseHit.x));
						m_positionDrawer->updateLabel2d(m_positionLabelZIdx, mouseHit, std::string("Z=") + std::to_string(mouseHit.z));
						m_positionDrawer->updateLabel2d(m_positionLabelYIdx, mouseHit, std::string("H=") + std::to_string(mouseHit.y));
					}

					if (trackingDistance())
					{
						float distance = mouseHit.distance_to(m_savedPosForTrackingDistance);
						m_positionDrawer->updateLine(m_positionLineIdx, m_savedPosForTrackingDistance, mouseHit, std::to_string(distance));
						m_positionDrawer->updateSphere(m_positionSphereStartIdx, m_savedPosForTrackingDistance, 5);
						m_positionDrawer->updateSphere(m_positionSphereEndIdx, mouseHit, 5);
					}
				}

				if (_positionVisible)
					m_positionDrawer->set_visible(true);
				else
					m_positionDrawer->set_visible(false);

				if (gizmoVisible())
				{
					m_gizmo->set_visible(true);
					m_gizmo->set_global_position(mouseHit);
				}
				else
					m_gizmo->set_visible(false);

				m_previousTrackedMouseHit = mouseHit;
			}
		}
		else
		{
			quadrantHit->mouseHitChanged(godot::Vector3(), false);
			for (auto& item : adjacentQuadrantsHit)
				item->mouseHitChanged(godot::Vector3(), false);
			m_normalDrawer->set_visible(false);
			m_positionDrawer->set_visible(false);
			m_gizmo->set_visible(false);
		}
	}
}

bool GDN_TheWorld_Viewer::mouseInsideMainEditPanel(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr && editModeUIControl->initilized())
		return editModeUIControl->mouseInsideEditPanel();
	else
		return false;
}

void GDN_TheWorld_Viewer::process_updateQuads(Vector3& cameraPosGlobalCoord)
{
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

		Chunk::RefreshChunkAction action(m_visibleInTree);
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			// ATTENZIONE
			if (itQuadTree->second->getQuadrant()->internalDataLocked())
				continue;

			itQuadTree->second->ForAllChunk(action);
		}
		m_refreshRequired = false;
	}
}

void GDN_TheWorld_Viewer::process_updateChunks(void)
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

		// ATTENZIONE
		if (itQuadTree->second->getQuadrant()->internalDataLocked())
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
					// ATTENZIONE
					if (iter != m_mapQuadTree.end() && !iter->second->statusToErase() && !iter->second->getQuadrant()->internalDataLocked())
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
					// ATTENZIONE
					if (iter != m_mapQuadTree.end() && !iter->second->statusToErase() && !iter->second->getQuadrant()->internalDataLocked())
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
					// ATTENZIONE
					if (iter != m_mapQuadTree.end() && !iter->second->statusToErase() && !iter->second->getQuadrant()->internalDataLocked())
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
					// ATTENZIONE
					if (iter != m_mapQuadTree.end() && !iter->second->statusToErase() && !iter->second->getQuadrant()->internalDataLocked())
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
			(*it)->update(m_visibleInTree);

		for (std::vector<Chunk*>::iterator it = vectAdditionalUpdateChunk.begin(); it != vectAdditionalUpdateChunk.end(); it++)
			(*it)->update(m_visibleInTree);

		itQuadTree->second->clearChunkUpdate();
		vectAdditionalUpdateChunk.clear();
	}
}

void GDN_TheWorld_Viewer::process_lookDev(void)
{
	if (m_desideredLookDevChanged)
	{
		ShaderTerrainData::freeLookDevSubViewport();

		enum class ShaderTerrainData::LookDev desideredLookDev = getDesideredLookDev();

		for (auto& it : m_mapQuadTree)
		{
			if (desideredLookDev == ShaderTerrainData::LookDev::NotSet)
				it.second->setRegularMaterial();
			else
				it.second->setLookDevMaterial(desideredLookDev);
		}
	}

	m_desideredLookDevChanged = false;
}

void GDN_TheWorld_Viewer::process_materialParam(void)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.11 ") + __FUNCTION__, "Material params stuff");

	TheWorld_Viewer_Utils::TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Material Params", false, false);
	clock1.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
		clock1.tock();
		this->m_numUpdateMaterialParams++;
		this->m_updateMaterialParamsDuration += clock1.duration().count();
		});

	if (ReloadSelectedQuadrantShaderRequired())
	{
		QuadTree* quadTreeSel = nullptr;
		QuadrantPos quadrantSelPos = getQuadrantSelForEdit(&quadTreeSel);

		if (quadTreeSel != nullptr)
		{
			quadTreeSel->reloadCurrentMaterial();
		}
		
		ReloadSelectedQuadrantShaderRequired(false);
	}

	if (ReloadAllShadersRequired())
	{
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			itQuadTree->second->reloadCurrentMaterial();
		}

		ReloadAllShadersRequired(false);
	}
	
	if (m_shaderParamChanged)
	{
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			itQuadTree->second->getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate(true);
		}

		m_shaderParamChanged = false;
	}

	//if ((int)globals->status() >= (int)TheWorldStatus::worldDeployed)
	{
		for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
		{
			bool reset = false;
			reset = itQuadTree->second->resetMaterialParams();
			bool updated = itQuadTree->second->updateMaterialParams();

			if (updated)
				m_previousTrackedMouseHit = godot::Vector3();

			if ((reset || updated) && !m_desideredLookDevChanged)
				break;
		}
	}
}

void GDN_TheWorld_Viewer::process_dump(GDN_TheWorld_Globals* globals)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.12 ") + __FUNCTION__, "Dump stuff");

	// Check for Dump
	if (TIME_INTERVAL_BETWEEN_DUMP != 0 && globals->isDebugEnabled())
	{
		int64_t timeElapsed = TIME()->get_ticks_msec();
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

void GDN_TheWorld_Viewer::process_statistics(GDN_TheWorld_Globals* globals)
{
	// Check for Statistics
	if (TIME_INTERVAL_BETWEEN_STATISTICS != 0)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.13 ") + __FUNCTION__, "Refresh statistics");

		int64_t timeElapsed = TIME()->get_ticks_msec();
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

void GDN_TheWorld_Viewer::_physics_process(double _delta)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("_physics_process 1 ") + __FUNCTION__, "ALL");

	//if (IS_EDITOR_HINT())
	//	return;

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	Input* input = Input::get_singleton();

	if (IS_EDITOR_HINT())
	{

	}
	else
	{
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
}

void GDN_TheWorld_Viewer::clearMapQuadTree(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	m_mapQuadTree.clear();
	//m_cameraQuadTree = nullptr;
	m_quadrantHit = nullptr;
	m_adjacentQuadrantsHit.clear();
	m_previousTrackedMouseHit = godot::Vector3();
	m_quadrantSelPos.reset();
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
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl == nullptr)
	{
		m_mtxQuadTreeAndMainProcessing.lock();
		editModeUIControl = memnew(GDN_TheWorld_Edit);
		m_mtxQuadTreeAndMainProcessing.unlock();
		if (editModeUIControl == nullptr)
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	}

	if (!editModeUIControl->initilized())
	{
		editModeUIControl->init(this);
		//SceneTree* scene = get_tree();
		//Node* sceneRoot = nullptr;
		//if (scene != nullptr)
		//	sceneRoot = scene->get_edited_scene_root();
		//if (sceneRoot != nullptr)
		//	editModeUIControl->set_owner(sceneRoot);
	}
}

bool GDN_TheWorld_Viewer::isQuadrantSelectedForEdit(void)
{
	if (m_quadrantSelPos.empty())
		return false;
	else
		return true;
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
		m_editModeUIControl = Object::cast_to<GDN_TheWorld_Edit>(root->find_child(THEWORLD_EDIT_MODE_UI_CONTROL_NAME, true, false));
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
		m_globals = Object::cast_to<GDN_TheWorld_Globals>(root->find_child(THEWORLD_GLOBALS_NODE_NAME, true, false));
	}

	return m_globals;
}

GDN_TheWorld_Camera* GDN_TheWorld_Viewer::CameraNode(bool useCache)
{
	if (m_worldCamera == NULL || !useCache)
	{
		SceneTree* scene = get_tree();
		if (!scene)
			return NULL;
		Viewport* root = scene->get_root();
		if (!root)
			return NULL;
		m_worldCamera = Object::cast_to<GDN_TheWorld_Camera>(root->find_child(THEWORLD_CAMERA_NODE_NAME, true, false));
	}

	return m_worldCamera;
}

bool GDN_TheWorld_Viewer::terrainShiftPermitted(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();

	if (editModeUIControl == nullptr || !editModeUIControl->initilized())
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

godot::Camera3D* GDN_TheWorld_Viewer::getCamera(void)
{
	if (IS_EDITOR_HINT())
		return getCameraInEditor();
	else
		return get_tree()->get_root()->get_camera_3d();
}

void GDN_TheWorld_Viewer::setEditorCamera(godot::Camera3D* editorCamera)
{
	if (m_editorCamera == nullptr)
		onTransformChanged();

	m_editorCamera = editorCamera;
}

godot::Camera3D* GDN_TheWorld_Viewer::getCameraInEditor(void)
{
	if (!IS_EDITOR_HINT())
		return nullptr;

	if (m_editorCamera != nullptr)
		return m_editorCamera;
	
	godot::Camera3D* editorCamera = nullptr;

	if (m_editorInterface != nullptr)
	{
		Node* editorSceneRootViewport = m_editorInterface->get_edited_scene_root();

		godot::Array foundNodes;
		//godot::Array children = get_tree()->get_edited_scene_root()->get_children();
		godot::Array children = editorSceneRootViewport->get_children();
		int64_t ___size = children.size();	// DEBUG
		if (!children.is_empty())
			_findChildNodes(foundNodes, children, "GDN_TheWorld_Camera");

		___size = foundNodes.size();	// DEBUG
		if (!foundNodes.is_empty())
			editorCamera = godot::Object::cast_to<godot::Camera3D>(foundNodes[0]);
	}

	return editorCamera;
}

void GDN_TheWorld_Viewer::_findChildNodes(godot::Array& foundNodes, godot::Array& searchNodes, String searchClass)
{
	for (int i = 0; i < searchNodes.size(); i++)
	{
		String className = godot::Object::cast_to<godot::Node>(searchNodes[i])->get_class();
		std::string ___classsName = to_string(className);		// DEBUG
		std::string ___nodeName = to_string(godot::Object::cast_to<godot::Node>(searchNodes[i])->get_name());	// DEBUG
		if (className == searchClass)
			foundNodes.push_back(searchNodes[i]);
		godot::Array children = godot::Object::cast_to<godot::Node>(searchNodes[i])->get_children();
		int64_t ___size = children.size();	// DEBUG
		if (!children.is_empty())
			_findChildNodes(foundNodes, children, searchClass);
	}
}

void GDN_TheWorld_Viewer::deployWorld(float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll, int level, int chunkSizeShift, int heightmapResolutionShift)
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
		
		Globals()->setStatus(TheWorldStatus::worldDeployInProgress);

		float _gridStepInWU = Globals()->gridStepInWU();

		Globals()->Client()->MapManagerDeployWorld(isInEditor(), level, (size_t)m_numWorldVerticesPerSize, _gridStepInWU, true, cameraX, cameraY, cameraZ, cameraDistanceFromTerrainForced, cameraYaw, cameraPitch, cameraRoll);
		
		//Globals()->Client()->MapManagerDeployLevel(level, (size_t)m_numWorldVerticesPerSize, _gridStepInWU, true, cameraX, cameraY, cameraZ, cameraDistanceFromTerrainForced, cameraYaw, cameraPitch, cameraRoll);

		//QuadrantPos quadrantPos(cameraX, cameraZ, level, m_numWorldVerticesPerSize, _gridStepInWU);
		//quadrantPos.setTag("Camera");

		//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

		//clearMapQuadTree();
		//m_mapQuadTree[quadrantPos] = make_unique<QuadTree>(this, quadrantPos);
		//m_mapQuadTree[quadrantPos]->init(cameraX, cameraY, cameraZ, cameraYaw, cameraPitch, cameraRoll, true, cameraDistanceFromTerrainForced);
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

void GDN_TheWorld_Viewer::undeployWorld(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	m_stopDeploy = true;
	
	enum class TheWorldStatus status = Globals()->status();

	if ((int)status <= (int)TheWorldStatus::worldDeployInProgress)
		return;
	
	if ((int)status >= (int)TheWorldStatus::worldUnDeployInProgress)
		return;

	Globals()->setStatus(TheWorldStatus::worldUnDeployInProgress);

	Globals()->Client()->MapManagerUndeployWorld(isInEditor());

	m_stopDeploy = false;
}

Node3D* GDN_TheWorld_Viewer::getWorldNode(void)
{
	// MapManager coordinates are relative to WorldNode
	return (Node3D*)get_parent();
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

void GDN_TheWorld_Viewer::updateMaterialParamsForEveryQuadrant(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("updateMaterialParamsForEveryQuadrant");

	if (!is_inside_tree())
		return;

	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;

		if (!itQuadTree->second->isVisible())
			continue;

		if (itQuadTree->second->status() != QuadrantStatus::uninitialized && itQuadTree->second->status() != QuadrantStatus::toErase)
			itQuadTree->second->materialParamsNeedUpdate(true);
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

Transform3D GDN_TheWorld_Viewer::getCameraChunkGlobalTransformApplied(void)
{
	if (m_cameraChunk && !m_cameraChunk->isMeshNull())
		return m_cameraChunk->getGlobalTransformApplied();
	else
		return Transform3D();
}

Transform3D GDN_TheWorld_Viewer::getCameraChunkDebugGlobalTransformApplied(void)
{
	if (m_cameraChunk && !m_cameraChunk->isDebugMeshNull())
		return m_cameraChunk->getDebugGlobalTransformApplied();
	else
		return Transform3D();
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

	double f = Engine::get_singleton()->get_frames_per_second();
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
		Node* node = godot::Object::cast_to<godot::Node>(nodes[i]);
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

	Globals()->debugPrint("streamer thread STARTED");

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
				
				if (m_recalcQuadrantsInViewNeeded && (int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
				{
					recalcQuadrantsInView();
					m_recalcQuadrantsInViewNeeded = false;
				}
				
				Globals()->Client()->Sync(isInEditor(), Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
				
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

	Globals()->debugPrint("streamer thread STOPPED");

	m_streamerThreadRunning = false;
}

Transform3D GDN_TheWorld_Viewer::getInternalGlobalTransform(void)
{
	Transform3D gt = get_global_transform();
	return gt;
}
