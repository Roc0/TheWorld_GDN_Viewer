//#include "pch.h"
#include "Collider.h"
#include "QuadTree.h"
#include "GDN_TheWorld_Viewer.h"

#include <assert.h>

#include <PhysicsServer.hpp>
#include <World.hpp>

namespace godot
{
	Collider::Collider(QuadTree* quadTree)
	{
		m_initialized = false;
		m_quadTree = quadTree;
		m_shapeRID = RID();
		m_bodyRID = RID();
	}
	
	Collider::~Collider(void)
	{
		deinit();
	}

	void Collider::init(Node* attachedNode, int64_t initialLayer, int64_t initialMask)
	{
		assert(attachedNode != nullptr);

		PhysicsServer* ps = PhysicsServer::get_singleton();
		m_shapeRID = ps->shape_create(PhysicsServer::SHAPE_HEIGHTMAP);
		m_bodyRID = ps->body_create(PhysicsServer::BODY_MODE_STATIC);
		ps->body_set_collision_layer(m_bodyRID, initialLayer);
		ps->body_set_collision_mask(m_bodyRID, initialMask);

		// SUPER DEBUGRIC - This is an attempt to workaround https ://github.com/godotengine/godot/issues/24390
		ps->body_set_ray_pickable(m_bodyRID, true);

		// Initial data - This is a workaround to https://github.com/godotengine/godot/issues/25304
		PoolRealArray arr;
		arr.resize(4);
		arr.set(0, 0);
		arr.set(1, 0);
		arr.set(2, 0);
		arr.set(3, 0);
		Dictionary data;
		data["width"] = 2;			// data["map_width"];	
		data["depth"] = 2;			// data["map_depth"];
		data["heights"] = arr;		// data["map_data"];
		data["min_height"] = -1;	// ???
		data["max_height"] = 1;		// ???
		ps->shape_set_data(m_shapeRID, data);

		ps->body_add_shape(m_bodyRID, m_shapeRID);

		// This makes collision hits report the provided object as `collider`
		ps->body_attach_object_instance_id(m_bodyRID, attachedNode->get_instance_id());
			
		m_initialized = true;
	}

	void Collider::deinit(void)
	{
		if (m_initialized)
		{
			PhysicsServer* ps = PhysicsServer::get_singleton();

			if (m_bodyRID != RID())
				ps->free_rid(m_bodyRID);
			if (m_shapeRID != RID())
				ps->free_rid(m_shapeRID);

			m_initialized = false;
		}
	}

	void Collider::enterWorld(void)
	{
		Ref<World> world = m_quadTree->Viewer()->get_world();
		if (world != nullptr && world.is_valid())
		{
			PhysicsServer* ps = PhysicsServer::get_singleton();
			ps->body_set_space(m_bodyRID, world->get_space());
		}
	}

	void Collider::exitWorld(void)
	{
		PhysicsServer* ps = PhysicsServer::get_singleton();
		ps->body_set_space(m_bodyRID, RID());
	}

	void Collider::setData(void)
	{
		std::vector<TheWorld_Utils::GridVertex>& gridVertices = m_quadTree->getQuadrant()->getGridVertices();

		int numVerticesPerSize = m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize();
		int areaSize = numVerticesPerSize * numVerticesPerSize;
		if (areaSize != m_quadTree->getQuadrant()->getHeights().size())
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Size of Heigths inconsistent, areaSize=" + std::to_string(areaSize) + " Heigths size=" + std::to_string(m_quadTree->getQuadrant()->getHeights().size())).c_str()));

		Dictionary data;
		data["width"] = numVerticesPerSize;			// data["map_width"];	
		data["depth"] = numVerticesPerSize;			// data["map_depth"];
		data["heights"] = m_quadTree->getQuadrant()->getHeights();					// data["map_data"];
		Vector3 startPoint = m_quadTree->getQuadrant()->getGlobalCoordAABB().position;
		Vector3 endPoint = startPoint + m_quadTree->getQuadrant()->getGlobalCoordAABB().size;
		data["min_height"] = startPoint.y;			// ???
		data["max_height"] = endPoint.y;			// ???

		PhysicsServer* ps = PhysicsServer::get_singleton();
		ps->shape_set_data(m_shapeRID, data);

		updateTransform();
	}
	void Collider::onGlobalTransformChanged(void)
	{
		updateTransform();
	}

	void Collider::updateTransform()
	{
		// set body to the center of the quadrant
		//int numVerticesPerSize = m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize();
		//Transform t = igt * Transform(Basis(), 0.5 * Vector3((float)numVerticesPerSize, 0, (float)numVerticesPerSize));
		float sizeInWU = m_quadTree->getQuadrant()->getPos().getSizeInWU();
		Transform igt = m_quadTree->getInternalGlobalTransform();
		Transform t(Basis(), 0.5 * Vector3((float)sizeInWU, 0, (float)sizeInWU));
		Transform t1 = igt * t;
		PhysicsServer* ps = PhysicsServer::get_singleton();
		ps->body_set_state(m_bodyRID, PhysicsServer::BODY_STATE_TRANSFORM, t1);
	}
}

