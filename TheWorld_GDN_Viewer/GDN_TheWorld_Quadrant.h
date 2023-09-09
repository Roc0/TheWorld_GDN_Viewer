#pragma once

#pragma warning (push, 0)
#include <godot_cpp/classes/node3d.hpp>
#pragma warning (pop)

namespace godot
{
	class QuadTree;

	//class GDN_Collider_MeshInstance : public MeshInstance
	//{
	//	GODOT_CLASS(GDN_Collider_MeshInstance, MeshInstance)

	//public:
	//	GDN_Collider_MeshInstance();
	//	~GDN_Collider_MeshInstance();
	//	void init(void);
	//	void deinit(void);

	//	static void _register_methods();

	//	//
	//	// Godot Standard Functions
	//	//
	//	void _init(void); // our initializer called by Godot
	//	void _ready(void);
	//	void _process(float _delta);
	//	void _input(const Ref<InputEvent> event);

	//private:
	//	bool m_initialized;
	//};

	class GDN_TheWorld_Quadrant : public Node3D
	{
		GDCLASS(GDN_TheWorld_Quadrant, Node3D)

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _init(void);

	public:
		GDN_TheWorld_Quadrant(void);
		~GDN_TheWorld_Quadrant(void);
		
		void init(QuadTree* quadTree);
		void deinit(void);
		void setColliderTransform(Transform3D t);
		Transform3D getColliderTransform(void);
		//void setDebugColliderMeshTransform(Transform t);
		//Transform getDebugColliderMeshTransform(void);
		//void showDebugColliderMesh(bool show = true);
		void onGlobalTransformChanged(void);

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;

	private:
		//void createDebugColliderMeshInstance(void);

	private:
		bool m_quitting;
		bool m_ready;
		QuadTree* m_quadTree;
		//GDN_Collider_MeshInstance* m_colliderMeshInstance;
		Transform3D m_lastCameraTransform;
		bool m_initialized;
	};
}

