//#include "pch.h"
#include "Chunk.h"
#include "QuadTree.h"
#include "MeshCache.h"

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "Utils.h"

#include <VisualServer.hpp>
#include <World.hpp>
#include <SpatialMaterial.hpp>
#include <Material.hpp>
#include <Ref.hpp>
#include <MeshDataTool.hpp>

using namespace godot;

Chunk::Chunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Ref<Material>& mat)
{
	m_slotPosX = slotPosX;
	m_slotPosZ = slotPosZ;
	m_lod = lod;
	m_posInQuad = PosInQuad::NotSet;
	m_isCameraVerticalOnChunk = false;
	m_viewer = viewer;
	m_debugMode = m_viewer->getRequiredChunkDebugMode();
	m_debugVisibility = m_viewer->getDebugVisibility();
	GDN_TheWorld_Globals* globals = m_viewer->Globals();
	m_numVerticesPerChuckSide = globals->numVerticesPerChuckSide();
	m_numChunksPerWorldGridSide = globals->numChunksPerHeightmapSide(m_lod);
	m_gridStepInGridInWGVs = globals->gridStepInHeightmap(m_lod);
	m_gridStepInWUs = globals->gridStepInHeightmapWUs(m_lod);
	m_chunkSizeInWGVs = m_numVerticesPerChuckSide * m_gridStepInGridInWGVs;
	m_chunkSizeInWUs = m_numVerticesPerChuckSide * m_gridStepInWUs;
	m_originXInGridInWGVs = m_slotPosX * m_numVerticesPerChuckSide * m_gridStepInGridInWGVs;
	m_originZInGridInWGVs = m_slotPosZ * m_numVerticesPerChuckSide * m_gridStepInGridInWGVs;
	m_originXInWUsLocalToGrid = m_slotPosX * m_numVerticesPerChuckSide * m_gridStepInWUs;
	m_originZInWUsLocalToGrid = m_slotPosZ * m_numVerticesPerChuckSide * m_gridStepInWUs;
	m_firstWorldVertCol = m_slotPosX * m_chunkSizeInWGVs;
	m_lastWorldVertCol = (m_slotPosX + 1) * m_chunkSizeInWGVs;
	m_firstWorldVertRow = m_slotPosZ * m_chunkSizeInWGVs;
	m_lastWorldVertRow = (m_slotPosZ + 1) * m_chunkSizeInWGVs;
	m_active = false;
	m_visible = false;
	m_pendingUpdate = false;
	m_justJoined = false;
	m_matOverride = mat;

	//float m_originXInWUsGlobal = m_viewer->get_global_transform().origin.x + m_originXInWUsLocalToGrid;
	//float m_originZInWUsGlobal = m_viewer->get_global_transform().origin.z + m_originZInWUsLocalToGrid;

	m_viewer->getPartialAABB(m_aabb, m_firstWorldVertCol, m_lastWorldVertCol, m_firstWorldVertRow, m_lastWorldVertRow, m_gridStepInGridInWGVs);
	m_gridRelativeAABB = m_aabb;
	m_aabb.position.x = 0;	// AABB is relative to the chunk
	m_aabb.position.z = 0;

	m_useVisualServer = m_viewer->useVisualServer();

	initVisual();

	//globals->debugPrint("Chunk::Chunk - ID: " + String(getPos().getId().c_str()));
}

Chunk::~Chunk()
{
	deinit();
}

void Chunk::initVisual(void)
{
	m_meshInstanceRID = RID();
	m_meshRID = RID();
	m_meshInstance = nullptr;

	m_visible = true;
	m_active = true;

	if (m_useVisualServer)
	{
		VisualServer* vs = VisualServer::get_singleton();
		m_meshInstanceRID = vs->instance_create();

		if (m_matOverride != nullptr)
			vs->instance_geometry_set_material_override(m_meshInstanceRID, m_matOverride->get_rid());

		Ref<World> world = m_viewer->get_world();
		if (world != nullptr && world.is_valid())
			vs->instance_set_scenario(m_meshInstanceRID, world->get_scenario());

		vs->instance_set_visible(m_meshInstanceRID, true);
	}
}

void Chunk::deinit(void)
{
	if (m_useVisualServer)
	{
		if (m_meshInstanceRID != RID())
		{
			VisualServer::get_singleton()->free_rid(m_meshInstanceRID);
			m_meshInstanceRID = RID();
		}
	}
	else
	{
		if (m_meshInstance != nullptr)
		{
			m_meshInstance->remove_from_group(GD_CHUNK_MESHINSTANCE_GROUP);
			m_meshInstance->queue_free();
			m_meshInstance = nullptr;
		}
	}
}

