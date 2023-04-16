//#include "pch.h"
#include "Chunk.h"
#include "QuadTree.h"
#include "MeshCache.h"

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "Viewer_Utils.h"

#include <VisualServer.hpp>
#include <World.hpp>
#include <SpatialMaterial.hpp>
#include <Material.hpp>
#include <Ref.hpp>
#include <MeshDataTool.hpp>

using namespace godot;

Chunk::Chunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat)
{
	m_distanceFromCamera = 0;
	m_slotPosX = slotPosX;
	m_slotPosZ = slotPosZ;
	m_lod = lod;
	m_posInQuad = PosInQuad::NotSet;
	m_isCameraVerticalOnChunk = false;
	m_viewer = viewer;
	m_quadTree = quadTree;
	m_quad = nullptr;
	m_debugMode = m_viewer->getRequiredChunkDebugMode();
	m_debugContentVisible = m_viewer->getDebugContentVisibility();
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
	m_visible = m_quadTree->isVisible();
	m_pendingUpdate = false;
	m_justJoined = false;

	m_originXInWUsGlobal = m_quadTree->getQuadrant()->getPos().getLowerXGridVertex() + m_originXInWUsLocalToGrid;
	m_originZInWUsGlobal = m_quadTree->getQuadrant()->getPos().getLowerZGridVertex() + m_originZInWUsLocalToGrid;

	m_initialGlobalTransform = Transform(Basis(), Vector3((real_t)m_originXInWUsGlobal, 0, m_originZInWUsGlobal));

	//checkAndCalcAABB();

	initVisual(mat);

	//globals->debugPrint("Chunk::Chunk - ID: " + String(getPos().getPos().c_str()));
}

Chunk::~Chunk()
{
	deinit();
}

void Chunk::checkAndCalcAABB()
{
	AABB aabb;
	if (m_aabb == aabb && m_quadTree->statusInitialized())
	{
		getGlobalCoordAABB(m_aabb, m_firstWorldVertCol, m_lastWorldVertCol, m_firstWorldVertRow, m_lastWorldVertRow, m_gridStepInGridInWGVs);
		m_globalCoordAABB = m_aabb;
		m_aabb.position.x = 0;	// AABB is relative to the chunk
		m_aabb.position.z = 0;
	}
}

void Chunk::initVisual(godot::Ref<godot::Material>& mat)
{
	m_meshInstanceRID = RID();
	m_meshRID = RID();
	//m_meshInstance = nullptr;

	m_active = true;

	godot::VisualServer* vs = godot::VisualServer::get_singleton();
	m_meshInstanceRID = vs->instance_create();
	
	if (mat != nullptr)
		vs->instance_geometry_set_material_override(m_meshInstanceRID, mat->get_rid());

	godot::Ref<godot::World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		vs->instance_set_scenario(m_meshInstanceRID, world->get_scenario());

	vs->instance_set_visible(m_meshInstanceRID, isVisible());

	m_globalTransformApplied = getGlobalTransform();

	godot::VisualServer::get_singleton()->instance_set_transform(m_meshInstanceRID, m_globalTransformApplied);		// World coordinates
}

void Chunk::deinit(void)
{
	if (m_meshInstanceRID != godot::RID())
	{
		godot::VisualServer::get_singleton()->free_rid(m_meshInstanceRID);
		m_meshInstanceRID = godot::RID();
	}

	if (m_viewer->m_cameraChunk == this)
		m_viewer->m_cameraChunk = nullptr;
	if (m_viewer->m_mouseHitChunk == this)
		m_viewer->m_mouseHitChunk = nullptr;
}

void Chunk::enterWorld(void)
{
	assert(m_meshInstanceRID != godot::RID());
	godot::Ref<godot::World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		godot::VisualServer::get_singleton()->instance_set_scenario(m_meshInstanceRID, world->get_scenario());
}

void Chunk::exitWorld(void)
{
	assert(m_meshInstanceRID != godot::RID());
	godot::VisualServer::get_singleton()->instance_set_scenario(m_meshInstanceRID, godot::RID());
}

void Chunk::onGlobalTransformChanged(void)
{
	assert(m_meshInstanceRID != godot::RID());
	m_globalTransformApplied = getGlobalTransform();
	// Set the mesh pos in global coord
	godot::VisualServer::get_singleton()->instance_set_transform(m_meshInstanceRID, m_globalTransformApplied);		// World coordinates
}

void Chunk::setVisible(bool b)
{
	if (!isActive())
		b = false;

	if (!m_quadTree->isVisible())
		b = false;

	assert(m_meshInstanceRID != godot::RID());
	godot::VisualServer::get_singleton()->instance_set_visible(m_meshInstanceRID, b);

	m_visible = b;
}

