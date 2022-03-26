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

using namespace godot;

Chunk::Chunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Ref<Material>& mat, enum class GDN_TheWorld_Globals::ChunkDebugMode debugMode)
{
	m_slotPosX = slotPosX;
	m_slotPosZ = slotPosZ;
	m_lod = lod;
	m_isCameraVerticalOnChunk = false;
	m_viewer = viewer;
	m_debugMode = debugMode;
	GDN_TheWorld_Globals* globals = m_viewer->Globals();
	m_numVerticesPerChuckSide = globals->numVerticesPerChuckSide();
	m_numChunksPerWorldGridSide = globals->numChunksPerBitmapSide(m_lod);
	m_gridStepInGridInWGVs = globals->gridStepInBitmap(m_lod);
	m_gridStepInWUs = globals->gridStepInBitmapWUs(m_lod);
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
	m_mat = mat;

	m_viewer->getPartialAABB(m_aabb, m_firstWorldVertCol, m_lastWorldVertCol, m_firstWorldVertRow, m_lastWorldVertRow, m_gridStepInGridInWGVs);
	m_gridRelativeAABB = m_aabb;
	m_aabb.position.x = 0;	// AABB is relative to the chunk
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
	
	if (m_mat == nullptr)
	{
		Ref<SpatialMaterial> mat = SpatialMaterial::_new();
		mat->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		mat->set_albedo(GDN_TheWorld_Globals::g_color_pink_amaranth);
		m_mat = mat;
	}

	vs->instance_geometry_set_material_override(m_meshInstance, m_mat->get_rid());

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

	// TODORIC mah
	if (mesh != nullptr)
		mesh->surface_set_material(0, m_mat);

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
	m_parentTransform = parentT;
	Transform localT(Basis(), Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid));
	Transform worldT = parentT * localT;
	VisualServer::get_singleton()->instance_set_transform(m_meshInstance, worldT);		// World coordinates
	//Transform chunkTranform = parentT * localT;
	//VisualServer::get_singleton()->instance_set_transform(m_meshInstance, chunkTranform);
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
	
	if (m_meshRID != RID())
		VisualServer::get_singleton()->instance_set_custom_aabb(m_meshInstance, aabb);
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

Transform Chunk::getGlobalTransformOfAABB(void)
{
	Vector3 pos((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
	return m_parentTransform * Transform(Basis().scaled(m_aabb.size), pos + m_aabb.position);
}

void Chunk::setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode)
{
	m_debugMode = mode;
}

void Chunk::applyDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode)
{
	if (mode != GDN_TheWorld_Globals::ChunkDebugMode::NotSet)
		setDebugMode(mode);
}

