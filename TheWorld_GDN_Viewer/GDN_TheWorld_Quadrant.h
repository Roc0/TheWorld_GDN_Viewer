#pragma once
#include <Godot.hpp>
#include <Spatial.hpp>
#include <MeshInstance.hpp>

namespace godot
{
	class QuadTree;

	class GDN_Collider_MeshInstance : public MeshInstance
	{
		GODOT_CLASS(GDN_Collider_MeshInstance, MeshInstance)

	public:
		GDN_Collider_MeshInstance();
		~GDN_Collider_MeshInstance();
		void init(void);
		void deinit(void);

		static void _register_methods();

		//
		// Godot Standard Functions
		//
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);

	private:
		bool m_initialized;
	};

	class GDN_TheWorld_Quadrant : public Spatial
	{
		GODOT_CLASS(GDN_TheWorld_Quadrant, Spatial)
	public:
		GDN_TheWorld_Quadrant(void);
		~GDN_TheWorld_Quadrant(void);
		void init(QuadTree* quadTree);
		void deinit(void);
		void setColliderTransform(Transform t);
		Transform getColliderTransform(void);
		//void setDebugColliderMeshTransform(Transform t);
		//Transform getDebugColliderMeshTransform(void);
		//void showDebugColliderMesh(bool show = true);
		void onGlobalTransformChanged(void);

		//
		// Godot Standard Functions
		//
		static void _register_methods();
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _physics_process(float _delta);
		void _input(const Ref<InputEvent> event);
		void _notification(int p_what);

	private:
		//void createDebugColliderMeshInstance(void);

	private:
		QuadTree* m_quadTree;
		//GDN_Collider_MeshInstance* m_colliderMeshInstance;
		Transform m_lastCameraTransform;
		bool m_initialized;
	};
}