void Chunk::setDebugContentVisible(bool b)
{
	if (!isActive())
		b = false;

	if (!m_quadTree->isVisible())
		b = false;

	m_debugContentVisible = b;
}

void Chunk::applyAABB(void)
{
	checkAndCalcAABB();
	
	assert(m_meshInstanceRID != godot::RID());
	if (m_meshRID != RID())
		godot::VisualServer::get_singleton()->instance_set_custom_aabb(m_meshInstanceRID, m_aabb);
}

void Chunk::setMesh(Ref<Mesh> mesh)
{
	godot::RID meshRID = (mesh != nullptr ? mesh->get_rid() : godot::RID());

	if (meshRID == m_meshRID)
		return;

	assert(m_meshInstanceRID != godot::RID());
	godot::VisualServer::get_singleton()->instance_set_base(m_meshInstanceRID, meshRID);

	m_meshRID = meshRID;
	m_mesh = mesh;
}

bool Chunk::isMeshNull(void)
{
	return (m_meshRID == godot::RID() ? true : false);
}

bool Chunk::isDebugMeshNull(void)
{
	return true;
}

bool Chunk::isQuadTreeVisible(void)
{
	return m_quadTree->isVisible() && m_quadTree->isValid();
}

// TODORIC Material stuff
//func set_material(material: Material) :
//	assert(_mesh_instance != RID())
//	VisualServer.instance_geometry_set_material_override(\
//		_mesh_instance, material.get_rid() if material != null else RID())

void Chunk::setMaterial(godot::Ref<godot::Material>& mat)
{
	assert(m_meshInstanceRID != godot::RID());
	assert(mat != nullptr);

	godot::VisualServer::get_singleton()->instance_geometry_set_material_override(m_meshInstanceRID, mat->get_rid());
}

