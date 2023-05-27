//#include "pch.h"

#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Quadrant.h"
#include "QuadTree.h"

namespace godot
{
	GDN_TheWorld_Quadrant::GDN_TheWorld_Quadrant()
	{
		m_quadTree = nullptr;
		//m_colliderMeshInstance = nullptr;
		m_initialized = false;
		m_ready = false;

		_init();
	}
	
	GDN_TheWorld_Quadrant::~GDN_TheWorld_Quadrant()
	{
		deinit();
	}

	void GDN_TheWorld_Quadrant::init(QuadTree* quadTree)
	{
		assert(m_initialized == false);

		m_quadTree = quadTree;
		//set_name(m_quadTree->getQuadrant()->getPos().getName().c_str());
		set_meta("QuadrantPos", m_quadTree->getQuadrant()->getPos().getIdStr().c_str());
		set_meta("QuadrantName", m_quadTree->getQuadrant()->getPos().getName().c_str());
		//set_meta("QuadrantTag", m_quadTree->getQuadrant()->getPos().getTag().c_str());	// wrong: tag change during movement
		set_meta("QuadrantOrig", Vector3(m_quadTree->getQuadrant()->getPos().getLowerXGridVertex(), 0, m_quadTree->getQuadrant()->getPos().getLowerZGridVertex()));
		set_meta("QuadrantStep", m_quadTree->getQuadrant()->getPos().getGridStepInWU());
		set_meta("QuadrantLevel", m_quadTree->getQuadrant()->getPos().getLevel());
		set_meta("QuadrantNumVert", m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize());
		set_meta("QuadrantSize", m_quadTree->getQuadrant()->getPos().getSizeInWU());
		m_initialized = true;
	}

	void GDN_TheWorld_Quadrant::deinit(void)
	{
		if (m_initialized)
		{
			m_quadTree = nullptr;
			//if (m_colliderMeshInstance)
			//{
			//	Node* parent = m_colliderMeshInstance->get_parent();
			//	if (parent)
			//		parent->remove_child(m_colliderMeshInstance);
			//	m_colliderMeshInstance->queue_free();
			//	m_colliderMeshInstance = nullptr;
			//}
			m_initialized = false;
		}
	}
		
	void GDN_TheWorld_Quadrant::_bind_methods()
	{
		ClassDB::bind_method(D_METHOD("set_collider_transform"), &GDN_TheWorld_Quadrant::setColliderTransform);
		ClassDB::bind_method(D_METHOD("get_collider_transform"), &GDN_TheWorld_Quadrant::getColliderTransform);
		//ClassDB::bind_method(D_METHOD("set_collider_mesh_transform"), &GDN_TheWorld_Quadrant::setDebugColliderMeshTransform);
		//ClassDB::bind_method(D_METHOD("get_collider_mesh_transform"), &GDN_TheWorld_Quadrant::getDebugColliderMeshTransform);
		//ClassDB::bind_method(D_METHOD("show_collider_mesh"), &GDN_TheWorld_Quadrant::showDebugColliderMesh);
	}

	void GDN_TheWorld_Quadrant::_init(void)
	{
	}

	void GDN_TheWorld_Quadrant::_ready(void)
	{
		m_ready = true;
	}

	void GDN_TheWorld_Quadrant::_input(const Ref<InputEvent>& event)
	{
	}

	void GDN_TheWorld_Quadrant::_notification(int p_what)
	{
		switch (p_what)
		{
		case NOTIFICATION_PREDELETE:
		{
		}
		break;
		case NOTIFICATION_ENTER_WORLD:
		{
		}
		break;
		case NOTIFICATION_EXIT_WORLD:
		{
		}
		break;
		case NOTIFICATION_VISIBILITY_CHANGED:
		{
		}
		break;
		case NOTIFICATION_TRANSFORM_CHANGED:
		{
			onGlobalTransformChanged();
		}
		break;
		}
	}

	void GDN_TheWorld_Quadrant::_physics_process(double _delta)
	{
	}

	void GDN_TheWorld_Quadrant::_process(double _delta)
	{
		if (m_quadTree != nullptr)
		{
			godot::Camera3D* camera = m_quadTree->Viewer()->getCamera();
			Transform3D cameraTransform = camera->get_global_transform();
			if (cameraTransform != m_lastCameraTransform)
			{
				m_lastCameraTransform = cameraTransform;
				m_quadTree->getQuadrant()->getCollider()->onCameraTransformChanged();
			}
		}
	}

