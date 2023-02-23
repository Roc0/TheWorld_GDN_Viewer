#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Spatial.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>
#include <Mesh.hpp>
#include <ShaderMaterial.hpp>
#include <Image.hpp>
#include <Texture.hpp>

#include <map>
#include <memory>
#include <thread>

#include "GDN_TheWorld_Globals.h"
#include "QuadTree.h"
//#include <MapManager.h>

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
	class GDN_TheWorld_Viewer : public Spatial, public TheWorld_ClientServer::ClientCallback
	{
		GODOT_CLASS(GDN_TheWorld_Viewer, Spatial)

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

	public:
		GDN_TheWorld_Viewer();
		~GDN_TheWorld_Viewer();
		bool init(void);
		bool canDeinit(void);
		void preDeinit(void);
		void deinit(void);

		void replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply);

		static void _register_methods();

		//
		// Godot Standard Functions
		//
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _physics_process(float _delta);
		void _input(const Ref<InputEvent> event);
		void _notification(int p_what);
		void _process_impl(float _delta, GDN_TheWorld_Camera* activeCamera);


		GDN_TheWorld_Globals* Globals(bool useCache = true);
		godot::GDN_TheWorld_Edit* EditModeUIControl(bool useCache = true);
		void createEditModeUI(void);
		void resetInitialWordlViewerPos(float x, float z, float cameraDistanceFromTerrain, int level, int chunkSizeShift, int heightmapResolutionShift);
		bool initialWordlViewerPosSet(void)
		{
			return m_initialWordlViewerPosSet;
		}
		void recalcQuadrantsInView(void);
		Spatial* getWorldNode(void);
		MeshCache* getMeshCache(void)
		{
			return m_meshCache.get(); 
		}
		Transform getInternalGlobalTransform(void);
		void setDumpRequired(void)
		{
			m_dumpRequired = true; 
		}
		void dump(void);
		void dumpRecurseIntoChildrenNodes(Array nodes, int level);
		void setCameraChunk(Chunk* chunk, QuadTree* quadTree);
		AABB getCameraChunkLocalAABB(void);
		AABB getCameraChunkLocalDebugAABB(void);
		Transform getCameraChunkGlobalTransformApplied(void);
		Transform getCameraChunkDebugGlobalTransformApplied(void);
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
		void refreshQuadTreeStatistics(void);
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
			return m_mouseHit;
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
			if (m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return std::string(m_mouseHitChunk->getQuadTree()->getQuadrant()->getPos().getName() + " " + m_mouseHitChunk->getIdStr()).c_str();
				//return m_mouseHitChunk->getIdStr().c_str();
			}
			else
				return "";
		}
		godot::Vector3 getMouseChunkHitPos(void)
		{
			if (m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return Vector3(m_mouseHitChunk->getLowerXInWUsGlobal(), 0, m_mouseHitChunk->getLowerZInWUsGlobal());
			}
			else
				return Vector3();
		}
		float getMouseChunkHitSize(void)
		{
			if (m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return m_mouseHitChunk->getChunkSizeInWUs();
			}
			else
				return 0.0;
		}

		float getMouseChunkHitDistFromCam(void)
		{
			if (m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
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

	private:
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

	private:
		bool m_initialized;
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
		int m_numinitializedVisibleQuadrant;
		TheWorld_Utils::TimerMs m_streamingTime;
		// Statistics data

		bool m_trackMouse;
		bool m_editMode;
		bool m_editModeHudVisible;
		int64_t m_timeElapsedFromLastMouseTrack;
		godot::Vector3 m_mouseHit;
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

		// streamer thread
		std::thread m_streamerThread;
		bool m_streamerThreadRequiredExit;
		bool m_streamerThreadRunning;
	};

}

//	/root
//		/Main										created in Godot editor
//			/GDN_TheWorld_MainNode					Only to startup the viewer
//			/GDN_TheWorld_Globals
//			/TheWorld_Main							created in Godot editor
//				/GDN_TheWorld_Viewer
//				/GDN_TheWorld_Camera
//				/EditModeUI