void Chunk::update(bool isVisible)
{
	if (!isActive())
	{
		setVisible(false);
		setPendingUpdate(false);
		return;
	}
	
	int seams = 0;

	// Seams are against grater chunks (greater lod = less resolution)
	ChunkPos posGreaterChunkContainingThisOne(m_slotPosX / 2, m_slotPosZ / 2, m_lod + 1);
	int numGreaterChunksPerSide = 0;
	if (posGreaterChunkContainingThisOne.getLod() <= m_viewer->Globals()->lodMaxDepth())
		numGreaterChunksPerSide = m_viewer->Globals()->numChunksPerHeightmapSide(posGreaterChunkContainingThisOne.getLod());

	switch (m_posInQuad)
	{
	case PosInQuad::First:
	{
		//  
		//	o =
		//	= =
		//  

		Chunk* chunk = m_quadTree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::XMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_LEFT;

		if (numGreaterChunksPerSide > 0)
		{
			QuadrantPos XMinusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XMinus);
			QuadTree* XMinusQuadTree = m_viewer->getQuadTree(XMinusQuadrantPos);
			if (XMinusQuadTree != nullptr && XMinusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(numGreaterChunksPerSide - 1, posGreaterChunkContainingThisOne.getSlotPosZ(), posGreaterChunkContainingThisOne.getLod());
				chunk = XMinusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_LEFT;
			}
		}

		chunk = m_quadTree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::ZMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_BOTTOM;

		if (numGreaterChunksPerSide > 0)
		{
			QuadrantPos ZMinusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZMinus);
			QuadTree* ZMinusQuadTree = m_viewer->getQuadTree(ZMinusQuadrantPos);
			if (ZMinusQuadTree != nullptr && ZMinusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(posGreaterChunkContainingThisOne.getSlotPosX(), numGreaterChunksPerSide - 1, posGreaterChunkContainingThisOne.getLod());
				chunk = ZMinusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_BOTTOM;
			}
		}
	}
	break;
	case PosInQuad::Second:
	{
		//  
		//	= o
		//	= =
		//  

		Chunk* chunk = m_quadTree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::XPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_RIGHT;

		if (numGreaterChunksPerSide > 0)
		{
			QuadrantPos XPlusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XPlus);
			QuadTree* XPlusQuadTree = m_viewer->getQuadTree(XPlusQuadrantPos);
			if (XPlusQuadTree != nullptr && XPlusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(0, posGreaterChunkContainingThisOne.getSlotPosZ(), posGreaterChunkContainingThisOne.getLod());
				chunk = XPlusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_RIGHT;
			}
		}

		chunk = m_quadTree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::ZMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_BOTTOM;

		if (numGreaterChunksPerSide > 0)
		{
			QuadrantPos ZMinusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZMinus);
			QuadTree* ZMinusQuadTree = m_viewer->getQuadTree(ZMinusQuadrantPos);
			if (ZMinusQuadTree != nullptr && ZMinusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(posGreaterChunkContainingThisOne.getSlotPosX(), numGreaterChunksPerSide - 1, posGreaterChunkContainingThisOne.getLod());
				chunk = ZMinusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_BOTTOM;
			}
		}
	}
	break;
	case PosInQuad::Third:
	{
		//  
		//	= =
		//	o =
		//  
		
		Chunk* chunk = m_quadTree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::XMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_LEFT;

		if (numGreaterChunksPerSide > 0)
		{
			QuadrantPos XMinusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XMinus);
			QuadTree* XMinusQuadTree = m_viewer->getQuadTree(XMinusQuadrantPos);
			if (XMinusQuadTree != nullptr && XMinusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(numGreaterChunksPerSide - 1, posGreaterChunkContainingThisOne.getSlotPosZ(), posGreaterChunkContainingThisOne.getLod());
				chunk = XMinusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_LEFT;
			}
		}

		chunk = m_quadTree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::ZPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_TOP;

		if (numGreaterChunksPerSide > 0)
		{
			QuadrantPos ZPlusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZPlus);
			QuadTree* ZPlusQuadTree = m_viewer->getQuadTree(ZPlusQuadrantPos);
			if (ZPlusQuadTree != nullptr && ZPlusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(posGreaterChunkContainingThisOne.getSlotPosX(), 0, posGreaterChunkContainingThisOne.getLod());
				chunk = ZPlusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_TOP;
			}
		}
	}
	break;
	case PosInQuad::Forth:
	{
		//  
		//	= =
		//	= o
		//  
		
		Chunk* chunk = m_quadTree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::XPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_RIGHT;

		if (numGreaterChunksPerSide > 0)
		{
			QuadrantPos XPlusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XPlus);
			QuadTree* XPlusQuadTree = m_viewer->getQuadTree(XPlusQuadrantPos);
			if (XPlusQuadTree != nullptr && XPlusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(0, posGreaterChunkContainingThisOne.getSlotPosZ(), posGreaterChunkContainingThisOne.getLod());
				chunk = XPlusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_RIGHT;
			}
		}

		chunk = m_quadTree->getChunkAt(posGreaterChunkContainingThisOne, Chunk::DirectionSlot::ZPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_TOP;

		if (numGreaterChunksPerSide > 0)
		{
			QuadrantPos ZPlusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZPlus);
			QuadTree* ZPlusQuadTree = m_viewer->getQuadTree(ZPlusQuadrantPos);
			if (ZPlusQuadTree != nullptr && ZPlusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(posGreaterChunkContainingThisOne.getSlotPosX(), 0, posGreaterChunkContainingThisOne.getLod());
				chunk = ZPlusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_TOP;
			}
		}
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
	//m_globalCoordAABB = m_aabb;
	//m_aabb.position.x = 0;	// AABB is relative to the chunk
	//m_aabb.position.z = 0;

	setMesh(nullptr);
	m_quadTree->addChunkUpdate(this);
}

void Chunk::heightsChanged(void)
{
	m_aabb = AABB();
	Quad* quad = getQuad();
	if (quad != nullptr)
		quad->resetChachedChunkAABB();
}

void Chunk::setActive(bool b)
{
	m_active = b;
	if (!b)
	{
		m_justJoined = false;
		if (m_viewer->m_cameraChunk == this)
			m_viewer->m_cameraChunk = nullptr;
		m_isCameraVerticalOnChunk = false;
		m_globalCoordCameraLastPos = Vector3(0, 0, 0);
	}
}

void Chunk::setCameraPos(Vector3 globalCoordCameraLastPos)
{
	if (!isActive())
	{
		m_globalCoordCameraLastPos = Vector3(0, 0, 0);
		if (m_viewer->m_cameraChunk == this)
			m_viewer->m_cameraChunk = nullptr;
		m_isCameraVerticalOnChunk = false;
	}
	else
	{
		m_globalCoordCameraLastPos = globalCoordCameraLastPos;

		if (globalCoordCameraLastPos.x >= m_originXInWUsGlobal && globalCoordCameraLastPos.x < m_originXInWUsGlobal + m_chunkSizeInWUs
			&& globalCoordCameraLastPos.z >= m_originZInWUsGlobal && globalCoordCameraLastPos.z < m_originZInWUsGlobal + m_chunkSizeInWUs)
			m_isCameraVerticalOnChunk = true;
		else
		{
			if (m_viewer->m_cameraChunk == this)
				m_viewer->m_cameraChunk = nullptr;
			m_isCameraVerticalOnChunk = false;
		}
	}

	if (m_isCameraVerticalOnChunk)
		m_viewer->setCameraChunk(this, m_quadTree);
}

