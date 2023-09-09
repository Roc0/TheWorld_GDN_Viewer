#pragma once

#pragma warning(push, 0)
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#pragma warning(pop)

#include <map>
#include <memory>
#include <thread>

#include "GDN_TheWorld_Globals.h"
#include "QuadTree.h"
//#include <MapManager.h>

#define MAX_DEPTH_ON_PERIMETER	3
#define MAX_CACHE_ON_PERIMETER	2

namespace godot
{
	class MeshCache;
	class QuadTree;
	class Chunk;
	class GDN_TheWorld_Globals;
	class GDN_TheWorld_Camera;
	class GDN_TheWorld_Edit;

	typedef std::map<QuadrantPos, std::unique_ptr<QuadTree>> MapQuadTree;

#define STREAMER_SLEEP_TIME	10
#define FAR_HORIZON_MULTIPLIER	50

#define MIN_MAP_SCALE 0.01F
#define TIME_INTERVAL_BETWEEN_DUMP 0			// secs, 0 = diasble periodic dump
#define TIME_INTERVAL_BETWEEN_STATISTICS 100	// ms, 0 = diasble periodic dump
#define TIME_INTERVAL_BETWEEN_MOUSE_TRACK 250	// ms

	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System
	// X growing on East direction
	// Z growing on South direction
	class GDN_TheWorld_Viewer : public Node3D, public TheWorld_ClientServer::ClientCallback
	{
		GDCLASS(GDN_TheWorld_Viewer, Node3D)

		enum Margin {
			MARGIN_LEFT,
			MARGIN_TOP,
			MARGIN_RIGHT,
			MARGIN_BOTTOM
		};

		friend class GDN_TheWorld_Edit;
		friend class Chunk;
		//friend class QuadTree;
		//friend class ShaderTerrainData;

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _init(void);

	public:
		GDN_TheWorld_Viewer();
		~GDN_TheWorld_Viewer();
		bool initRequired(void)
		{
			return m_initRequired;
		};
		void initRequired(bool b)
		{
			m_initRequired = b;
		};
		bool init(void);
		bool canDeinit(void);
		void preDeinit(void);
		void deinit(void);

		void replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply);

		void setDesideredLookDev(enum class ShaderTerrainData::LookDev desideredLookDev)
		{
			if (m_desiderdLookDev != desideredLookDev)
			{
				m_desiderdLookDev = desideredLookDev;
				m_desideredLookDevChanged = true;
			}
		}

		enum class ShaderTerrainData::LookDev getDesideredLookDev(void)
		{
			return m_desiderdLookDev;
		}

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;
		
		void _process_impl(double _delta, Camera3D* activeCamera);

		//Ref<Material> getRegularMaterial(void);
		//Ref<Material> getLookDevMaterial(void);
		godot::Plane getShaderParamGroundUVScale()
		{
			return m_shaderParam_groundUVScale;
		}
		bool getShaderParamDepthBlening()
		{
			return m_shaderParam_depthBlending;
		}
		bool getShaderParamTriplanar()
		{
			return m_shaderParam_triplanar;
		}
		godot::Plane getShaderParamTileReduction()
		{
			return m_shaderParam_tileReduction;
		}
		float getShaderParamGlobalmapBlendStart()
		{
			return m_shaderParam_globalmapBlendStart;
		}
		float getShaderParamGlobalmapBlendDistance()
		{
			return m_shaderParam_globalmapBlendDistance;
		}
		godot::Plane getShaderParamColormapOpacity()
		{
			return m_shaderParam_colormapOpacity;
		}

