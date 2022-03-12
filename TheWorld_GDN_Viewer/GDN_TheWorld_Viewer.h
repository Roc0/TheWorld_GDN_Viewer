#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Spatial.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>

#include <memory>

#include <MapManager.h>

namespace godot
{
	class MeshCache;
	class QuadTree;
	class GDN_TheWorld_Globals;
	class GDN_TheWorld_Camera;

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
		void resetInitialWordlViewerPos(float x, float z, float cameraDistanceFromTerrain, int level);
		Spatial* getWorldNode(void);
		QuadTree* getQuadTree(void) { return m_quadTree.get(); }
		MeshCache* getMeshCache(void) { return m_meshCache.get(); }
		void getPartialAABB(AABB& aabb, int firstWorldVertCol, int lastWorldVertCol, int firstWorldVertRow, int lastWorldVertRow, int step);
		Transform internalTransformGlobalCoord(void);
		Transform internalTransformLocalCoord(void);
		void setMapScale(Vector3 mapScaleVector);
		void setDumpRequired(void) { m_dumpRequired = true; }
		void dump(void);

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
		void loadWorldData(float& x, float& z, int level);

	private:
		bool m_initialized;
		int64_t m_timeElapsedFromLastDump;
		bool m_initialWordlViewerPosSet;
		bool m_dumpRequired;
		
		std::unique_ptr<MeshCache> m_meshCache;
		std::unique_ptr<QuadTree> m_quadTree;

		Vector3 m_mapScaleVector;

		// Viewer (Camera)
		//Vector3 m_worldViewerPos;	// in local coordinate of WorldNode
		int m_worldViewerLevel;
		GDN_TheWorld_Camera* m_worldCamera;
		
		// World Data
		std::vector<TheWorld_MapManager::SQLInterface::GridVertex> m_worldVertices;
		int m_numWorldVerticesX;
		int m_numWorldVerticesZ;

		// Node cache
		GDN_TheWorld_Globals* m_globals;
	};

}

//	/root
//		/Main										created in Godot editor
//			/GDN_TheWorld_MainNode
//			/GDN_TheWorld_Globals
//			/TheWorld_Main							created in Godot editor
//				/GDN_TheWorld_Viewer
//				/GDN_TheWorld_Camera