void Chunk::getCameraPos(Vector3& globalCoordCameraLastPos)
{
	globalCoordCameraLastPos = m_globalCoordCameraLastPos;
}

Transform Chunk::getGlobalTransform(void)
{
	Transform igt = m_viewer->getInternalGlobalTransform();
	Transform gt = igt * m_initialGlobalTransform;

	return gt;
}

Transform Chunk::getGlobalTransformApplied(void)
{
	return m_globalTransformApplied;
}

Transform Chunk::getDebugGlobalTransform(void)
{
	return Transform();
}

Transform Chunk::getDebugGlobalTransformApplied(void)
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

void Chunk::getGlobalCoordAABB(AABB& aabb, int firstWorldVertCol, int lastWorldVertCol, int firstWorldVertRow, int lastWorldVertRow, int step)
{
	int numWorldVerticesPerSize = m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize();
	int idxFirstColFirstRowWorldVert = numWorldVerticesPerSize * firstWorldVertRow + firstWorldVertCol;
	int idxLastColFirstRowWorldVert = numWorldVerticesPerSize * firstWorldVertRow + lastWorldVertCol;
	int idxFirstColLastRowWorldVert = numWorldVerticesPerSize * lastWorldVertRow + firstWorldVertCol;
	int idxLastColLastRowWorldVert = numWorldVerticesPerSize * lastWorldVertRow + lastWorldVertCol;

	// altitudes
	float minHeigth = 0, maxHeigth = 0;
	bool firstTime = true;
	for (int idxRow = 0; idxRow < lastWorldVertRow - firstWorldVertRow + 1; idxRow += step)
	{
		for (int idxVert = idxFirstColFirstRowWorldVert + idxRow * numWorldVerticesPerSize; idxVert < idxLastColFirstRowWorldVert + idxRow * numWorldVerticesPerSize + 1; idxVert += step)
		{
			if (firstTime)
			{
				minHeigth = maxHeigth = m_quadTree->getQuadrant()->getAltitudeFromHeigthmap(idxVert);
				firstTime = false;
			}
			else
			{
				minHeigth = TheWorld_Viewer_Utils::Utils::min2(minHeigth, m_quadTree->getQuadrant()->getAltitudeFromHeigthmap(idxVert));
				maxHeigth = TheWorld_Viewer_Utils::Utils::max2(maxHeigth, m_quadTree->getQuadrant()->getAltitudeFromHeigthmap(idxVert));
			}
		}
	}

	Vector3 startPosition(m_quadTree->getQuadrant()->getPosXFromHeigthmap(idxFirstColFirstRowWorldVert), minHeigth, m_quadTree->getQuadrant()->getPosZFromHeigthmap(idxFirstColFirstRowWorldVert));
	//Vector3 endPosition(m_worldVertices[idxLastColLastRowWorldVert].posX() - m_worldVertices[idxFirstColFirstRowWorldVert].posX(), maxHeigth, m_worldVertices[idxLastColLastRowWorldVert].posZ() - m_worldVertices[idxFirstColFirstRowWorldVert].posZ());
	Vector3 endPosition(m_quadTree->getQuadrant()->getPosXFromHeigthmap(idxLastColLastRowWorldVert), maxHeigth, m_quadTree->getQuadrant()->getPosZFromHeigthmap(idxLastColLastRowWorldVert));
	Vector3 size = endPosition - startPosition;

	aabb.set_position(startPosition);
	aabb.set_size(size);
}

void Chunk::setDistanceFromCamera(float distanceFromCamera)
{
	m_distanceFromCamera = distanceFromCamera;
}

float Chunk::getDistanceFromCamera(void)
{
	return m_distanceFromCamera;
}

void Chunk::checkMouseHit(void)
{
	godot::Vector3 mouseHit = m_viewer->getMouseHit();
	if (checkHit(mouseHit))
	{
		//m_viewer->setMouseHitQuadTree(m_quadTree);
		m_viewer->setMouseHitChunk(this);
	}

	//godot::Point2 origOfChunk(getLowerXInWUsGlobal(), getLowerZInWUsGlobal());
	//godot::Point2 size(getChunkSizeInWUs(), getChunkSizeInWUs());
	////godot::Rect2 rect(origOfChunk, size);
	//if (mouseHit.x >= origOfChunk.x && mouseHit.x <= origOfChunk.x + size.x && mouseHit.z >= origOfChunk.y && mouseHit.z <= origOfChunk.y + size.y)
	////if (rect.has_point(godot::Point2(mouseHit.x, mouseHit.z)))
	//{
	//	m_viewer->setMouseHitQuadTree(m_quadTree);
	//	m_viewer->setMouseHitChunk(this);
	//}
}