void Chunk::enterWorld(void)
{
	if (m_useVisualServer)
	{
		assert(m_meshInstanceRID != RID());
		Ref<World> world = m_viewer->get_world();
		if (world != nullptr && world.is_valid())
			VisualServer::get_singleton()->instance_set_scenario(m_meshInstanceRID, world->get_scenario());
	}
}

void Chunk::exitWorld(void)
{
	if (m_useVisualServer)
	{
		assert(m_meshInstanceRID != RID());
		VisualServer::get_singleton()->instance_set_scenario(m_meshInstanceRID, RID());
	}
}

void Chunk::setParentGlobalTransform(Transform parentT)
{
	// Pos of the lower point of the mesh of current chunk in global coord
	m_parentTransform = parentT;
	Transform worldTransform = getGlobalTransform();

	if (m_useVisualServer)
	{
		assert(m_meshInstanceRID != RID());
		// Set the mesh pos in global coord
		VisualServer::get_singleton()->instance_set_transform(m_meshInstanceRID, worldTransform);		// World coordinates
	}
	else
	{
		if (m_meshInstance != nullptr)
			m_meshInstance->set_global_transform(worldTransform);
	}

	m_meshGlobaTransformApplied = worldTransform;
}

void Chunk::setVisible(bool b)
{
	if (!isActive())
		b = false;

	if (m_useVisualServer)
	{
		assert(m_meshInstanceRID != RID());
		VisualServer::get_singleton()->instance_set_visible(m_meshInstanceRID, b);
	}
	else
	{
		if (m_meshInstance != nullptr)
			m_meshInstance->set_visible(b);
	}

	m_visible = b;
}

void Chunk::setDebugVisibility(bool b)
{
	if (!isActive())
		b = false;

	m_debugVisibility = b;
}

void Chunk::applyAABB(void)
{
	if (m_useVisualServer)
	{
		assert(m_meshInstanceRID != RID());
		if (m_meshRID != RID())
			VisualServer::get_singleton()->instance_set_custom_aabb(m_meshInstanceRID, m_aabb);
	}
	else
	{
		if (m_meshInstance != nullptr)
			if (m_meshRID != RID())
				m_meshInstance->set_custom_aabb(m_aabb);
	}
}

void Chunk::releaseMesh(void)
{
	if (m_meshInstance != nullptr)
	{
		String name = m_meshInstance->get_name();
		name = "DELETED_" + name;
		m_meshInstance->set_name(name);
		m_meshInstance->remove_from_group(GD_CHUNK_MESHINSTANCE_GROUP);
		m_meshInstance->queue_free();
		m_meshInstance = nullptr;
		m_meshRID = RID();
	}
}

void Chunk::releaseDebugMesh(void)
{
}

void Chunk::setMesh(Ref<Mesh> mesh)
{
	RID meshRID = (mesh != nullptr ? mesh->get_rid() : RID());

	if (meshRID == m_meshRID)
		return;

	if (m_useVisualServer)
	{
		assert(m_meshInstanceRID != RID());
		VisualServer::get_singleton()->instance_set_base(m_meshInstanceRID, meshRID);
	}
	else
	{
		releaseMesh();

		if (mesh != nullptr)
		{
			m_meshInstance = GDN_Chunk_MeshInstance::_new();
			m_meshInstance->init(this);
			m_meshInstance->add_to_group(GD_CHUNK_MESHINSTANCE_GROUP);
			m_meshInstance->set_mesh(mesh);
			string id = Utils::ReplaceString(getPos().getId(), ":", "");
			String name = String("Chunk_") + String(id.c_str());
			m_meshInstance->set_name(name);
			m_viewer->add_child(m_meshInstance);
			if (isVisible())
				m_meshInstance->set_visible(true);
			else
				m_meshInstance->set_visible(false);
			if (m_meshGlobaTransformApplied != Transform())
				m_meshInstance->set_global_transform(m_meshGlobaTransformApplied);
			if (m_matOverride != nullptr)
				m_meshInstance->set_surface_material(0, m_matOverride);
		}
	}

	m_meshRID = meshRID;
	m_mesh = mesh;
}

bool Chunk::isMeshNull(void)
{
	return (m_meshRID == RID() ? true : false);
}

bool Chunk::isDebugMeshNull(void)
{
	return true;
}

// TODORIC Material stuff
//func set_material(material: Material) :
//	assert(_mesh_instance != RID())
//	VisualServer.instance_geometry_set_material_override(\
//		_mesh_instance, material.get_rid() if material != null else RID())

