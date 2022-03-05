//#include "pch.h"
#include "Chunk.h"
#include "QuadTree.h"
#include "MeshCache.h"

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

#include <VisualServer.hpp>
#include <World.hpp>
#include <SpatialMaterial.hpp>
#include <Material.hpp>
//#include <DirectMesh.hpp>

using namespace godot;

Chunk::Chunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Material* mat)
{
	m_slotPosX = slotPosX;
	m_slotPosZ = slotPosZ;
	m_lod = lod;
	m_viewer = viewer;
	GDN_TheWorld_Globals* globals = m_viewer->Globals();
	m_numVerticesPerChuckSide = globals->numVerticesPerChuckSide();
	m_numChunksPerWorldGridSide = globals->numChunksPerBitmapSide(m_lod);
	m_gridStepInWorldGrid = globals->gridStepInBitmap(m_lod);
	m_chunkSizeInWorldGrid = m_numVerticesPerChuckSide * m_gridStepInWorldGrid;
	m_originXInGrid = m_slotPosX * m_numVerticesPerChuckSide * m_gridStepInWorldGrid;
	m_originZInGrid = m_slotPosZ * m_numVerticesPerChuckSide * m_gridStepInWorldGrid;
	m_firstWorldVertCol = m_slotPosX * m_chunkSizeInWorldGrid;
	m_lastWorldVertCol = (m_slotPosX + 1) * m_chunkSizeInWorldGrid;
	m_firstWorldVertRow = m_slotPosZ * m_chunkSizeInWorldGrid;
	m_lastWorldVertRow = (m_slotPosZ + 1) * m_chunkSizeInWorldGrid;
	m_active = false;
	m_visible = false;
	m_pendingUpdate = false;
	m_justJoined = false;
	m_mat = mat;

	m_viewer->getPartialAABB(m_aabb, m_firstWorldVertCol, m_lastWorldVertCol, m_firstWorldVertRow, m_lastWorldVertRow, m_gridStepInWorldGrid);
	m_aabb.position.x = 0;
	m_aabb.position.z = 0;

	initVisual();
}

Chunk::~Chunk()
{
	deinit();
}

void Chunk::initVisual(void)
{
	VisualServer* vs = VisualServer::get_singleton();
	m_meshInstance = vs->instance_create();
	
	if (m_mat != nullptr)
		vs->instance_geometry_set_material_override(m_meshInstance, m_mat->get_rid());

	//enterWorld();
	Ref<World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		vs->instance_set_scenario(m_meshInstance, world->get_scenario());
	
	m_visible = true; 

	// TODORIC Is this needed?
	vs->instance_set_visible(m_meshInstance, m_visible);

	m_active = true;
}

void Chunk::deinit(void)
{
	if (m_meshInstance != RID())
	{
		VisualServer::get_singleton()->free_rid(m_meshInstance);
		m_meshInstance = RID();
	}
}

void Chunk::setMesh(Ref<Mesh> mesh)
{
	assert(m_meshInstance != RID());

	RID meshRID = (mesh != nullptr ? mesh->get_rid() : RID());

	if (meshRID == m_meshRID)
		return;

	VisualServer::get_singleton()->instance_set_base(m_meshInstance, meshRID);

	m_meshRID = meshRID;
	m_mesh = mesh;
}

void Chunk::enterWorld(void)
{
	assert(m_meshInstance != RID());
	Ref<World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		VisualServer::get_singleton()->instance_set_scenario(m_meshInstance, world->get_scenario());
}

void Chunk::exitWorld(void)
{
	assert(m_meshInstance != RID());
	VisualServer::get_singleton()->instance_set_scenario(m_meshInstance, RID());
}

void Chunk::parentTransformChanged(Transform parentT)
{
	assert(m_meshInstance != RID());
	// TODORIC mah
	Transform localT(Basis(), Vector3((real_t)m_originXInGrid, 0, (real_t)m_originZInGrid));
	Transform worldT = parentT * localT;
	VisualServer::get_singleton()->instance_set_transform(m_meshInstance, worldT);
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
		setPendingUpdate(false);
		return;
	}
	
	int seams = 0;
	QuadTree* tree = m_viewer->getQuadTree();
	ChunkPos posGreaterLod(m_slotPosX / 2, m_slotPosZ / 2, m_lod + 1);
	
	Chunk* chunk = tree->getChunkAt(posGreaterLod, Chunk::DirectionSlot::Left);
	if (chunk && chunk->isActive())
		seams |= SEAM_LEFT;
	chunk = tree->getChunkAt(posGreaterLod, Chunk::DirectionSlot::Right);
	if (chunk && chunk->isActive())
		seams |= SEAM_RIGHT;
	chunk = tree->getChunkAt(posGreaterLod, Chunk::DirectionSlot::Bottom);
	if (chunk && chunk->isActive())
		seams |= SEAM_BOTTOM;
	chunk = tree->getChunkAt(posGreaterLod, Chunk::DirectionSlot::Top);
	if (chunk && chunk->isActive())
		seams |= SEAM_TOP;

	setMesh(m_viewer->getMeshCache()->getMesh(seams, m_lod));

	// TODORIC Is this needed at every update?
	setAABB(m_aabb);

	setVisible(isVisible);
	setPendingUpdate(false);
}