		void setShaderParam(godot::String name, godot::Variant value);
		void debugPrint(String message)
		{
			Globals()->debugPrint(message);
		}
		void errorPrint(String message)
		{
			Globals()->errorPrint(message);
		}
		void warningPrint(String message)
		{
			Globals()->warningPrint(message);
		}
		void infoPrint(String message)
		{
			Globals()->infoPrint(message);
		}
		void print(String message)
		{
			Globals()->print(message);
		}
		void setEditorInterface(godot::Node* editorInterface)
		{
			m_editorInterface = (godot::EditorInterface*)editorInterface;
		}
		void toggleTrackMouse(void);
		void toggleEditMode(void);
		void toggleDebugVisibility(void);
		void rotateChunkDebugMode(void);
		void rotateDrawingMode(void);
		bool getTrackMouseState(void)
		{
			return m_trackMouse;
		}
		void toggleQuadrantSelected(void);
		godot::Camera3D* getCamera(void);
		godot::Camera3D* getCameraInEditor(void);
		void setEditorCamera(godot::Camera3D* editorCamera);
		void setDepthQuadOnPerimeter(int depth);
		int getDepthQuadOnPerimeter(void);
		void setCacheQuadOnPerimeter(int cache);
		int getCacheQuadOnPerimeter(void);
		String getInfoCamera(void);
		int getCameraProjectionMode(void);
		GDN_TheWorld_Globals* Globals(bool useCache = true);
		GDN_TheWorld_Camera* CameraNode(bool useCache = true);
		godot::GDN_TheWorld_Edit* EditModeUIControl(bool useCache = true);
		godot::Control* getOrCreateEditModeUIControl(void);
		void createEditModeUI(void);
		void deployWorld(float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll, int level, int chunkSizeShift, int heightmapResolutionShift);
		void undeployWorld(void);
		bool initialWordlViewerPosSet(void)
		{
			return m_initialWordlViewerPosSet;
		}
		void recalcQuadrantsInView(void);
		Node3D* getWorldNode(void);
		MeshCache* getMeshCache(void)
		{
			return m_meshCache.get(); 
		}
		Transform3D getInternalGlobalTransform(void);
		void setDumpRequired(void)
		{
			m_dumpRequired = true; 
		}
		void dump(void);
		void dumpRecurseIntoChildrenNodes(Array nodes, int level);
		void setCameraChunk(Chunk* chunk, QuadTree* quadTree);
		AABB getCameraChunkLocalAABB(void);
		AABB getCameraChunkLocalDebugAABB(void);
		Transform3D getCameraChunkGlobalTransformApplied(void);
		Transform3D getCameraChunkDebugGlobalTransformApplied(void);
		String getCameraChunkId(void);
		String getCameraQuadrantName(void);
		int getNumSplits(void);
		int getNumJoins(void);
		int getNumChunks(void);
		int getNumActiveChunks(void);
		int getProcessDuration(void);
		int getRefreshMapQuadDuration(void);
		int getUpdateQuads1Duration(void);
		int getUpdateQuads2Duration(void);
		int getUpdateQuads3Duration(void);
		int getUpdateChunksDuration(void);
		int getUpdateMaterialParamsDuration(void);
		int getMouseTrackHitDuration(void);
		int getProcessNotOwnsLock(void);
		int getNumQuadrant(void);
		int getNuminitializedQuadrant(void);
		int getNumVisibleQuadrant(void);
		int getNuminitializedVisibleQuadrant(void);
		int getNumEmptyQuadrant(void);
		int getNumFlushedQuadrant(void);
		void refreshQuadTreeStatistics(void);
		void updateMaterialParamsForEveryQuadrant(void);
		GDN_TheWorld_Globals::ChunkDebugMode getRequiredChunkDebugMode(void)
		{
			return m_requiredChunkDebugMode;
		}
		String getChunkDebugModeStr(void);
		bool getDebugContentVisibility(void)
		{
			return m_debugContentVisibility; 
		}
		String getDebugDrawMode(void);
		bool useVisualServer(void)
		{
			return m_useVisualServer; 
		}
		void forceRefreshMapQuadTree(void) 
		{
			m_refreshMapQuadTree = true; 
		}
		void getAllQuadrantPos(std::vector<QuadrantPos>& allQuandrantPos);
		QuadTree* getQuadTree(QuadrantPos pos);
		Chunk* getActiveChunkAt(QuadrantPos pos, Chunk::ChunkPos chunkPos, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter);
		Chunk* getActiveChunkAt(Chunk* chunk, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter);
		bool terrainShiftPermitted(void);
		godot::Vector3 getMouseHit(void)
		{
			if (m_mouseTrackedOnTerrain)
				return m_mouseHit;
			else
				return godot::Vector3();
		}
		float getMouseHitDistanceFromCamera(void)
		{
			if (m_mouseTrackedOnTerrain)
			{
				godot::Camera3D* camera = getCamera();
				return camera->get_global_transform().origin.distance_to(m_mouseHit);
			}
			else
				return 0.0f;
		}
		godot::String _getMouseQuadrantHitName(void)
		{
			return getMouseQuadrantHitName().c_str();
		}
		std::string getMouseQuadrantHitName(void)
		{
			return m_mouseQuadrantHitName;
		}
		godot::String _getMouseQuadrantHitTag(void)
		{
			return getMouseQuadrantHitTag().c_str();
		}
		std::string getMouseQuadrantHitTag(void)
		{
			return m_mouseQuadrantHitTag;
		}
		godot::Vector3 getMouseQuadrantHitPos(void)
		{
			return m_mouseQuadrantHitPos;
		}
		float getMouseQuadrantHitSize(void)
		{
			return m_mouseQuadrantHitSize;
		}
		godot::String getMouseChunkHitId(void)
		{
			if (m_mouseTrackedOnTerrain && m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return std::string(m_mouseHitChunk->getQuadTree()->getQuadrant()->getPos().getName() + " " + m_mouseHitChunk->getIdStr()).c_str();
				//return m_mouseHitChunk->getIdStr().c_str();
			}
			else
				return "";
		}
		godot::Vector3 getMouseChunkHitPos(void)
		{
			if (m_mouseTrackedOnTerrain && m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return Vector3(m_mouseHitChunk->getLowerXInWUsGlobal(), 0, m_mouseHitChunk->getLowerZInWUsGlobal());
			}
			else
				return Vector3();
		}
		float getMouseChunkHitSize(void)
		{
			if (m_mouseTrackedOnTerrain && m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return m_mouseHitChunk->getChunkSizeInWUs();
			}
			else
				return 0.0;
		}

