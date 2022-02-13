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
		void _input(const Ref<InputEvent> event);

		GDN_TheWorld_Globals* Globals(bool useCache = true);
		void resetInitialWordlViewerPos(float x, float z, int level);
		Spatial* getWorldNode(void);
	
	private:
		GDN_TheWorld_Camera* WorldCamera(bool useCache = true)
		{ 
			return m_worldCamera;
		};
		void assignWorldCamera(GDN_TheWorld_Camera* camera)
		{
			m_worldCamera = camera;
		};
		void loadWorldData(float& x, float& z, int level);
		Transform internalTransformGlobalCoord(void);

	private:
		bool m_initialized;
		
		std::unique_ptr<MeshCache> m_meshCache;
		std::unique_ptr<QuadTree> m_quadTree;

		Vector3 m_mapScaleVector;

		// Viewer (Camera)
		Vector3 m_worldViewerPos;	// in local coordinate of WorldNode
		int m_worldViewerLevel;
		GDN_TheWorld_Camera* m_worldCamera;
		
		// World Data
		std::vector<TheWorld_MapManager::SQLInterface::GridVertex> m_worldVertices;
		int m_numWordlVerticesX;
		int m_numWordlVerticesZ;

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
