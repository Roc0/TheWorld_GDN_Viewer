//#include "pch.h"
#include "Collider.h"
#include "QuadTree.h"
#include "GDN_TheWorld_Viewer.h"

#pragma warning (push, 0)
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#pragma warning (pop)

#include <assert.h>


//#define MOCK_COLLIDER true
#define MOCK_COLLIDER false

namespace godot
{
	Collider::Collider(QuadTree* quadTree)
	{
		m_initialized = false;
		m_dataSet = false;
		m_quadTree = quadTree;
		m_shapeRID = RID();
		m_bodyRID = RID();
	}
	
	Collider::~Collider(void)
	{
		deinit();
	}

	void Collider::init(Node* attachedNode, uint32_t initialLayer, uint32_t initialMask)
	{
		if (MOCK_COLLIDER)
			return;

		assert(!m_initialized);
		assert(attachedNode != nullptr);

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		m_shapeRID = ps->heightmap_shape_create();
		m_bodyRID = ps->body_create();
		ps->body_set_mode(m_bodyRID, godot::PhysicsServer3D::BODY_MODE_STATIC);
		ps->body_set_collision_layer(m_bodyRID, initialLayer);
		ps->body_set_collision_mask(m_bodyRID, initialMask);

		// SUPERDEBUGRIC - This is an attempt to workaround https ://github.com/godotengine/godot/issues/24390
		ps->body_set_ray_pickable(m_bodyRID, false);

		// Initial data - This is a workaround to https://github.com/godotengine/godot/issues/25304
		PackedFloat32Array arr;
		arr.resize(4);
		arr.set(0, 0);
		arr.set(1, 0);
		arr.set(2, 0);
		arr.set(3, 0);
		Dictionary data;
		data["width"] = 2;			// data["map_width"];	
		data["depth"] = 2;			// data["map_depth"];
		data["heights"] = arr;		// data["map_data"];
		data["min_height"] = -1;		// ???
		data["max_height"] = 1;		// ???
		ps->shape_set_data(m_shapeRID, data);

		ps->body_add_shape(m_bodyRID, m_shapeRID);

		// This makes collision hits report the provided object as `collider`
		ps->body_attach_object_instance_id(m_bodyRID, attachedNode->get_instance_id());
			
		m_initialized = true;
	}

	void Collider::deinit(void)
	{
		if (MOCK_COLLIDER)
			return;

		if (m_initialized)
		{
			godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();

			if (m_bodyRID != RID())
				ps->free_rid(m_bodyRID);
			if (m_shapeRID != RID())
				ps->free_rid(m_shapeRID);

			m_dataSet = false;
			m_initialized = false;
		}
	}

	void Collider::enterWorld(void)
	{
		if (MOCK_COLLIDER)
			return;

		assert(m_initialized);
		if (!m_initialized)
			return;

		Ref<godot::World3D> world = m_quadTree->Viewer()->get_world_3d();
		if (world != nullptr && world.is_valid())
		{
			godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
			ps->body_set_space(m_bodyRID, world->get_space());
		}
	}

	void Collider::exitWorld(void)
	{
		if (MOCK_COLLIDER)
			return;

		//assert(m_initialized);
		if (!m_initialized)
			return;

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		ps->body_set_space(m_bodyRID, RID());
	}