void Chunk::update(bool isVisible)
{
	if (!isActive())
	{
		setVisible(false);
		setPendingUpdate(false);
		return;
	}
	
	int seams = 0;
	QuadTree* tree = m_viewer->getQuadTree();

	// Seams are against grater chunks (greater lod = less resolution)
	ChunkPos posGreaterChunkContainingThisOne(m_slotPosX / 2, m_slotPosZ / 2, m_lod + 1);

	switch (m_posInQuad)
	{
	case PosInQuad::First:
	{
		//  
		//	o =
		//	= =
		//  
		Chunk* chunk = tree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::XMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_LEFT;
		chunk = tree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::ZMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_BOTTOM;
	}
	break;
	case PosInQuad::Second:
	{
		//  
		//	= o
		//	= =
		//  
		Chunk* chunk = tree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::XPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_RIGHT;
		chunk = tree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::ZMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_BOTTOM;
	}
	break;
	case PosInQuad::Third:
	{
		//  
		//	= =
		//	o =
		//  
		Chunk* chunk = tree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::XMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_LEFT;
		chunk = tree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::ZPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_TOP;
	}
	break;
	case PosInQuad::Forth:
	{
		//  
		//	= =
		//	= o
		//  
		Chunk* chunk = tree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::XPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_RIGHT;
		chunk = tree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::ZPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_TOP;
	}
	break;
	}

	setMesh(m_viewer->getMeshCache()->getMesh(seams, m_lod));
	applyDebugMesh();
	applyAABB();

	setVisible(isVisible);

	setPendingUpdate(false);
}

void Chunk::refresh(bool isVisible)
{
	if (!isActive())
		return;
	
	// if needed the AABB can be re-computed
	//m_viewer->getPartialAABB(m_aabb, m_firstWorldVertCol, m_lastWorldVertCol, m_firstWorldVertRow, m_lastWorldVertRow, m_gridStepInGridInWGVs);
	//m_gridRelativeAABB = m_aabb;
	//m_aabb.position.x = 0;	// AABB is relative to the chunk
	//m_aabb.position.z = 0;

	setMesh(nullptr);
	m_viewer->getQuadTree()->addChunkUpdate(this);
}

void Chunk::setActive(bool b)
{
	m_active = b;
	if (!b)
	{
		m_justJoined = false;
		m_isCameraVerticalOnChunk = false;
		m_localToGriddCoordCameraLastPos = Vector3(0, 0, 0);
		m_globalCoordCameraLastPos = Vector3(0, 0, 0);

		if (!m_useVisualServer)
			releaseMesh();
	}
}

void Chunk::setCameraPos(Vector3 localToGriddCoordCameraLastPos, Vector3 globalCoordCameraLastPos)
{
	if (!isActive())
	{
		m_localToGriddCoordCameraLastPos = Vector3(0, 0, 0);
		m_globalCoordCameraLastPos = Vector3(0, 0, 0);
		m_isCameraVerticalOnChunk = false;
	}
	else
	{
		m_localToGriddCoordCameraLastPos = localToGriddCoordCameraLastPos;
		m_globalCoordCameraLastPos = globalCoordCameraLastPos;

		if (localToGriddCoordCameraLastPos.x >= m_originXInWUsLocalToGrid && localToGriddCoordCameraLastPos.x < m_originXInWUsLocalToGrid + m_chunkSizeInWUs
			&& localToGriddCoordCameraLastPos.z >= m_originZInWUsLocalToGrid && localToGriddCoordCameraLastPos.z < m_originZInWUsLocalToGrid + m_chunkSizeInWUs)
			m_isCameraVerticalOnChunk = true;
		else
			m_isCameraVerticalOnChunk = false;
	}

	if (m_isCameraVerticalOnChunk)
		m_viewer->setCameraChunk(this);
}

void Chunk::getCameraPos(Vector3& localToGriddCoordCameraLastPos, Vector3& globalCoordCameraLastPos)
{
	localToGriddCoordCameraLastPos = m_localToGriddCoordCameraLastPos;
	globalCoordCameraLastPos = m_globalCoordCameraLastPos;
}

//Transform Chunk::getGlobalTransformOfAABB(void)
//{
//	Vector3 pos((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
//	return m_parentTransform * Transform(Basis().scaled(m_aabb.size), pos + m_aabb.position);
//}