	void GDN_TheWorld_Quadrant::setColliderTransform(Transform3D t)
	{
		m_quadTree->getQuadrant()->getCollider()->setTransform(t);
	}

	Transform3D GDN_TheWorld_Quadrant::getColliderTransform(void)
	{
		return m_quadTree->getQuadrant()->getCollider()->getTransform();
	}

	//void GDN_TheWorld_Quadrant::setDebugColliderMeshTransform(Transform t)
	//{
	//	if (m_colliderMeshInstance != nullptr)
	//	{
	//		m_colliderMeshInstance->set_global_transform(t);
	//	}
	//}
	
	//Transform GDN_TheWorld_Quadrant::getDebugColliderMeshTransform(void)
	//{
	//	Transform t;

	//	if (m_colliderMeshInstance != nullptr)
	//	{
	//		t = m_colliderMeshInstance->get_global_transform();
	//	}

	//	return t;
	//}

	//void GDN_TheWorld_Quadrant::showDebugColliderMesh(bool show)
	//{
	//	if (!show && m_colliderMeshInstance == nullptr)
	//		return;
	//	
	//	createDebugColliderMeshInstance();

	//	if (show)
	//		m_colliderMeshInstance->set_visible(true);
	//	else
	//		m_colliderMeshInstance->set_visible(false);
	//}
	//
	//void GDN_TheWorld_Quadrant::createDebugColliderMeshInstance(void)
	//{
	//	if (m_colliderMeshInstance == nullptr)
	//	{
	//		m_quadTree->Viewer()->getMainProcessingMutex().lock();
	//		m_colliderMeshInstance = GDN_Collider_MeshInstance::_new();
	//		m_quadTree->Viewer()->getMainProcessingMutex().unlock();
	//		m_colliderMeshInstance->init();

	//		PoolRealArray heights;
	//		int numVertices = m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize() * m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize();
	//		heights.resize((int)numVertices);
	//		{
	//			godot::PoolRealArray::Write w = heights.write();
	//			memcpy((char*)w.ptr(), m_quadTree->getQuadrant()->getFloat32HeightsBuffer().ptr(), m_quadTree->getQuadrant()->getFloat32HeightsBuffer().size());
	//		}

	//		size_t size = heights.size();
	//		if (size == 0)
	//			return;

	//		PoolVector3Array positions;
	//		PoolColorArray colors;
	//		Color initialVertexColor = GDN_TheWorld_Globals::g_color_yellow_apricot;
	//		int numVerticesPerSide = m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize();
	//		positions.resize((int)pow(numVerticesPerSide, 2));
	//		colors.resize((int)pow(numVerticesPerSide, 2));
	//		int pos = 0;
	//		for (real_t z = 0; z < numVerticesPerSide; z++)
	//			for (real_t x = 0; x < numVerticesPerSide; x++)
	//			{
	//				Vector3 v(x, heights[pos], z);
	//				positions.set(pos, v);
	//				colors.set(pos, initialVertexColor);
	//				pos++;
	//			}

	//		PoolIntArray indices;
	//		void GDN_TheWorld_Quadrant_showCollider_makeIndices(PoolIntArray& indices, int numVerticesPerSide);
	//		GDN_TheWorld_Quadrant_showCollider_makeIndices(indices, numVerticesPerSide);

	//		godot::Array arrays;
	//		arrays.resize(ArrayMesh::ARRAY_MAX);
	//		arrays[ArrayMesh::ARRAY_VERTEX] = positions;
	//		arrays[ArrayMesh::ARRAY_COLOR] = colors;
	//		arrays[ArrayMesh::ARRAY_INDEX] = indices;

	//		m_quadTree->Viewer()->getMainProcessingMutex().lock();
	//		Ref<ArrayMesh> mesh = ArrayMesh::_new();
	//		m_quadTree->Viewer()->getMainProcessingMutex().unlock();
	//		int64_t surf_idx = mesh->get_surface_count();	// next surface added will have this surf_idx
	//		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