		float getMouseChunkHitDistFromCam(void)
		{
			if (m_mouseTrackedOnTerrain && m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return m_mouseHitChunk->getDistanceFromCamera();
			}
			else
				return 0.0;
		}

		bool trackMouse(void)
		{
			return m_trackMouse;
		}
		void setMouseHitChunk(Chunk* chunk)
		{
			m_mouseHitChunk = chunk;
		}
		//void setMouseHitQuadTree(QuadTree* quadTree)
		//{
		//	m_mouseHitQuadTree = quadTree;
		//}

		bool editMode(void)
		{
			return m_editMode;
		}

		//void GenerateHeigths(void);

		std::recursive_mutex& getMainProcessingMutex(void)
		{
			return m_mtxQuadTreeAndMainProcessing;
		}

		std::string to_string(godot::String s)
		{
			//godot::PackedByteArray byteStream = s.to_utf8_buffer();
			const char* str = s.utf8().get_data();
			std::string ret = str;
			//char* str = s.alloc_c_string();
			//std::string ret = str;
			//godot::api->godot_free(str);
			return ret;
		}

		bool isInEditor(void)
		{
			return m_isInEditor;
		}

	private:
		void _findChildNodes(godot::Array& foundNodes, godot::Array& searchNodes, String searchClass);
		void onTransformChanged(void);
		GDN_TheWorld_Camera* WorldCamera(bool useCache = true)
		{ 
			return m_worldCamera;
		};
		void assignWorldCamera(GDN_TheWorld_Camera* camera)
		{
			m_worldCamera = camera;
		};
		void printKeyboardMapping(void);
		void streamer(void);
		void streamingQuadrantStuff(void);
		QuadrantPos getQuadrantSelForEdit(QuadTree** quadTreeSel);
		godot::Camera3D* m_editorCamera;

	private:
		bool m_initialized;
		bool m_quitting;
		bool m_initRequired;
		bool m_isInEditor;
		bool m_ready;
		bool m_useVisualServer;
		bool m_altPressed;
		bool m_ctrlPressed;
		bool m_shiftPressed;
		bool m_firstProcess;
		int64_t m_timeElapsedFromLastDump;
		bool m_initialWordlViewerPosSet;
		bool m_dumpRequired;
		Chunk* lastTrackedChunk;
		//Vector3 m_mapScaleVector;
		enum class ShaderTerrainData::LookDev m_desiderdLookDev;
		bool m_desideredLookDevChanged;