Transform Chunk::getGlobalTransform(void)
{
	// Return pos of the lower point of the mesh of current chunk in global coord
	Vector3 pos((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
	return m_parentTransform * Transform(Basis(), pos);
	//return m_parentTransform * Transform(Basis(), pos + m_aabb.position);		// DEBUGRIC: assign AABB pos to the mesh altitude
}

Transform Chunk::getMeshGlobalTransformApplied(void)
{
	return m_meshGlobaTransformApplied;
}

Transform Chunk::getDebugMeshGlobalTransform(void)
{
	return Transform();
}

Transform Chunk::getDebugMeshGlobalTransformApplied(void)
{
	return Transform();
}

void Chunk::setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode)
{
	m_debugMode = mode;
}

void Chunk::applyDebugMesh()
{
}

void Chunk::dump(void)
{
	if (!isActive())
		return;
	
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	Transform localT(Basis(), Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid));
	Transform worldT = m_parentTransform * localT;
	Vector3 v1 = worldT * Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
	Vector3 v2 = m_parentTransform * Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
	Vector3 v3 = m_viewer->to_global(Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid));

	float globalOriginXInGridInWUs = m_viewer->get_global_transform().origin.x + m_originXInWUsLocalToGrid;
	float globalOriginZInGridInWUs = m_viewer->get_global_transform().origin.z + m_originZInWUsLocalToGrid;

	globals->debugPrint(String("Chunk ID: ") + getPos().getId().c_str()
		+ " - chunk size (WUs) " + to_string(m_chunkSizeInWUs).c_str()
		+ " - Pos in GRID (local):"
		+ " X = " + to_string(m_originXInWUsLocalToGrid).c_str()
		+ ", Z = " + to_string(m_originZInWUsLocalToGrid).c_str()
		+ ", X1 = " + to_string(m_originXInWUsLocalToGrid + m_chunkSizeInWUs).c_str()
		+ ", Z1 = " + to_string(m_originZInWUsLocalToGrid + m_chunkSizeInWUs).c_str()
		+ " - Pos in GRID (global):"
		+ " X = " + to_string(globalOriginXInGridInWUs).c_str()
		+ ", Z = " + to_string(globalOriginZInGridInWUs).c_str()
		+ ", X1 = " + to_string(globalOriginXInGridInWUs + m_chunkSizeInWUs).c_str()
		+ ", Z1 = " + to_string(globalOriginZInGridInWUs + m_chunkSizeInWUs).c_str()
		+ " - MinH = " + to_string(m_aabb.position.y).c_str()
		+ " - MaxH = " + to_string((m_aabb.position + m_aabb.size).y).c_str()
		+ (m_isCameraVerticalOnChunk ? " - CAMERA" : ""));
	globals->debugPrint(String("t: ") + getMeshGlobalTransformApplied() + String(" - t (debug): ") + getDebugMeshGlobalTransformApplied());
}

ChunkDebug::ChunkDebug(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Ref<Material>& mat)
	: Chunk(slotPosX, slotPosZ, lod, viewer, mat)
{
	m_debugMeshInstanceRID = RID();
	m_debugMeshRID = RID();
	m_debugMeshInstance = nullptr;

	if (m_useVisualServer)
	{
		VisualServer* vs = VisualServer::get_singleton();
		m_debugMeshInstanceRID = vs->instance_create();

		Ref<World> world = m_viewer->get_world();
		if (world != nullptr && world.is_valid())
			vs->instance_set_scenario(m_debugMeshInstanceRID, world->get_scenario());

		vs->instance_set_visible(m_debugMeshInstanceRID, m_debugVisibility);
	}

	m_debugMeshAABB = m_aabb;

	applyDebugMesh();
}

ChunkDebug::~ChunkDebug()
{
	if (m_useVisualServer)
	{
		if (m_debugMeshInstanceRID != RID())
		{
			VisualServer::get_singleton()->free_rid(m_debugMeshInstanceRID);
			m_debugMeshInstanceRID = RID();
		}
	}
	else
	{
		if (m_debugMeshInstance != nullptr)
		{
			m_debugMeshInstance->remove_from_group(GD_DEBUGCHUNK_MESHINSTANCE_GROUP);
			m_debugMeshInstance->queue_free();
			m_debugMeshInstance = nullptr;
		}
	}
}

void ChunkDebug::enterWorld(void)
{
	Chunk::enterWorld();

	if (m_useVisualServer)
	{
		assert(m_debugMeshInstanceRID != RID());
		Ref<World> world = m_viewer->get_world();
		if (world != nullptr && world.is_valid())
			VisualServer::get_singleton()->instance_set_scenario(m_debugMeshInstanceRID, world->get_scenario());
	}
}