void Chunk::setVisible(bool b)
{
	assert(m_meshInstance != RID());
	if (!isActive())
		b = false;
	VisualServer::get_singleton()->instance_set_visible(m_meshInstance, b);
	m_visible = b; 
}

void Chunk::setAABB(AABB& aabb)
{
	assert(m_meshInstance != RID());
	VisualServer::get_singleton()->instance_set_custom_aabb(m_meshInstance, aabb);
}

ChunkDebug::ChunkDebug(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Material* mat) : Chunk(slotPosX, slotPosZ, lod, viewer, mat)
{
	Variant mesh;

	if (!m_viewer->has_meta(DEBUG_WIRECUBE_MESH))
	{
		Mesh* _mesh = createWirecubeMesh();
		SpatialMaterial* mat = SpatialMaterial::_new();
		mat->set_flag(SpatialMaterial::Flags::FLAG_UNSHADED, true);
		_mesh->surface_set_material(0, mat);
		mesh = _mesh;
		m_viewer->set_meta(DEBUG_WIRECUBE_MESH, mesh);
	}
	else
		mesh = m_viewer->get_meta(DEBUG_WIRECUBE_MESH);
	
	VisualServer* vs = VisualServer::get_singleton();
	m_meshInstance = vs->instance_create();
	vs->instance_set_visible(m_meshInstance, true);
	setMesh(mesh);

	//enterWorld();
	Ref<World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		vs->instance_set_scenario(m_meshInstance, world->get_scenario());
}

ChunkDebug::~ChunkDebug()
{
	if (m_meshInstance != RID())
	{
		VisualServer::get_singleton()->free_rid(m_meshInstance);
		m_meshInstance = RID();
	}
}

void ChunkDebug::enterWorld(void)
{
	Chunk::enterWorld();

	assert(m_meshInstance != RID());
	Ref<World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		VisualServer::get_singleton()->instance_set_scenario(m_meshInstance, world->get_scenario());
}

void ChunkDebug::exitWorld(void)
{
	Chunk::exitWorld();
	
	assert(m_meshInstance != RID());
	VisualServer::get_singleton()->instance_set_scenario(m_meshInstance, RID());
}

void ChunkDebug::parentTransformChanged(Transform t)
{
	Chunk::parentTransformChanged(t);

	m_parentTransform = t;
	assert(m_meshInstance != RID());
	Transform worldTransform = computeAABB();
	VisualServer::get_singleton()->instance_set_transform(m_meshInstance, worldTransform);
}

void ChunkDebug::setVisible(bool b)
{
	Chunk::setVisible(b);
	
	assert(m_meshInstance != RID());
	VisualServer::get_singleton()->instance_set_visible(m_meshInstance, b);
}

void ChunkDebug::setAABB(AABB& aabb)
{
	Chunk::setAABB(aabb);

	m_aabb = aabb;
	assert(m_meshInstance != RID());
	VisualServer::get_singleton()->instance_set_custom_aabb(m_meshInstance, aabb);
	Transform worldTransform = computeAABB();
	VisualServer::get_singleton()->instance_set_transform(m_meshInstance, worldTransform);
}


void ChunkDebug::setMesh(Ref<Mesh> mesh)
{
	assert(m_meshInstance != RID());
	RID meshRID = mesh->get_rid();
	VisualServer::get_singleton()->instance_set_base(m_meshInstance, meshRID);
	m_meshRID = meshRID;

	m_mesh = mesh;
}

Transform ChunkDebug::computeAABB(void)
{
	// TODORIC
	Vector3 pos((real_t)m_originXInGrid, 0, (real_t)m_originZInGrid);
	return m_parentTransform * Transform(Basis().scaled(m_aabb.size), pos + m_aabb.position);
}

Mesh* ChunkDebug::createWirecubeMesh(Color c)
{
	PoolVector3Array positions;
	positions.append(Vector3(0, 0, 0));
	positions.append(Vector3(1, 0, 0));
	positions.append(Vector3(1, 0, 1));
	positions.append(Vector3(0, 0, 1));
	positions.append(Vector3(0, 1, 0));
	positions.append(Vector3(1, 1, 0));
	positions.append(Vector3(1, 1, 1));
	positions.append(Vector3(0, 1, 1));

	PoolColorArray colors;
	for (int i = 0; i < 8; i++)
		colors.append(c);

	PoolIntArray indices;
	indices.append(0); indices.append(1);
	indices.append(1);	indices.append(2);
	indices.append(2); indices.append(3);
	indices.append(3); indices.append(0);
	indices.append(4); indices.append(5);
	indices.append(5); indices.append(6);
	indices.append(6); indices.append(7);
	indices.append(7); indices.append(4);
	indices.append(0); indices.append(4);
	indices.append(1); indices.append(5);
	indices.append(2); indices.append(6);
	indices.append(3); indices.append(7);

	godot::Array arrays;
	arrays.resize(ArrayMesh::ARRAY_MAX);
	arrays[ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[ArrayMesh::ARRAY_COLOR] = colors;
	arrays[ArrayMesh::ARRAY_INDEX] = indices;

	ArrayMesh* mesh = ArrayMesh::_new();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);
	
	return mesh;
}
