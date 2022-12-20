//#include "pch.h"
#include "Collider.h"

#include <assert.h>

#include <PhysicsServer.hpp>

namespace godot
{
	Collider::Collider(void)
	{
		m_initialized = false;
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
		ps->body_set_ray_pickable(m_bodyRID, false);

		// Initial data - This is a workaround to https://github.com/godotengine/godot/issues/25304
		PoolRealArray arr;
		arr.append(0);
		arr.append(0);
		arr.append(0);
		arr.append(0);
		Dictionary data;
		data["width"] = 2;			// data["map_width"] = 2;	
		data["depth"] = 2;			// data["map_depth"] = 2;
		data["heights"] = arr;		// data["map_data"] = arr;
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
			m_initialized = false;
		}
	}
}