void ChunkDebug::exitWorld(void)
{
	Chunk::exitWorld();
	
	if (m_useVisualServer)
	{
		assert(m_debugMeshInstanceRID != RID());
		VisualServer::get_singleton()->instance_set_scenario(m_debugMeshInstanceRID, RID());
	}
}

void ChunkDebug::refresh(bool isVisible)
{
	Chunk::refresh(isVisible);
	setDebugMesh(nullptr);
	m_debugMeshAABB = m_aabb;
}

void ChunkDebug::update(bool isVisible)
{
	Chunk::update(isVisible);
}

//Transform ChunkDebug::getGlobalTransformOfAABB(void)
//{
//	Transform worldTransform = Chunk::getGlobalTransformOfAABB();
//	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
//		worldTransform.origin.y = 0;
//	return worldTransform;
//}

Transform ChunkDebug::getDebugMeshGlobalTransform(void)
{
	Transform worldTransform = Chunk::getGlobalTransform() * Transform(Basis().scaled(m_aabb.size));

	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
		worldTransform.origin.y = 0;

	return worldTransform;
}

void ChunkDebug::setParentGlobalTransform(Transform parentT)
{
	Chunk::setParentGlobalTransform(parentT);

	Transform worldTransform = getDebugMeshGlobalTransform();

	if (m_useVisualServer)
	{
		assert(m_debugMeshInstanceRID != RID());
		VisualServer::get_singleton()->instance_set_transform(m_debugMeshInstanceRID, worldTransform);
	}
	else
	{
		if (m_debugMeshInstance != nullptr)
			m_debugMeshInstance->set_global_transform(worldTransform);
	}
	
	m_debugMeshGlobaTransformApplied = worldTransform;

	//assert(m_debugMeshGlobaTransformApplied.origin == m_meshGlobaTransformApplied.origin);	// DEBUGRIC1
}

Transform ChunkDebug::getDebugMeshGlobalTransformApplied(void)
{
	return m_debugMeshGlobaTransformApplied;
}

void ChunkDebug::setActive(bool b)
{
	Chunk::setActive(b);

	if (!b)
	{
		if (!m_useVisualServer)
			releaseDebugMesh();
	}
}

void ChunkDebug::setVisible(bool b)
{
	Chunk::setVisible(b);
	
	//Chunk::setVisible(false);	// SUPER DEBUGRIC
	//if (m_isCameraVerticalOnChunk)	// SUPER DEBUGRIC
	//	Chunk::setVisible(true);	// SUPER DEBUGRIC
	//else							// SUPER DEBUGRIC
	//	Chunk::setVisible(false);	// SUPER DEBUGRIC

	if (!isActive())
		b = false;

	if (!m_debugVisibility)
		b = false;

	if (m_useVisualServer)
	{
		assert(m_debugMeshInstanceRID != RID());
		VisualServer::get_singleton()->instance_set_visible(m_debugMeshInstanceRID, b);
	}
	else
	{
		if (m_debugMeshInstance != nullptr)
			m_debugMeshInstance->set_visible(b);
	}
}

void ChunkDebug::setDebugVisibility(bool b)
{
	Chunk::setDebugVisibility(b);

	if (!isActive())
		b = false;

	if (m_useVisualServer)
	{
		assert(m_debugMeshInstanceRID != RID());
		VisualServer::get_singleton()->instance_set_visible(m_debugMeshInstanceRID, b);
	}
	else
	{
		if (m_debugMeshInstance != nullptr)
			m_debugMeshInstance->set_visible(b);
	}
}

void ChunkDebug::applyAABB(void)
{
	Chunk::applyAABB();

	if (m_useVisualServer)
	{
		assert(m_debugMeshInstanceRID != RID());
		if (m_debugVisibility)
			if (m_debugMeshRID != RID())
				VisualServer::get_singleton()->instance_set_custom_aabb(m_debugMeshInstanceRID, m_debugMeshAABB);
	}
	else
	{
		if (m_debugMeshInstance != nullptr)
			if (m_debugVisibility)
				if (m_debugMeshRID != RID())
					m_debugMeshInstance->set_custom_aabb(m_debugMeshAABB);
	}
}

void ChunkDebug::releaseDebugMesh(void)
{
	Chunk::releaseDebugMesh();

	if (m_debugMeshInstance != nullptr)
	{
		String name = m_debugMeshInstance->get_name();
		name = "DELETED_" + name;
		m_debugMeshInstance->set_name(name);
		m_debugMeshInstance->remove_from_group(GD_DEBUGCHUNK_MESHINSTANCE_GROUP);
		m_debugMeshInstance->queue_free();
		m_debugMeshInstance = nullptr;
		m_debugMeshRID = RID();
	}
}

