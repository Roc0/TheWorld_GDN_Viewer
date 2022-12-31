#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <SceneTree.hpp>
#include <Viewport.hpp>

namespace godot
{
	class QuadTree;

	class Collider
	{
	public:
		Collider(QuadTree* quadTree);
		~Collider(void);

		void init(Node* attachedNode, int64_t initialLayer, int64_t initialMask);
		void deinit(void);

		void enterWorld(void);
		void exitWorld(void);
		void setData(void);
		void updateTransform();
		void onGlobalTransformChanged(void);
		void onCameraTransformChanged(void);
		void setTransform(Transform t);
		Transform getTransform(void);

	private:
		inline float getColliderAltitude(void);

	private:
		bool m_initialized;
		bool m_dataSet;
		QuadTree* m_quadTree;
		RID m_shapeRID;
		RID m_bodyRID;
		Transform m_colliderTransform;
		Transform m_cameraGlobalTranform;
	};
}