	//		// Initial Material
	//		m_quadTree->Viewer()->getMainProcessingMutex().lock();
	//		Ref<SpatialMaterial> initialMaterial = SpatialMaterial::_new();
	//		m_quadTree->Viewer()->getMainProcessingMutex().unlock();
	//		initialMaterial->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	//		initialMaterial->set_albedo(initialVertexColor);
	//		mesh->surface_set_material(surf_idx, initialMaterial);

	//		//Ref<Mesh> mesh;
	//		m_colliderMeshInstance->set_mesh(mesh);

	//		add_child(m_colliderMeshInstance);

	//		onGlobalTransformChanged();
	//	}
	//}

	void GDN_TheWorld_Quadrant::onGlobalTransformChanged(void)
	{
		//if (m_colliderMeshInstance != nullptr)
		//{
		//	float gridStepInWU = m_quadTree->getQuadrant()->getPos().getGridStepInWU();
		//	//float axisScaleFactr = sqrtf(powf(gridStepInWU, (float)2.0) / (float)2.0);
		//	float axisScaleFactr = gridStepInWU;
		//	Transform gt = get_global_transform();
		//	Transform tScaled(Basis().scaled(Vector3(axisScaleFactr, (float)1.0, axisScaleFactr)));
		//	Transform t = gt * tScaled;
		//	m_colliderMeshInstance->set_global_transform(t);
		//}
	}

//	void GDN_Collider_MeshInstance::_register_methods()
//	{
//		register_method("_ready", &GDN_Collider_MeshInstance::_ready);
//		register_method("_process", &GDN_Collider_MeshInstance::_process);
//		register_method("_input", &GDN_Collider_MeshInstance::_input);
//	}
//
//	GDN_Collider_MeshInstance::GDN_Collider_MeshInstance()
//	{
//		m_initialized = false;
//	}
//
//	GDN_Collider_MeshInstance::~GDN_Collider_MeshInstance()
//	{
//		deinit();
//	}
//
//	void GDN_Collider_MeshInstance::init(void)
//	{
//		m_initialized = true;
//	}
//
//	void GDN_Collider_MeshInstance::deinit(void)
//	{
//		if (m_initialized)
//		{
//			m_initialized = false;
//		}
//	}
//
//	void GDN_Collider_MeshInstance::_init(void)
//	{
//		//Godot::print("GDN_Template::Init");
//	}
//
//	void GDN_Collider_MeshInstance::_ready(void)
//	{
//		//Godot::print("GDN_Template::_ready");
//		//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
//	}
//
//	void GDN_Collider_MeshInstance::_input(const Ref<InputEvent> event)
//	{
//	}
//
//	void GDN_Collider_MeshInstance::_process(float _delta)
//	{
//		// To activate _process method add this Node to a Godot Scene
//		//Godot::print("GDN_Template::_process");
//	}
}

void GDN_TheWorld_Quadrant_showCollider_makeIndices(godot::PackedInt32Array& indices, int numVerticesPerSide)
{
	int numColumnsOrRows = numVerticesPerSide - 1;
	assert(numColumnsOrRows % 2 == 0);

	int vertex = 0;
	int idx = 0;
	indices.resize((int)pow(numColumnsOrRows, 2) * 6);
	for (int z = 0; z < numColumnsOrRows; z++)
	{
		// sliding all rows with the exclusion of the ones on the border if it nead a seam (on the top or on the bottom) against a less defined mesh
		for (int x = 0; x < numColumnsOrRows; x++)
		{
			//    x ==>
			// z  00---10
			// |   |\  |
			// -   | \ |
			//     |  \|
			//    01---11

			int ixz00 = vertex;											// current vertex
			int ixz10 = vertex + 1;										// next vertex
			int ixz01 = vertex + numVerticesPerSide;						// same vertex on next row
			int ixz11 = ixz01 + 1;										// next vertex

			// first triangle
			indices.set(idx, ixz00);	// indices.append(ixz00);
			idx++;
			indices.set(idx, ixz10);	// indices.append(ixz10);
			idx++;
			indices.set(idx, ixz11);	// indices.append(ixz11);
			idx++;

			// second triangle
			indices.set(idx, ixz00);	// indices.append(ixz00);
			idx++;
			indices.set(idx, ixz11);	// indices.append(ixz11);
			idx++;
			indices.set(idx, ixz01);	// indices.append(ixz01);
			idx++;

			vertex++;
		}
	}
}