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

	typedef std::map<QuadrantPos, std::unique_ptr<QuadTree>> MapQuadTree;

#define STREAMER_SLEEP_TIME	10
#define FAR_HORIZON_MULTIPLIER	50

#define MIN_MAP_SCALE 0.01F
#define TIME_INTERVAL_BETWEEN_DUMP 0			// secs, 0 = diasble periodic dump
#define TIME_INTERVAL_BETWEEN_STATISTICS 500	// ms, 0 = diasble periodic dump

	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System
	class GDN_TheWorld_Viewer : public Spatial, public TheWorld_ClientServer::ClientCallback
	{
		GODOT_CLASS(GDN_TheWorld_Viewer, Spatial)

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

		GDN_TheWorld_Globals* Globals(bool useCache = true);
		void resetInitialWordlViewerPos(float x, float z, float cameraDistanceFromTerrain, int level, int chunkSizeShift, int heightmapResolutionShift);
		bool initialWordlViewerPosSet(void)
		{
			return m_initialWordlViewerPosSet;
		}
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
		void trackChunk(bool b)
		{
			m_trackChunk = b;
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
		QuadTree* getQuadTree(QuadrantPos pos);
		Chunk* getChunkAt(QuadrantPos pos, Chunk::ChunkPos chunkPos, enum class Chunk::DirectionSlot dir);
		Chunk* getChunkAt(Chunk* chunk, enum class Chunk::DirectionSlot dir);
		Chunk* getTrackedChunk(void);
		String getTrackedChunkStr(void);

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

	private:
		bool m_initialized;
		bool m_useVisualServer;
		bool m_ctrlPressed;
		bool m_firstProcess;
		int64_t m_timeElapsedFromLastDump;
		bool m_initialWordlViewerPosSet;
		bool m_dumpRequired;
		bool m_trackChunk;
		Chunk* lastTrackedChunk;
		//Vector3 m_mapScaleVector;

		// Statistics data
		int m_numProcessExecution;
		long long m_duration;
		int m_averageProcessDuration;
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
		// Statistics data

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
		QuadTree* m_cameraQuadTree;
		
		// World Data
		MapQuadTree m_mapQuadTree;
		std::recursive_mutex m_mtxQuadTree;
		QuadrantPos m_computedCameraQuadrantPos;
		bool m_refreshMapQuadTree;
		int m_numWorldVerticesPerSize;
		int m_worldViewerLevel;		// actually world viewer manage one level at the time, otherwise we should have multiple quadtrees
		size_t m_numVisibleQuadrantOnPerimeter;
		size_t m_numCacheQuadrantOnPerimeter;
		// Node cache
		GDN_TheWorld_Globals* m_globals;

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