void ChunkDebug::setDebugMesh(Ref<Mesh> mesh)
{
	if (m_isCameraVerticalOnChunk)
		RID();
	
	RID meshRID = (mesh != nullptr ? mesh->get_rid() : RID());

	if (m_debugMeshRID == meshRID)
		return;

	//if (mesh == nullptr)
	//	RID();

	if (m_useVisualServer)
	{
		assert(m_debugMeshInstanceRID != RID());
		VisualServer::get_singleton()->instance_set_base(m_debugMeshInstanceRID, meshRID);
	}
	else
	{
		releaseDebugMesh();
		
		if (mesh != nullptr)
		{
			m_debugMeshInstance = MeshInstance::_new();
			m_debugMeshInstance->add_to_group(GD_DEBUGCHUNK_MESHINSTANCE_GROUP);
			m_debugMeshInstance->set_mesh(mesh);
			string id = Utils::ReplaceString(getPos().getId(), ":", "");
			String name = String("ChunkDebug_") + String(id.c_str());
			m_debugMeshInstance->set_name(name);
			m_viewer->add_child(m_debugMeshInstance);
			if (m_debugVisibility && isVisible())
				m_debugMeshInstance->set_visible(true);
			else
				m_debugMeshInstance->set_visible(false);
			if (m_debugMeshGlobaTransformApplied != Transform())
				m_debugMeshInstance->set_global_transform(m_debugMeshGlobaTransformApplied);
		}
	}

	m_debugMeshRID = meshRID;
	m_debugMesh = mesh;
}

bool ChunkDebug::isDebugMeshNull(void)
{
	return (m_debugMeshRID == RID() ? true : false);
}

void ChunkDebug::setCameraPos(Vector3 localToGriddCoordCameraLastPos, Vector3 globalCoordCameraLastPos)
{
	bool prevCameraVerticalOnChunk = isCameraVerticalOnChunk();

	Chunk::setCameraPos(localToGriddCoordCameraLastPos, globalCoordCameraLastPos);

	//return;
	
	if (isCameraVerticalOnChunk() && !prevCameraVerticalOnChunk)
	{
		float globalOriginXInGridInWUs = m_viewer->get_global_transform().origin.x + m_originXInWUsLocalToGrid;
		float globalOriginZInGridInWUs = m_viewer->get_global_transform().origin.z + m_originZInWUsLocalToGrid;

		m_viewer->Globals()->debugPrint("Camera vertical on chunk ("
			+ String(to_string(getPos().getSlotPosX()).c_str()) + ":" + to_string(getPos().getSlotPosZ()).c_str()
			+ ") - Chunk pos (Global) = "
			+ to_string(globalOriginXInGridInWUs).c_str() + ":" + to_string(globalOriginZInGridInWUs).c_str()
			+ " - MinH = " + to_string(m_aabb.position.y).c_str()
			+ " - MaxH = " + to_string((m_aabb.position + m_aabb.size).y).c_str()
			+ " - Chunk Size in WUs = " + to_string(getChunkSizeInWUs()).c_str()
		);
		applyDebugMesh();
		applyAABB();
	}
}

void ChunkDebug::setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode)
{
	Chunk::setDebugMode(mode);
}

