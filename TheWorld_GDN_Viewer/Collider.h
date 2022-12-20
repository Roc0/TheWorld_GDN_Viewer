#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <SceneTree.hpp>
#include <Viewport.hpp>

namespace godot
{
	class Collider
	{
	public:
		Collider(void);
		~Collider(void);

		void init(Node* attachedNode, int64_t initialLayer, int64_t initialMask);
		void deinit(void);

	private:
		bool m_initialized;
		RID m_shapeRID;
		RID m_bodyRID;
		Transform m_terrainTransform;
	};
}