	void Collider::setData(void)
	{
		if (MOCK_COLLIDER)
			return;

		if (!m_initialized)
			return;

		int numVerticesPerSize = m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize();
		int areaSize = numVerticesPerSize * numVerticesPerSize;
		//{
		//	TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4.1.1.1 ") + __FUNCTION__, "Collider::setData - gen empty heights");
		//	PoolRealArray& h = m_quadTree->getQuadrant()->getHeightsForCollider();
		//}
		PackedFloat32Array& heightsForCollider = m_quadTree->getQuadrant()->getHeightsForCollider();
		my_assert(areaSize == heightsForCollider.size());
		if (areaSize != heightsForCollider.size())
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Size of Heigths inconsistent, areaSize=" + std::to_string(areaSize) + " Heigths size=" + std::to_string(heightsForCollider.size())).c_str()));

		size_t size = heightsForCollider.size();
		Dictionary data;
		data["width"] = numVerticesPerSize;			// data["map_width"];	
		data["depth"] = numVerticesPerSize;			// data["map_depth"];
		data["heights"] = heightsForCollider;		// data["map_data"];
		Vector3 startPoint = m_quadTree->getQuadrant()->getGlobalCoordAABB().position;
		Vector3 endPoint = startPoint + m_quadTree->getQuadrant()->getGlobalCoordAABB().size;
		data["min_height"] = startPoint.y;			// ???
		//data["min_height"] = -endPoint.y;			// ???
		data["max_height"] = endPoint.y;			// ???

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4.1.1.2 ") + __FUNCTION__, "Collider::setData - ps->shape_set_data(m_shapeRID, data)");
			ps->shape_set_data(m_shapeRID, data);
		}

		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4.1.1.3 ") + __FUNCTION__, "Collider::setData - updateTransform");
			updateTransform();
		}
	}
	
	void Collider::onGlobalTransformChanged(void)
	{
		if (MOCK_COLLIDER)
			return;

		if (!m_initialized)
			return;

		updateTransform();
	}

	void Collider::onCameraTransformChanged(void)
	{
		if (MOCK_COLLIDER)
			return;

		if (!m_initialized)
			return;

		m_cameraGlobalTranform = m_quadTree->Viewer()->getCamera()->get_global_transform();
		float colliderAltitude = getColliderAltitude();
		Transform3D t = getTransform();
		t.origin.y = colliderAltitude;
		setTransform(t);
	}
	
	float Collider::getColliderAltitude(void)
	{
		float colliderAltitude = -(m_cameraGlobalTranform.origin.y / (float)2.34);
		colliderAltitude = 0;
		return colliderAltitude;
	}

	void Collider::updateTransform()
	{
		if (MOCK_COLLIDER)
			return;

		assert(m_initialized);
		if (!m_initialized)
			return;

		//int numVerticesPerSize = m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize();
		float sizeInWU = m_quadTree->getQuadrant()->getPos().getSizeInWU();
		float gridStepInWU = m_quadTree->getQuadrant()->getPos().getGridStepInWU();
		Transform3D igt = m_quadTree->getInternalGlobalTransform();
		//float axisScaleFactr = sqrtf(powf(gridStepInWU, (float)2.0) / (float)2.0);
		float axisScaleFactr = gridStepInWU;
		Camera3D* cam = m_quadTree->Viewer()->getCamera();
		m_cameraGlobalTranform = cam->get_global_transform();
		float colliderAltitude = getColliderAltitude();
		//Vector3 centerOfQuadrant = igt.origin + 0.5 * Vector3(sizeInWU, 0, sizeInWU);
		//Vector3 centerOfQuadrantDistFromCamera = cam->get_global_transform().origin - centerOfQuadrant;

		// set body to the center of the quadrant
		Transform3D tTranslate(Basis(), Vector3((float)0.5 * sizeInWU, (float)colliderAltitude, (float)0.5 * sizeInWU));
		Transform3D tScale(Basis().scaled(Vector3(axisScaleFactr, (float)1.0, axisScaleFactr)), Vector3(0, 0, 0));
		Transform3D t = tTranslate * tScale;

		m_colliderTransform = igt * t;
		setTransform(m_colliderTransform);
	}

	void Collider::setTransform(Transform3D t)
	{
		assert(m_initialized);
		if (!m_initialized)
			return;

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		ps->body_set_state(m_bodyRID, godot::PhysicsServer3D::BodyState::BODY_STATE_TRANSFORM, t);
		//ps->body_set_shape_transform(m_bodyRID, 0, t);
	}

	Transform3D Collider::getTransform(void)
	{
		assert(m_initialized);
		if (!m_initialized)
			return Transform3D();

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		Transform3D t = ps->body_get_state(m_bodyRID, PhysicsServer3D::BodyState::BODY_STATE_TRANSFORM);
		return t;
	}
}