void ChunkDebug::applyDebugMesh()
{
	Chunk::applyDebugMesh();
	
	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
	{
		m_debugMeshAABB = m_aabb;
		setDebugMesh(nullptr);
		return;
	}

	Variant mesh;

	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB)
	{
		m_debugMeshAABB = m_aabb;

		if (isCameraVerticalOnChunk())
		{
			// set special Wirecube
			Ref<Mesh> _mesh = createWireCubeMesh(GDN_TheWorld_Globals::g_color_blue);
			SpatialMaterial* mat = SpatialMaterial::_new();
			mat->set_flag(SpatialMaterial::Flags::FLAG_UNSHADED, true);
			mat->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
			mat->set_albedo(GDN_TheWorld_Globals::g_color_blue);
			_mesh->surface_set_material(0, mat);
			mesh = _mesh;
		}
		else
		{
			// reset normal Wirecube
			string metaNameMesh = DEBUG_WIRECUBE_MESH + to_string(m_lod);
			if (!m_viewer->has_meta(metaNameMesh.c_str()))
			{
				Color wiredMeshColor;
				if (m_lod == 0)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_yellow_apricot;
				else if (m_lod == 1)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_aquamarine_green;
				else if (m_lod == 2)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_green;
				else if (m_lod == 3)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_cyan;
				else if (m_lod == 4)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_pink;
				else
					wiredMeshColor = GDN_TheWorld_Globals::g_color_red;

				Ref<ArrayMesh> _mesh = createWireCubeMesh(wiredMeshColor);
				SpatialMaterial* mat = SpatialMaterial::_new();
				mat->set_flag(SpatialMaterial::Flags::FLAG_UNSHADED, true);
				mat->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
				mat->set_albedo(wiredMeshColor);
				_mesh->surface_set_material(0, mat);
				mesh = _mesh;
				m_viewer->set_meta(metaNameMesh.c_str(), mesh);
			}
			else
				mesh = m_viewer->get_meta(metaNameMesh.c_str());
		}
	}
	else if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
	{
		//worldTransform.origin.y = 0;	inutile
		m_debugMeshAABB = m_aabb;
		m_debugMeshAABB.position.y = 0;
		m_debugMeshAABB.size.y = 0;

		if (isCameraVerticalOnChunk())
		{
			// set special Wirecube
			Color wiredMeshColor = GDN_TheWorld_Globals::g_color_blue;
			Ref<Mesh> _mesh = createWireSquareMesh(wiredMeshColor);
			SpatialMaterial* mat = SpatialMaterial::_new();
			mat->set_flag(SpatialMaterial::Flags::FLAG_UNSHADED, true);
			mat->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
			mat->set_albedo(wiredMeshColor);
			_mesh->surface_set_material(0, mat);
			mesh = _mesh;
		}
		else
		{
			// reset normal Wirecube
			string metaNameMesh = DEBUG_WIRESQUARE_MESH + to_string(m_lod);
			if (!m_viewer->has_meta(metaNameMesh.c_str()))
			{
				Color wiredMeshColor;
				if (m_lod == 0)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_yellow_apricot;
				else if (m_lod == 1)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_aquamarine_green;
				else if (m_lod == 2)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_green;
				else if (m_lod == 3)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_cyan;
				else if (m_lod == 4)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_pink;
				else
					wiredMeshColor = GDN_TheWorld_Globals::g_color_red;

				Ref<ArrayMesh> _mesh = createWireSquareMesh(wiredMeshColor);
				SpatialMaterial* mat = SpatialMaterial::_new();
				mat->set_flag(SpatialMaterial::Flags::FLAG_UNSHADED, true);
				mat->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
				mat->set_albedo(wiredMeshColor);
				_mesh->surface_set_material(0, mat);
				mesh = _mesh;
				m_viewer->set_meta(metaNameMesh.c_str(), mesh);
			}
			else
				mesh = m_viewer->get_meta(metaNameMesh.c_str());
		}
	}

	setDebugMesh(nullptr);
	setDebugMesh(mesh);

	Transform worldTransform = getDebugMeshGlobalTransform();
	if (m_useVisualServer)
		VisualServer::get_singleton()->instance_set_transform(m_debugMeshInstanceRID, worldTransform);
	else
		m_debugMeshInstance->set_global_transform(worldTransform);
	m_debugMeshGlobaTransformApplied = worldTransform;


	/*GDN_TheWorld_Globals* globals = m_viewer->Globals();
	globals->debugPrint(String("Debug Mesh - ID: ") + getPos().getId().c_str()
		+ String(" POS ==> ") + String("X = ") + String(to_string(worldTransform.origin.x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(worldTransform.origin.y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(worldTransform.origin.z).c_str())
		+ (isCameraVerticalOnChunk() ? "CAMERA" : ""));*/
}