bool Chunk::checkHit(godot::Vector3 hit)
{
	godot::Point2 origOfChunk(getLowerXInWUsGlobal(), getLowerZInWUsGlobal());
	godot::Point2 size(getChunkSizeInWUs(), getChunkSizeInWUs());
	//godot::Rect2 rect(origOfChunk, size);
	//if (rect.has_point(godot::Point2(mouseHit.x, mouseHit.z)))
	if (hit.x >= origOfChunk.x && hit.x <= origOfChunk.x + size.x && hit.z >= origOfChunk.y && hit.z <= origOfChunk.y + size.y)
		return true;
	else
		return false;
}

void Chunk::dump(void)
{
	if (!isActive())
		return;
	
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	//Transform localT(Basis(), Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid));
	//Transform worldT = m_parentTransform * localT;
	//Transform worldT = m_initialGlobalTransform;
	//Vector3 v1 = worldT * Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
	//Vector3 v2 = m_parentTransform * Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid);
	//Vector3 v3 = m_viewer->to_global(Vector3((real_t)m_originXInWUsLocalToGrid, 0, (real_t)m_originZInWUsLocalToGrid));

	float globalOriginXInGridInWUs = m_viewer->get_global_transform().origin.x + m_originXInWUsLocalToGrid;
	float globalOriginZInGridInWUs = m_viewer->get_global_transform().origin.z + m_originZInWUsLocalToGrid;

	globals->debugPrint(String("Chunk ID: ") + getPos().getIdStr().c_str()
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
	globals->debugPrint(String("t: ") + getGlobalTransformApplied() + String(" - t (debug): ") + getDebugGlobalTransformApplied());
}

ChunkDebug::ChunkDebug(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat)
	: Chunk(slotPosX, slotPosZ, lod, viewer, quadTree, mat)
{
	m_debugMeshInstanceRID = RID();
	m_debugMeshRID = RID();
	m_debugMeshInstance = nullptr;
	m_debugWirecubeMeshNeedRegen = false;
	m_debugWiresquareMeshNeedRegen = false;

	VisualServer* vs = VisualServer::get_singleton();
	m_debugMeshInstanceRID = vs->instance_create();

	Ref<World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		vs->instance_set_scenario(m_debugMeshInstanceRID, world->get_scenario());

	vs->instance_set_visible(m_debugMeshInstanceRID, isDebugContentVisible());

	//checkAndCalcDebugMeshAABB;

	//applyDebugMesh();
}

void ChunkDebug::checkAndCalcDebugMeshAABB()
{
	AABB aabb;
	if (m_debugMeshAABB == aabb && m_quadTree->statusInitialized())
	{
		checkAndCalcAABB();
		m_debugMeshAABB = m_aabb;
	}
}

ChunkDebug::~ChunkDebug()
{
	if (m_debugMeshInstanceRID != godot::RID())
	{
		godot::VisualServer::get_singleton()->free_rid(m_debugMeshInstanceRID);
		m_debugMeshInstanceRID = godot::RID();
	}
}

void ChunkDebug::enterWorld(void)
{
	Chunk::enterWorld();

	assert(m_debugMeshInstanceRID != godot::RID());
	godot::Ref<godot::World> world = m_viewer->get_world();
	if (world != nullptr && world.is_valid())
		godot::VisualServer::get_singleton()->instance_set_scenario(m_debugMeshInstanceRID, world->get_scenario());
}

void ChunkDebug::exitWorld(void)
{
	Chunk::exitWorld();
	
	assert(m_debugMeshInstanceRID != godot::RID());
	godot::VisualServer::get_singleton()->instance_set_scenario(m_debugMeshInstanceRID, godot::RID());
}

void ChunkDebug::refresh(bool isVisible)
{
	Chunk::refresh(isVisible);
	setDebugMesh(nullptr);
	checkAndCalcAABB();
	m_debugMeshAABB = m_aabb;
}

void ChunkDebug::heightsChanged(void)
{
	Chunk::heightsChanged();
	m_debugMeshAABB = AABB();
	m_debugWirecubeMeshNeedRegen = true;
	m_debugWiresquareMeshNeedRegen = true;
}

void ChunkDebug::update(bool isVisible)
{
	Chunk::update(isVisible);
}

godot::Transform ChunkDebug::getDebugGlobalTransform(void)
{
	checkAndCalcAABB();
	Transform worldTransform = getGlobalTransformApplied() * godot::Transform(Basis().scaled(m_aabb.size));

	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
		worldTransform.origin.y = 0;

	return worldTransform;
}

void ChunkDebug::onGlobalTransformChanged()
{
	Chunk::onGlobalTransformChanged();

	godot::Transform worldTransform = getDebugGlobalTransform();

	assert(m_debugMeshInstanceRID != godot::RID());
	godot::VisualServer::get_singleton()->instance_set_transform(m_debugMeshInstanceRID, worldTransform);
	
	m_debugGlobaTransformApplied = worldTransform;

	//assert(m_debugGlobaTransformApplied.origin == m_meshGlobaTransformApplied.origin);	// DEBUGRIC
}

Transform ChunkDebug::getDebugGlobalTransformApplied(void)
{
	return m_debugGlobaTransformApplied;
}

void ChunkDebug::setActive(bool b)
{
	Chunk::setActive(b);
}

void ChunkDebug::setVisible(bool b)
{
	Chunk::setVisible(b);
	
	//Chunk::setVisible(false);	// SUPERDEBUGRIC
	//if (m_isCameraVerticalOnChunk)	// SUPERDEBUGRIC
	//	Chunk::setVisible(true);	// SUPERDEBUGRIC
	//else							// SUPERDEBUGRIC
	//	Chunk::setVisible(false);	// SUPERDEBUGRIC

	if (!isVisible())
		b = false;

	if (!isDebugContentVisible())
		b = false;

	assert(m_debugMeshInstanceRID != godot::RID());
	godot::VisualServer::get_singleton()->instance_set_visible(m_debugMeshInstanceRID, b);
}

void ChunkDebug::setDebugContentVisible(bool b)
{
	Chunk::setDebugContentVisible(b);

	assert(m_debugMeshInstanceRID != godot::RID());
	godot::VisualServer::get_singleton()->instance_set_visible(m_debugMeshInstanceRID, isDebugContentVisible());
}

void ChunkDebug::applyAABB(void)
{
	Chunk::applyAABB();

	checkAndCalcDebugMeshAABB();

	assert(m_debugMeshInstanceRID != godot::RID());
	if (isDebugContentVisible())
		if (m_debugMeshRID != godot::RID())
			godot::VisualServer::get_singleton()->instance_set_custom_aabb(m_debugMeshInstanceRID, m_debugMeshAABB);
}

void ChunkDebug::setDebugMesh(godot::Ref<godot::Mesh> mesh)
{
	if (m_isCameraVerticalOnChunk)
		godot::RID();
	
	godot::RID meshRID = (mesh != nullptr ? mesh->get_rid() : godot::RID());

	if (m_debugMeshRID == meshRID)
		return;

	//if (mesh == nullptr)
	//	RID();

	assert(m_debugMeshInstanceRID != godot::RID());
	godot::VisualServer::get_singleton()->instance_set_base(m_debugMeshInstanceRID, meshRID);

	m_debugMeshRID = meshRID;
	m_debugMesh = mesh;
}

bool ChunkDebug::isDebugMeshNull(void)
{
	return (m_debugMeshRID == godot::RID() ? true : false);
}

void ChunkDebug::setCameraPos(Vector3 globalCoordCameraLastPos)
{
	bool prevCameraVerticalOnChunk = isCameraVerticalOnChunk();

	Chunk::setCameraPos(globalCoordCameraLastPos);

	//return;
	
	if (isCameraVerticalOnChunk() && !prevCameraVerticalOnChunk)
	{
		float globalOriginXInGridInWUs = m_viewer->get_global_transform().origin.x + m_originXInWUsLocalToGrid;
		float globalOriginZInGridInWUs = m_viewer->get_global_transform().origin.z + m_originZInWUsLocalToGrid;

		//m_viewer->Globals()->debugPrint("Camera vertical on chunk ("
		//	+ String(to_string(getPos().getSlotPosX()).c_str()) + ":" + to_string(getPos().getSlotPosZ()).c_str()
		//	+ ") of " + m_quadTree->getQuadrant()->getPos().getName().c_str() + " - Chunk pos (Global) = "
		//	+ to_string(globalOriginXInGridInWUs).c_str() + ":" + to_string(globalOriginZInGridInWUs).c_str()
		//	+ " - MinH = " + to_string(m_aabb.position.y).c_str()
		//	+ " - MaxH = " + to_string((m_aabb.position + m_aabb.size).y).c_str()
		//	+ " - Chunk Size in WUs = " + to_string(getChunkSizeInWUs()).c_str()
		//);
		
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
	
	checkAndCalcAABB();

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
			godot::Ref<godot::Mesh> _mesh = createWireCubeMesh(GDN_TheWorld_Globals::g_color_blue);
			m_viewer->getMainProcessingMutex().lock();
			godot::SpatialMaterial* mat = godot::SpatialMaterial::_new();
			m_viewer->getMainProcessingMutex().unlock();
			mat->set_flag(godot::SpatialMaterial::Flags::FLAG_UNSHADED, true);
			mat->set_flag(godot::SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
			mat->set_albedo(godot::GDN_TheWorld_Globals::g_color_blue);
			_mesh->surface_set_material(0, mat);
			mesh = _mesh;
		}
		else
		{
			// reset normal Wirecube
			string metaNameMesh = DEBUG_WIRECUBE_MESH + to_string(m_lod);
			
			if (m_debugWirecubeMeshNeedRegen)
			{
				if (m_viewer->has_meta(metaNameMesh.c_str()))
					m_viewer->remove_meta(metaNameMesh.c_str());
				m_debugWirecubeMeshNeedRegen = false;
			}

			if (!m_viewer->has_meta(metaNameMesh.c_str()))
			{
				godot::Color wiredMeshColor;
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

				godot::Ref<godot::ArrayMesh> _mesh = createWireCubeMesh(wiredMeshColor);
				m_viewer->getMainProcessingMutex().lock();
				godot::SpatialMaterial* mat = godot::SpatialMaterial::_new();
				m_viewer->getMainProcessingMutex().unlock();
				mat->set_flag(godot::SpatialMaterial::Flags::FLAG_UNSHADED, true);
				mat->set_flag(godot::SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
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
			godot::Color wiredMeshColor = GDN_TheWorld_Globals::g_color_blue;
			godot::Ref<godot::Mesh> _mesh = createWireSquareMesh(wiredMeshColor);
			m_viewer->getMainProcessingMutex().lock();
			godot::SpatialMaterial* mat = godot::SpatialMaterial::_new();
			m_viewer->getMainProcessingMutex().unlock();
			mat->set_flag(godot::SpatialMaterial::Flags::FLAG_UNSHADED, true);
			mat->set_flag(godot::SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
			mat->set_albedo(wiredMeshColor);
			_mesh->surface_set_material(0, mat);
			mesh = _mesh;
		}
		else
		{
			// reset normal Wirecube
			string metaNameMesh = DEBUG_WIRESQUARE_MESH + to_string(m_lod);

			if (m_debugWiresquareMeshNeedRegen)
			{
				if (m_viewer->has_meta(metaNameMesh.c_str()))
					m_viewer->remove_meta(metaNameMesh.c_str());
				m_debugWiresquareMeshNeedRegen = false;
			}

			if (!m_viewer->has_meta(metaNameMesh.c_str()))
			{
				godot::Color wiredMeshColor;
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

				godot::Ref<godot::ArrayMesh> _mesh = createWireSquareMesh(wiredMeshColor);
				m_viewer->getMainProcessingMutex().lock();
				godot::SpatialMaterial* mat = godot::SpatialMaterial::_new();
				m_viewer->getMainProcessingMutex().unlock();
				mat->set_flag(godot::SpatialMaterial::Flags::FLAG_UNSHADED, true);
				mat->set_flag(godot::SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
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

	godot::Transform worldTransform = getDebugGlobalTransform();
	godot::VisualServer::get_singleton()->instance_set_transform(m_debugMeshInstanceRID, worldTransform);
	m_debugGlobaTransformApplied = worldTransform;

	/*GDN_TheWorld_Globals* globals = m_viewer->Globals();
	globals->debugPrint(String("Debug Mesh - ID: ") + getPos().getPos().c_str()
		+ String(" POS ==> ") + String("X = ") + String(to_string(worldTransform.origin.x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(worldTransform.origin.y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(worldTransform.origin.z).c_str())
		+ (isCameraVerticalOnChunk() ? "CAMERA" : ""));*/
}

godot::Ref<godot::ArrayMesh> ChunkDebug::createWireCubeMesh(Color c)
{
	godot::PoolVector3Array positions;
	godot::PoolColorArray colors;
	godot::PoolIntArray indices;

	positions.append(godot::Vector3(0, 0, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 0, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 0, 1));		colors.append(c);
	positions.append(godot::Vector3(0, 0, 1));		colors.append(c);
	positions.append(godot::Vector3(0, 1, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 1, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 1, 1));		colors.append(c);
	positions.append(godot::Vector3(0, 1, 1));		colors.append(c);

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
	arrays.resize(godot::ArrayMesh::ARRAY_MAX);
	arrays[godot::ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[godot::ArrayMesh::ARRAY_COLOR] = colors;
	arrays[godot::ArrayMesh::ARRAY_INDEX] = indices;

	m_viewer->getMainProcessingMutex().lock();
	godot::Ref<godot::ArrayMesh> mesh = godot::ArrayMesh::_new();
	m_viewer->getMainProcessingMutex().unlock();
	mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_LINES, arrays);
	
	return mesh;
}

godot::Ref<godot::ArrayMesh> ChunkDebug::createWireSquareMesh(Color c)
{
	godot::PoolVector3Array positions;
	godot::PoolColorArray colors;
	godot::PoolIntArray indices;

	positions.append(godot::Vector3(0, 0, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 0, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 0, 1));		colors.append(c);
	positions.append(godot::Vector3(0, 0, 1));		colors.append(c);

	// square
	indices.append(0); indices.append(1);
	indices.append(1); indices.append(2);
	indices.append(2); indices.append(3);
	indices.append(3); indices.append(0);
	// diagonals
	indices.append(0); indices.append(2);
	indices.append(1); indices.append(3);

	godot::Array arrays;
	arrays.resize(godot::ArrayMesh::ARRAY_MAX);
	arrays[godot::ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[godot::ArrayMesh::ARRAY_COLOR] = colors;
	arrays[godot::ArrayMesh::ARRAY_INDEX] = indices;

	m_viewer->getMainProcessingMutex().lock();
	godot::Ref<godot::ArrayMesh> mesh = godot::ArrayMesh::_new();
	m_viewer->getMainProcessingMutex().unlock();
	mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_LINES, arrays);

	return mesh;
}

void ChunkDebug::dump(void)
{
	Chunk::dump();

	if (!isActive())
		return;

	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	globals->debugPrint(String("Chunk ID: ") + getPos().getIdStr().c_str()
		+ " DEBUG MESH - MinH = " + to_string(m_debugMeshAABB.position.y).c_str()
		+ " - MaxH = " + to_string((m_debugMeshAABB.position + m_debugMeshAABB.size).y).c_str()
		+ (m_isCameraVerticalOnChunk ? " - CAMERA" : ""));
}

//void GDN_Chunk_MeshInstance::_register_methods()
//{
//	register_method("_ready", &GDN_Chunk_MeshInstance::_ready);
//	register_method("_process", &GDN_Chunk_MeshInstance::_process);
//	register_method("_input", &GDN_Chunk_MeshInstance::_input);
//
//	register_method("get_lod", &GDN_Chunk_MeshInstance::getLod);
//	register_method("get_slot_pos_x", &GDN_Chunk_MeshInstance::getSlotPosX);
//	register_method("get_slot_pos_z", &GDN_Chunk_MeshInstance::getSlotPosZ);
//	register_method("get_id", &GDN_Chunk_MeshInstance::getIdStr);
//}
//
//GDN_Chunk_MeshInstance::GDN_Chunk_MeshInstance()
//{
//	m_initialized = false;
//	m_chunk = nullptr;
//}
//
//GDN_Chunk_MeshInstance::~GDN_Chunk_MeshInstance()
//{
//	deinit();
//}
//
//void GDN_Chunk_MeshInstance::init(Chunk* chunk)
//{
//	m_chunk = chunk;
//}
//
//void GDN_Chunk_MeshInstance::deinit(void)
//{
//	if (m_initialized)
//	{
//		m_initialized = false;
//	}
//}
//
//void GDN_Chunk_MeshInstance::_init(void)
//{
//	//Godot::print("GDN_Template::Init");
//}
//
//void GDN_Chunk_MeshInstance::_ready(void)
//{
//	//Godot::print("GDN_Template::_ready");
//	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
//}
//
//void GDN_Chunk_MeshInstance::_input(const Ref<InputEvent> event)
//{
//}
//
//void GDN_Chunk_MeshInstance::_process(float _delta)
//{
//	// To activate _process method add this Node to a Godot Scene
//	//Godot::print("GDN_Template::_process");
//}
//
//int GDN_Chunk_MeshInstance::getLod(void)
//{
//	return m_chunk->getLod();
//}
//
//int GDN_Chunk_MeshInstance::getSlotPosX(void)
//{
//	return m_chunk->getSlotPosX();
//}
//
//int GDN_Chunk_MeshInstance::getSlotPosZ(void)
//{
//	return m_chunk->getSlotPosZ();
//}
//
//String GDN_Chunk_MeshInstance::getIdStr(void)
//{
//	return m_chunk->getIdStr().c_str();
//}
