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

#include "GDN_TheWorld_Globals.h"
#include <MapManager.h>

namespace godot
{
	class MeshCache;
	class QuadTree;
	class Chunk;
	class GDN_TheWorld_Globals;
	class GDN_TheWorld_Camera;

	typedef std::map<TheWorld_MapManager::MapManager::QuadrantId, std::unique_ptr<QuadTree>> MapQuadTree;

#define MIN_MAP_SCALE 0.01F
#define TIME_INTERVAL_BETWEEN_DUMP 0	// secs, 0 = diasble periodic dump

	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System
	class GDN_TheWorld_Viewer : public Spatial
	{
		GODOT_CLASS(GDN_TheWorld_Viewer, Spatial)

	public:
		GDN_TheWorld_Viewer();
		~GDN_TheWorld_Viewer();
		bool init(void);
		void deinit(void);

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
		Spatial* getWorldNode(void);
		MeshCache* getMeshCache(void) { return m_meshCache.get(); }
		//void getPartialAABB(AABB& aabb, int firstWorldVertCol, int lastWorldVertCol, int firstWorldVertRow, int lastWorldVertRow, int step);
		Transform internalTransformGlobalCoord(void);
		Transform internalTransformLocalCoord(void);
		void setMapScale(Vector3 mapScaleVector);
		void setDumpRequired(void) { m_dumpRequired = true; }
		void dump(void);
		void dumpRecurseIntoChildrenNodes(Array nodes, int level);
		void setCameraChunk(Chunk* chunk, QuadTree* quadTree);
		//Transform getCameraChunkGlobalTransformOfAABB(void);
		AABB getCameraChunkLocalAABB(void);
		AABB getCameraChunkLocalDebugAABB(void);
		Transform getCameraChunkMeshGlobalTransformApplied(void);
		Transform getCameraChunkDebugMeshGlobalTransformApplied(void);
		String getCameraChunkId(void);
		int getNumSplits(void);
		int getNumJoins(void);
		int getNumChunks(void);
		GDN_TheWorld_Globals::ChunkDebugMode getRequiredChunkDebugMode(void)
		{
			return m_requiredChunkDebugMode;
		}
		String getChunkDebugModeStr(void);
		bool getDebugVisibility(void) { return m_debugVisibility; }
		String getDebugDrawMode(void);
		//ShaderTerrainData& getShaderTerrainData(void) { return m_shaderTerrainData; }
		bool useVisualServer(void) { return m_useVisualServer; }

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
		TheWorld_MapManager::MapManager::Quadrant* loadWorldData(float& x, float& z, int level, int numWorldVerticesPerSize);
		void printKeyboardMapping(void);

	private:
		bool m_initialized;
		bool m_useVisualServer;
		bool m_ctrlPressed;
		bool m_firstProcess;
		int64_t m_timeElapsedFromLastDump;
		bool m_initialWordlViewerPosSet;
		bool m_dumpRequired;
		Vector3 m_mapScaleVector;

		bool m_debugVisibility;
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
		int m_worldViewerLevel;		// actually world viewer manage one level at the time, otherwise we should have multiple quadtrees
		GDN_TheWorld_Camera* m_worldCamera;
		Chunk* m_cameraChunk;
		
		// World Data
		MapQuadTree m_mapQuadTree;
		int m_numWorldVerticesPerSize;

		// Shader Data
		//ShaderTerrainData m_shaderTerrainData;

		// Node cache
		GDN_TheWorld_Globals* m_globals;
	};

}

//	/root
//		/Main										created in Godot editor
//			/GDN_TheWorld_MainNode					Only to startup the viewer
//			/GDN_TheWorld_Globals
//			/TheWorld_Main							created in Godot editor
//				/GDN_TheWorld_Viewer
//				/GDN_TheWorld_Camera