Ref<ArrayMesh> ChunkDebug::createWireCubeMesh(Color c)
{
	PoolVector3Array positions;
	PoolColorArray colors;
	PoolIntArray indices;

	positions.append(Vector3(0, 0, 0));		colors.append(c);
	positions.append(Vector3(1, 0, 0));		colors.append(c);
	positions.append(Vector3(1, 0, 1));		colors.append(c);
	positions.append(Vector3(0, 0, 1));		colors.append(c);
	positions.append(Vector3(0, 1, 0));		colors.append(c);
	positions.append(Vector3(1, 1, 0));		colors.append(c);
	positions.append(Vector3(1, 1, 1));		colors.append(c);
	positions.append(Vector3(0, 1, 1));		colors.append(c);

	// lower face quad
	indices.append(0); indices.append(1);
	indices.append(1); indices.append(2);
	indices.append(2); indices.append(3);
	indices.append(3); indices.append(0);
	// upper face quad
	indices.append(4); indices.append(5);
	indices.append(5); indices.append(6);
	indices.append(6); indices.append(7);
	indices.append(7); indices.append(4);
	// vertical lines
	indices.append(0); indices.append(4);
	indices.append(1); indices.append(5);
	indices.append(2); indices.append(6);
	indices.append(3); indices.append(7);
	// lower face diagonals
	indices.append(0); indices.append(2);
	indices.append(1); indices.append(3);

	godot::Array arrays;
	arrays.resize(ArrayMesh::ARRAY_MAX);
	arrays[ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[ArrayMesh::ARRAY_COLOR] = colors;
	arrays[ArrayMesh::ARRAY_INDEX] = indices;

	Ref<ArrayMesh> mesh = ArrayMesh::_new();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);
	
	return mesh;
}

Ref<ArrayMesh> ChunkDebug::createWireSquareMesh(Color c)
{
	PoolVector3Array positions;
	PoolColorArray colors;
	PoolIntArray indices;

	positions.append(Vector3(0, 0, 0));		colors.append(c);
	positions.append(Vector3(1, 0, 0));		colors.append(c);
	positions.append(Vector3(1, 0, 1));		colors.append(c);
	positions.append(Vector3(0, 0, 1));		colors.append(c);

	// square
	indices.append(0); indices.append(1);
	indices.append(1); indices.append(2);
	indices.append(2); indices.append(3);
	indices.append(3); indices.append(0);
	// diagonals
	indices.append(0); indices.append(2);
	indices.append(1); indices.append(3);

	godot::Array arrays;
	arrays.resize(ArrayMesh::ARRAY_MAX);
	arrays[ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[ArrayMesh::ARRAY_COLOR] = colors;
	arrays[ArrayMesh::ARRAY_INDEX] = indices;

	Ref<ArrayMesh> mesh = ArrayMesh::_new();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);

	return mesh;
}

void ChunkDebug::dump(void)
{
	Chunk::dump();

	if (!isActive())
		return;

	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	globals->debugPrint(String("Chunk ID: ") + getPos().getId().c_str()
		+ " DEBUG MESH - MinH = " + to_string(m_debugMeshAABB.position.y).c_str()
		+ " - MaxH = " + to_string((m_debugMeshAABB.position + m_debugMeshAABB.size).y).c_str()
		+ (m_isCameraVerticalOnChunk ? " - CAMERA" : ""));
}

void GDN_Chunk_MeshInstance::_register_methods()
{
	register_method("_ready", &GDN_Chunk_MeshInstance::_ready);
	register_method("_process", &GDN_Chunk_MeshInstance::_process);
	register_method("_input", &GDN_Chunk_MeshInstance::_input);

	register_method("get_lod", &GDN_Chunk_MeshInstance::getLod);
	register_method("get_slot_pos_x", &GDN_Chunk_MeshInstance::getSlotPosX);
	register_method("get_slot_pos_z", &GDN_Chunk_MeshInstance::getSlotPosZ);
	register_method("get_id", &GDN_Chunk_MeshInstance::getId);
}

GDN_Chunk_MeshInstance::GDN_Chunk_MeshInstance()
{
	m_initialized = false;
	m_chunk = nullptr;
}

GDN_Chunk_MeshInstance::~GDN_Chunk_MeshInstance()
{
	deinit();
}

void GDN_Chunk_MeshInstance::init(Chunk* chunk)
{
	m_chunk = chunk;
}

void GDN_Chunk_MeshInstance::deinit(void)
{
	if (m_initialized)
	{
		m_initialized = false;
	}
}

void GDN_Chunk_MeshInstance::_init(void)
{
	//Godot::print("GDN_Template::Init");
}

void GDN_Chunk_MeshInstance::_ready(void)
{
	//Godot::print("GDN_Template::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_Chunk_MeshInstance::_input(const Ref<InputEvent> event)
{
}

void GDN_Chunk_MeshInstance::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_Template::_process");
}

int GDN_Chunk_MeshInstance::getLod(void)
{
	return m_chunk->getLod();
}

int GDN_Chunk_MeshInstance::getSlotPosX(void)
{
	return m_chunk->getSlotPosX();
}

int GDN_Chunk_MeshInstance::getSlotPosZ(void)
{
	return m_chunk->getSlotPosZ();
}

String GDN_Chunk_MeshInstance::getId(void)
{
	return m_chunk->getId().c_str();
}