void Chunk::dump(void)
{
	if (!isActive())
		return;
	
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	// TODORIC
	Transform localT(Basis(), Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid));
	Transform worldT = m_parentTransform * localT;
	Vector3 v1 = worldT * Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
	Vector3 v2 = m_parentTransform * Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
	Vector3 v3 = m_viewer->to_global(Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid));
	// TODORIC

	float globalOriginXInGridInWUs = m_viewer->get_global_transform().origin.x + m_originXInWUsLocalToGrid;
	float globalOriginZInGridInWUs = m_viewer->get_global_transform().origin.z + m_originZInWUsLocalToGrid;

	globals->debugPrint(String("Slot in GRID (X, Z)")
		+ " - " + to_string(m_slotPosX).c_str()	+ "," + to_string(m_slotPosZ).c_str()
		+ " - lod " + to_string(m_lod).c_str()
		+ " - chunk size (WUs) " + to_string(m_chunkSizeInWUs).c_str()
		+ " - Pos in GRID (local):"
		+ " X = " + to_string(m_originXInWUsLocalToGrid).c_str()
		+ ", Z = " + to_string(m_originZInWUsLocalToGrid).c_str()
		+ " - Pos in GRID (global):"
		+ " X = " + to_string(globalOriginXInGridInWUs).c_str()
		+ ", Z = " + to_string(globalOriginZInGridInWUs).c_str()
		+ " - MinH = " + to_string(m_aabb.position.y).c_str()
		+ " - MaxH = " + to_string((m_aabb.position + m_aabb.size).y).c_str()
		+ (m_isCameraVerticalOnChunk ? " - Camera IN" : ""));

	/*globals->debugPrint(String("\t") + String("AABB - point 0:")
		+ String(" ") + String("X = ") + String(to_string(m_aabb.get_endpoint(0).x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(m_aabb.get_endpoint(0).y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(m_aabb.get_endpoint(0).z).c_str()));
	globals->debugPrint(String("\t") + String("AABB - point 1:")
		+ String(" ") + String("X = ") + String(to_string(m_aabb.get_endpoint(1).x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(m_aabb.get_endpoint(1).y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(m_aabb.get_endpoint(1).z).c_str()));
	globals->debugPrint(String("\t") + String("AABB - point 2:")
		+ String(" ") + String("X = ") + String(to_string(m_aabb.get_endpoint(2).x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(m_aabb.get_endpoint(2).y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(m_aabb.get_endpoint(2).z).c_str()));
	globals->debugPrint(String("\t") + String("AABB - point 3:")
		+ String(" ") + String("X = ") + String(to_string(m_aabb.get_endpoint(3).x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(m_aabb.get_endpoint(3).y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(m_aabb.get_endpoint(3).z).c_str()));
	globals->debugPrint(String("\t") + String("AABB - point 4:")
		+ String(" ") + String("X = ") + String(to_string(m_aabb.get_endpoint(4).x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(m_aabb.get_endpoint(4).y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(m_aabb.get_endpoint(4).z).c_str()));
	globals->debugPrint(String("\t") + String("AABB - point 5:")
		+ String(" ") + String("X = ") + String(to_string(m_aabb.get_endpoint(5).x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(m_aabb.get_endpoint(5).y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(m_aabb.get_endpoint(5).z).c_str()));
	globals->debugPrint(String("\t") + String("AABB - point 6:")
		+ String(" ") + String("X = ") + String(to_string(m_aabb.get_endpoint(6).x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(m_aabb.get_endpoint(6).y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(m_aabb.get_endpoint(6).z).c_str()));
	globals->debugPrint(String("\t") + String("AABB - point 7:")
		+ String(" ") + String("X = ") + String(to_string(m_aabb.get_endpoint(7).x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(m_aabb.get_endpoint(7).y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(m_aabb.get_endpoint(7).z).c_str()));*/

	/*globals->debugPrint(String("\t") + String("AABB")
		+ String(" - ") + String("X = ") + String(to_string(m_aabb.position.x).c_str())
		+ String(" - ") + String("Y = ") + String(to_string(m_aabb.position.y).c_str())
		+ String(" - ") + String("Z = ") + String(to_string(m_aabb.position.z).c_str())
		+ String(" - ") + String("SX = ") + String(to_string(m_aabb.size.x).c_str())
		+ String(" - ") + String("SY = ") + String(to_string(m_aabb.size.y).c_str())
		+ String(" - ") + String("SZ = ") + String(to_string(m_aabb.size.z).c_str()));*/
}