		// Statistics data
		int m_numProcessExecution;
		long long m_processDuration;
		int m_averageProcessDuration;
		int m_numRefreshMapQuad;
		long long m_RefreshMapQuadDuration;
		int m_averageRefreshMapQuadDuration;
		int m_numUpdateQuads1;
		long long m_updateQuads1Duration;
		int m_averageUpdateQuads1Duration;
		int m_numUpdateQuads2;
		long long m_updateQuads2Duration;
		int m_averageUpdateQuads2Duration;
		int m_numUpdateQuads3;
		long long m_updateQuads3Duration;
		int m_averageUpdateQuads3Duration;
		int m_numUpdateChunks;
		long long m_updateChunksDuration;
		int m_averageUpdateChunksDuration;
		int m_numUpdateMaterialParams;
		long long m_updateMaterialParamsDuration;
		int m_averageUpdateMaterialParamsDuration;
		int m_numMouseTrackHit;
		long long m_mouseTrackHitDuration;
		int m_averageMouseTrackHitDuration;
		int64_t m_timeElapsedFromLastStatistic;
		int m_numSplits;
		int m_numJoins;
		int m_numChunks;
		int m_numActiveChunks;
		int m_numProcessNotOwnsLock;
		int m_numQuadrant;
		int m_numinitializedQuadrant;
		int m_numVisibleQuadrant;
		int m_numEmptyQuadrant;
		int m_numFlushedQuadrant;
		int m_numinitializedVisibleQuadrant;
		TheWorld_Utils::TimerMs m_streamingTime;
		// Statistics data

		bool m_trackMouse;
		bool m_editMode;
		int64_t m_timeElapsedFromLastMouseTrack;
		godot::Vector3 m_mouseHit;
		bool m_mouseTrackedOnTerrain;
		std::string m_mouseQuadrantHitName;
		std::string m_mouseQuadrantHitTag;
		float m_mouseQuadrantHitSize;
		godot::Vector3 m_mouseQuadrantHitPos;
		QuadrantPos m_quadrantHitPos;
		QuadrantPos m_quadrantSelPos;
		Chunk* m_mouseHitChunk;
		//QuadTree* m_mouseHitQuadTree;

		bool m_debugContentVisibility;
		bool m_updateTerrainVisibilityRequired;
		
		enum class GDN_TheWorld_Globals::ChunkDebugMode m_currentChunkDebugMode;
		enum class GDN_TheWorld_Globals::ChunkDebugMode m_requiredChunkDebugMode;
		bool m_updateDebugModeRequired;
		
		Viewport::DebugDraw m_debugDraw;

		bool m_refreshRequired;

		//RID m_viewFrustumMeshInstance;
		//RID m_viewFrustumMeshRID;
		//Ref<Mesh> m_viewFrustumMesh;

		// MeshCache
		std::unique_ptr<MeshCache> m_meshCache;

		// Viewer (Camera)
		GDN_TheWorld_Camera* m_worldCamera;
		Chunk* m_cameraChunk;
		//QuadTree* m_cameraQuadTree;
		
		// World Data
		MapQuadTree m_mapQuadTree;	// Actually only 1 level is managed (to manage more levels we shold have a map for each level)
		std::recursive_mutex m_mtxQuadTreeAndMainProcessing;
		std::vector<QuadrantPos> m_toErase;
		QuadrantPos m_computedCameraQuadrantPos;
		bool m_recalcQuadrantsInViewNeeded;
		bool m_refreshMapQuadTree;
		int m_numWorldVerticesPerSize;
		int m_worldViewerLevel;		// actually world viewer manage one level at the time, otherwise we should have multiple quadtrees
		size_t m_numVisibleQuadrantOnPerimeter;
		size_t m_numCacheQuadrantOnPerimeter;
		// Node cache
		GDN_TheWorld_Globals* m_globals;
		GDN_TheWorld_Edit* m_editModeUIControl;
		//Ref<ShaderMaterial> m_regularMaterial;
		//Ref<ShaderMaterial> m_lookDevMaterial;
		
		// Shader parameters
		godot::Plane m_shaderParam_groundUVScale;
		bool m_shaderParam_depthBlending;
		bool m_shaderParam_triplanar;
		godot::Plane m_shaderParam_tileReduction;
		float m_shaderParam_globalmapBlendStart;
		float m_shaderParam_globalmapBlendDistance;
		godot::Plane  m_shaderParam_colormapOpacity;
		bool m_shaderParamChanged;

		// streamer thread
		std::thread m_streamerThread;
		bool m_streamerThreadRequiredExit;
		bool m_streamerThreadRunning;

		// Editor stuff
		godot::EditorInterface* m_editorInterface;
		size_t m_depthQuadOnPerimeter;
		size_t m_cacheQuadOnPerimeter;
	};

}

//	/root
//		...											created in Godot editor
//			/TheWorld_Main							created in Godot editor
//				/GDN_TheWorld_MainNode				Only to startup the viewer and globals
//				/GDN_TheWorld_Globals
//				/GDN_TheWorld_Viewer
//				/GDN_TheWorld_Camera
//				/EditModeUI
