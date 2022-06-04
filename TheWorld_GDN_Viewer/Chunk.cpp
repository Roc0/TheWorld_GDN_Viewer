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

	initVisual();

	//globals->debugPrint("Chunk::Chunk - ID: " + String(getPos().getId().c_str()));
}

Chunk::~Chunk()
{
	deinit();
}

void Chunk::initVisual(void)
{
	VisualServer* vs = VisualServer::get_singleton();
	m_meshInstance = vs->instance_create();
	
	if (m_matOverride != nullptr)
		vs->instance_geometry_set_material_override(m_meshInstance, m_matOverride->get_rid());

	//enterWorld();
	Ref<World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		vs->instance_set_scenario(m_meshInstance, world->get_scenario());
	
	m_visible = true; 
	m_active = true;

	vs->instance_set_visible(m_meshInstance, m_visible);
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

bool Chunk::isMeshNull(void)
{
	return (m_meshRID == RID() ? true : false);
}

bool Chunk::isDebugMeshNull(void)
{
	return true;
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

void Chunk::setParentGlobalTransform(Transform parentT)
{
	assert(m_meshInstance != RID());

	m_parentTransform = parentT;

	//Transform localTransform(Basis(), Vector3((real_t)m_originXInWUsLocalToGrid, m_aabb.position.y, (real_t)m_originZInWUsLocalToGrid));	// DEBUGRIC1
	//Transform worldTransformOld = m_parentTransform * localTransform;	// DEBUGRIC1

	Transform worldTransform = getGlobalTransform();

	//assert(worldTransform.origin == worldTransformOld.origin);	// DEBUGRIC1

	VisualServer::get_singleton()->instance_set_transform(m_meshInstance, worldTransform);		// World coordinates
	m_meshGlobaTransformApplied = worldTransform;
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

	// Seams are against grater chunks (greater lod)
	ChunkPos posGreaterLodIfGotJoined(m_slotPosX / 2, m_slotPosZ / 2, m_lod + 1);

	switch (m_posInQuad)
	{
	case PosInQuad::First:
	{
		Chunk* chunk = tree->getChunkAt(posGreaterLodIfGotJoined, Chunk::DirectionSlot::XMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_LEFT;
		chunk = tree->getChunkAt(posGreaterLodIfGotJoined, Chunk::DirectionSlot::ZMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_BOTTOM;
	}
	break;
	case PosInQuad::Second:
	{
		Chunk* chunk = tree->getChunkAt(posGreaterLodIfGotJoined, Chunk::DirectionSlot::XPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_RIGHT;
		chunk = tree->getChunkAt(posGreaterLodIfGotJoined, Chunk::DirectionSlot::ZMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_BOTTOM;
	}
	break;
	case PosInQuad::Third:
	{
		Chunk* chunk = tree->getChunkAt(posGreaterLodIfGotJoined, Chunk::DirectionSlot::XMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_LEFT;
		chunk = tree->getChunkAt(posGreaterLodIfGotJoined, Chunk::DirectionSlot::ZPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_TOP;
	}
	break;
	case PosInQuad::Forth:
	{
		Chunk* chunk = tree->getChunkAt(posGreaterLodIfGotJoined, Chunk::DirectionSlot::XPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_RIGHT;
		chunk = tree->getChunkAt(posGreaterLodIfGotJoined, Chunk::DirectionSlot::ZPlusChunk);
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
	
	// if neeed the AABB can be re-computed
	//m_viewer->getPartialAABB(m_aabb, m_firstWorldVertCol, m_lastWorldVertCol, m_firstWorldVertRow, m_lastWorldVertRow, m_gridStepInGridInWGVs);
	//m_gridRelativeAABB = m_aabb;
	//m_aabb.position.x = 0;	// AABB is relative to the chunk
	//m_aabb.position.z = 0;

	setMesh(nullptr);
	m_viewer->getQuadTree()->addChunkUpdate(this);
}

void Chunk::setVisible(bool b)
{
	assert(m_meshInstance != RID());
	if (!isActive())
		b = false;
	VisualServer::get_singleton()->instance_set_visible(m_meshInstance, b);
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
	assert(m_meshInstance != RID());
	
	if (m_meshRID != RID())
		VisualServer::get_singleton()->instance_set_custom_aabb(m_meshInstance, m_aabb);
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

		if (localToGriddCoordCameraLastPos.x >= m_originXInWUsLocalToGrid && localToGriddCoordCameraLastPos.x <= m_originXInWUsLocalToGrid + m_chunkSizeInWUs
			&& localToGriddCoordCameraLastPos.z >= m_originZInWUsLocalToGrid && localToGriddCoordCameraLastPos.z <= m_originZInWUsLocalToGrid + m_chunkSizeInWUs)
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
	Vector3 pos((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
	return m_parentTransform * Transform(Basis(), pos);
	//return m_parentTransform * Transform(Basis(), pos + m_aabb.position);
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
		+ " - Pos in GRID (global):"
		+ " X = " + to_string(globalOriginXInGridInWUs).c_str()
		+ ", Z = " + to_string(globalOriginZInGridInWUs).c_str()
		+ " - MinH = " + to_string(m_aabb.position.y).c_str()
		+ " - MaxH = " + to_string((m_aabb.position + m_aabb.size).y).c_str()
		+ (m_isCameraVerticalOnChunk ? " - CAMERA" : ""));
}

ChunkDebug::ChunkDebug(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Ref<Material>& mat)
	: Chunk(slotPosX, slotPosZ, lod, viewer, mat)
{
	VisualServer* vs = VisualServer::get_singleton();
	m_debugMeshInstance = vs->instance_create();
	
	//enterWorld();
	Ref<World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		vs->instance_set_scenario(m_debugMeshInstance, world->get_scenario());

	vs->instance_set_visible(m_debugMeshInstance, m_debugVisibility);

	m_debugMeshAABB = m_aabb;

	applyDebugMesh();
}

ChunkDebug::~ChunkDebug()
{
	if (m_debugMeshInstance != RID())
	{
		VisualServer::get_singleton()->free_rid(m_debugMeshInstance);
		m_debugMeshInstance = RID();
	}
}

void ChunkDebug::enterWorld(void)
{
	Chunk::enterWorld();

	assert(m_debugMeshInstance != RID());
	Ref<World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		VisualServer::get_singleton()->instance_set_scenario(m_debugMeshInstance, world->get_scenario());
}

void ChunkDebug::exitWorld(void)
{
	Chunk::exitWorld();
	
	assert(m_debugMeshInstance != RID());
	VisualServer::get_singleton()->instance_set_scenario(m_debugMeshInstance, RID());
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

	assert(m_debugMeshInstance != RID());

	Transform worldTransform = getDebugMeshGlobalTransform();
	VisualServer::get_singleton()->instance_set_transform(m_debugMeshInstance, worldTransform);
	m_debugMeshGlobaTransformApplied = worldTransform;

	//assert(m_debugMeshGlobaTransformApplied.origin == m_meshGlobaTransformApplied.origin);	// DEBUGRIC1
}

Transform ChunkDebug::getDebugMeshGlobalTransformApplied(void)
{
	return m_debugMeshGlobaTransformApplied;
}

void ChunkDebug::setVisible(bool b)
{
	Chunk::setVisible(b);
	
	assert(m_debugMeshInstance != RID());

	if (!isActive())
		b = false;

	if (!m_debugVisibility)
		b = false;

	VisualServer::get_singleton()->instance_set_visible(m_debugMeshInstance, b);
}

void ChunkDebug::setDebugVisibility(bool b)
{
	Chunk::setDebugVisibility(b);

	assert(m_debugMeshInstance != RID());

	if (!isActive())
		b = false;

	VisualServer::get_singleton()->instance_set_visible(m_debugMeshInstance, b);
}

void ChunkDebug::applyAABB(void)
{
	Chunk::applyAABB();

	assert(m_debugMeshInstance != RID());
	if (m_debugVisibility)
		if (m_debugMeshRID != RID())
			VisualServer::get_singleton()->instance_set_custom_aabb(m_debugMeshInstance, m_debugMeshAABB);
}


void ChunkDebug::setDebugMesh(Ref<Mesh> mesh)
{
	assert(m_debugMeshInstance != RID());

	RID meshRID = (mesh != nullptr ? mesh->get_rid() : RID());

	if (m_debugMeshRID == meshRID)
		return;

	VisualServer::get_singleton()->instance_set_base(m_debugMeshInstance, meshRID);
	
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
	
	if (isCameraVerticalOnChunk() != prevCameraVerticalOnChunk)
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
	Transform worldTransform = getDebugMeshGlobalTransform();

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

	VisualServer::get_singleton()->instance_set_transform(m_debugMeshInstance, worldTransform);
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