ChunkDebug::ChunkDebug(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Ref<Material>& mat, enum class GDN_TheWorld_Globals::ChunkDebugMode debugMode)
	: Chunk(slotPosX, slotPosZ, lod, viewer, mat, debugMode)
{
	VisualServer* vs = VisualServer::get_singleton();
	m_debugMeshInstance = vs->instance_create();
	
	//enterWorld();
	Ref<World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		vs->instance_set_scenario(m_debugMeshInstance, world->get_scenario());

	vs->instance_set_visible(m_debugMeshInstance, isVisible());

	applyDebugMode();
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

void ChunkDebug::parentTransformChanged(Transform parentT)
{
	Chunk::parentTransformChanged(parentT);

	assert(m_debugMeshInstance != RID());
	// TODORIC mah
	Transform worldTransform = getGlobalTransformOfAABB();
	VisualServer::get_singleton()->instance_set_transform(m_debugMeshInstance, worldTransform);
}

void ChunkDebug::setVisible(bool b)
{
	Chunk::setVisible(b);
	
	assert(m_debugMeshInstance != RID());

	if (!isActive())
		b = false;

	VisualServer::get_singleton()->instance_set_visible(m_debugMeshInstance, b);
}

void ChunkDebug::setAABB(AABB& aabb)
{
	Chunk::setAABB(aabb);

	assert(m_debugMeshInstance != RID());
	VisualServer::get_singleton()->instance_set_custom_aabb(m_debugMeshInstance, aabb);
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

void ChunkDebug::setCameraPos(Vector3 localToGriddCoordCameraLastPos, Vector3 globalCoordCameraLastPos)
{
	bool prevCameraVerticalOnChunk = isCameraVerticalOnChunk();

	Chunk::setCameraPos(localToGriddCoordCameraLastPos, globalCoordCameraLastPos);

	//return;
	
	if (isCameraVerticalOnChunk() != prevCameraVerticalOnChunk)
		applyDebugMode();
}

void ChunkDebug::setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode)
{
	Chunk::setDebugMode(mode);
}

void ChunkDebug::applyDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode)
{
	Chunk::applyDebugMode(mode);
	
	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
	{
		setDebugMesh(nullptr);
		return;
	}

	Variant mesh;
	Transform worldTransform = getGlobalTransformOfAABB();

	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB)
	{
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

				Mesh* _mesh = createWireCubeMesh(wiredMeshColor);
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
		//Vector3 pos((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
		//worldTransform = m_parentTransform * Transform(Basis().scaled(m_aabb.size), pos + m_aabb.position);
		worldTransform.origin.y = 0;

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

				Mesh* _mesh = createWireSquareMesh(wiredMeshColor);
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
	setAABB(m_aabb);
	AABB aabb = m_aabb;
	aabb.position.y = 0;
	aabb.size.y = 0;
	assert(m_debugMeshInstance != RID());
	VisualServer::get_singleton()->instance_set_custom_aabb(m_debugMeshInstance, aabb);


	VisualServer::get_singleton()->instance_set_transform(m_debugMeshInstance, worldTransform);

	GDN_TheWorld_Globals* globals = m_viewer->Globals();
	globals->debugPrint(String("Debug Mesh - ID: ") + getPos().getId().c_str()
		+ String(" ") + String("X = ") + String(to_string(worldTransform.origin.x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(worldTransform.origin.y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(worldTransform.origin.z).c_str())
		+ (isCameraVerticalOnChunk() ? "CAMERA" : ""));
}

Mesh* ChunkDebug::createWireCubeMesh(Color c)
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

	/*
	// DEBUGRIC
	if (m_aabb.position.y != 0)
	{
		// Vertical faces diagonals
		indices.append(0); indices.append(5);
		indices.append(1); indices.append(4);
		
		indices.append(2); indices.append(7);
		indices.append(3); indices.append(6);

		indices.append(1); indices.append(6);
		indices.append(2); indices.append(5);

		indices.append(3); indices.append(4);
		indices.append(0); indices.append(7);
	}
	*/

	godot::Array arrays;
	arrays.resize(ArrayMesh::ARRAY_MAX);
	arrays[ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[ArrayMesh::ARRAY_COLOR] = colors;
	arrays[ArrayMesh::ARRAY_INDEX] = indices;

	ArrayMesh* mesh = ArrayMesh::_new();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);
	
	return mesh;
}

Mesh* ChunkDebug::createWireSquareMesh(Color c)
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

	ArrayMesh* mesh = ArrayMesh::_new();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);

	return mesh;
}

void ChunkDebug::dump(void)
{
	Chunk::dump();
}