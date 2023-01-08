//#include "pch.h"
#include "QuadTree.h"
#include "TheWorld_Utils.h"

#include <File.hpp>
#include <OS.hpp>

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Quadrant.h"

#include "assert.h"
#include <filesystem>

using namespace godot;
namespace fs = std::filesystem;

// QuadTree coordinates are local to Viewer Node which contains the grid of quads: the first chunk (posX = posZ = 0) has its lower vertex in the origin of the Viwer Node

Quad::Quad(int slotPosX, int slotPosZ, int lod, enum PosInQuad posInQuad, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
{
	m_slotPosX = slotPosX;
	m_slotPosZ = slotPosZ;
	m_lod = lod;
	m_posInQuad = posInQuad;
	m_viewer = viewer;
	m_chunk = nullptr;
	m_chunkSizeInWUs = 0;
	m_splitRequired = false;

	m_quadTree = quadTree;
	
	assignChunk();
}

Quad::~Quad()
{
	clearChildren();
}

Quad* Quad::getChild(int idx)
{
	if (isLeaf())
		return nullptr;

	return m_children[idx].get();
}

bool Quad::isLeaf(void)
{
	return (m_children[0].get() == nullptr ? true : false);
}

void Quad::split(void)
{
	assert(m_lod > 0);
	assert(m_children[0] == nullptr);
	
	for (int i = 0; i < 4; i++)
	{
		//int x = (m_slotPosX * 2) + (i & 1);
		//int z = (m_slotPosZ * 2) + ((i & 2) >> 1);
		m_children[i] = make_unique<Quad>((m_slotPosX * 2) + (i & 1), (m_slotPosZ * 2) + ((i & 2) >> 1), m_lod - 1, enum PosInQuad(i + 1), m_viewer, m_quadTree);
	}
	
	recycleChunk();
}

void Quad::recycleChunk(void)
{
	if (m_chunk)
	{
		m_chunk->setPosInQuad(PosInQuad::NotSet);
		m_chunk->setVisible(false);
		m_chunk->setActive(false);
		m_chunk = nullptr;
	}
}

void Quad::clearChildren(void)
{
	assert(m_children.size() == 0 || m_children.size() == 4);
	if (m_children.size() != 0)
		for (int i = 0; i < 4; i++)
		{
			if (m_children[i] != nullptr)
			{
				m_children[i]->recycleChunk();
				m_children[i].reset();
			}
		}
}

void Quad::assignChunk(void)
{
	assert(m_chunk == nullptr);
	
	m_chunk = m_quadTree->getChunkAt(Chunk::ChunkPos(m_slotPosX, m_slotPosZ, m_lod));

	if (m_chunk == nullptr)
	{
		// TODORIC Material stuff

		// create chunk for current quad because this is the first time that is needed for current lod and pos
		Ref<Material> mat = m_quadTree->getQuadrant()->getShaderTerrainData()->getMaterial();
		if (_DEBUG_AAB)
			m_chunk = new ChunkDebug(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);
		else
			m_chunk = new Chunk(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);

		// Set Viewer Pos in Global Coord
		//m_chunk->setParentGlobalTransform(m_viewer->internalTransformGlobalCoord());
		m_quadTree->addChunk(m_chunk);
	}
	m_chunk->setPosInQuad(m_posInQuad, this);
	m_quadTree->addChunkUpdate(m_chunk);
	m_chunk->setActive(true);
	m_chunkAABB = m_chunk->getAABB();
	m_chunkSizeInWUs = m_chunk->getChunkSizeInWUs();
}

void Quad::setCameraPos(Vector3 globalCoordCameraLastPos)
{
	if (isLeaf())
	{
		Chunk* chunk = getChunk();
		if (chunk != nullptr)
			chunk->setCameraPos(globalCoordCameraLastPos);
	}
}
	
QuadTree::QuadTree(GDN_TheWorld_Viewer* viewer, QuadrantPos quadrantPos)
{
	setStatus(QuadrantStatus::uninitialized);
	m_isVisible = false;
	m_viewer = viewer;
	m_numSplits = 0;
	m_numJoins = 0;
	m_numLeaf = 0;
	m_worldQuadrant = new Quadrant(quadrantPos, viewer, this);
	m_GDN_Quadrant = nullptr;
	m_tag = quadrantPos.getTag();
	int ret = timespec_get(&m_refreshTime, TIME_UTC);
}

void QuadTree::init(float viewerPosX, float viewerPosZ, bool setCamera, float cameraDistanceFromTerrain)
{
	//TheWorld_Utils::TimerMs clock;
	//clock.tick();

	if (status() != QuadrantStatus::uninitialized)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not comply: " + std::to_string((int)status())).c_str()));

	std::lock_guard lock(m_mtxQuadrant);

	m_GDN_Quadrant = GDN_TheWorld_Quadrant::_new();
	m_GDN_Quadrant->init(this);
	std::string quadrantNodeName = m_worldQuadrant->getPos().getName();
	m_GDN_Quadrant->set_name(quadrantNodeName.c_str());
	m_viewer->add_child(m_GDN_Quadrant);
	onGlobalTransformChanged();

	setStatus(QuadrantStatus::getVerticesInProgress);

	TheWorld_Utils::TimerMs clock1;
	clock1.tick();
	m_worldQuadrant->populateGridVertices(viewerPosX, viewerPosZ, setCamera, cameraDistanceFromTerrain);
	clock1.tock();	
	//m_viewer->Globals()->debugPrint(String("ELAPSED - QUADRANT ") + m_worldQuadrant->getId().getId().c_str() + " TAG=" + m_tag.c_str() + " - QuadTree::init (populateGridVertices) " + std::to_string(clock1.duration().count()).c_str() + " ms");

	getQuadrant()->getShaderTerrainData()->init();

	m_root = make_unique<Quad>(0, 0, m_viewer->Globals()->lodMaxDepth(), PosInQuad::Root, m_viewer, this);

	//m_worldQuadrant->getCollider()->init(m_GDN_Quadrant, 1, 1);
	//m_worldQuadrant->getCollider()->enterWorld();

	//clock.tock();
	//m_viewer->Globals()->debugPrint(String("ELAPSED - QUADRANT ") + m_worldQuadrant->getId().getId().c_str() + " TAG=" + m_tag.c_str() + " - QuadTree::init " + std::to_string(clock.duration().count()).c_str() + " ms");
}

void QuadTree::onGlobalTransformChanged(void)
{
	QuadrantPos quadrantPos = getQuadrant()->getPos();
	Transform gt = m_viewer->get_global_transform();
	Transform t = gt * Transform(Basis(), Vector3(quadrantPos.getLowerXGridVertex(), 0, quadrantPos.getLowerZGridVertex()));
	m_GDN_Quadrant->set_global_transform(t);
}

QuadTree::~QuadTree()
{
	if (m_GDN_Quadrant)
	{
		m_GDN_Quadrant->deinit();
		Node* parent = m_GDN_Quadrant->get_parent();
		if (parent)
			parent->remove_child(m_GDN_Quadrant);
		m_GDN_Quadrant->queue_free();
		m_GDN_Quadrant = nullptr;
	}

	m_root.reset();
	
	clearChunkUpdate();
	
	// Delete all Chunks
	for (Chunk::MapChunk::iterator it = m_mapChunk.begin(); it != m_mapChunk.end(); it++)
	{
		for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
		{
			delete it1->second;
		}
		it->second.clear();
	}
	m_mapChunk.clear();

	if (m_worldQuadrant)
		delete m_worldQuadrant;
}

void QuadTree::ForAllChunk(Chunk::ChunkAction& chunkAction)
{
	if (!isValid())
		return;

	for (Chunk::MapChunk::iterator it = m_mapChunk.begin(); it != m_mapChunk.end(); it++)
	{
		for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
		{
			chunkAction.exec(it1->second);
		}
	}
}

void QuadTree::update(Vector3 cameraPosGlobalCoord, enum class UpdateStage updateStage, int& numSplitRequired)
{
	if (!isValid())
		return;

	if (!isVisible())
		return;

	m_numLeaf = 0;
	internalUpdate(cameraPosGlobalCoord, m_root.get(), updateStage, numSplitRequired);
	assert(!m_root->isLeaf() || m_root->getChunk() != nullptr);
}

void QuadTree::internalUpdate(Vector3 cameraPosGlobalCoord, Quad* quad, enum class UpdateStage updateStage, int& numSplitRequired)
{
	if (updateStage == UpdateStage::Stage1)
	{
		// cameraPosViewerNodeLocalCoord are in grid local coordinates (WUs)
		GDN_TheWorld_Globals* globals = m_viewer->Globals();
		float chunkSizeInWUs = globals->gridStepInHeightmapWUs(quad->Lod()) * globals->numVerticesPerChuckSide();									// chunk size in World Units
		//float chunkSizeInWUs = quad->getChunkSizeInWUs();
		//Vector3 chunkCenter = real_t(chunkSizeInWUs) * (Vector3(real_t(quad->slotPosX()), 0, real_t(quad->slotPosZ())) + Vector3(0.5, 0, 0.5));	// chunk center in local coord. respect Viewer Node (or the grid of chunks)
		//chunkCenter.y = (chunkAABB.position + chunkAABB.size / 2).y;		// medium altitude of the chunk
		AABB chunkAABB = quad->getChunkAABB();
		Vector3 quadCenterGlobal(real_t((quad->slotPosX() * chunkSizeInWUs) + m_worldQuadrant->getPos().getLowerXGridVertex() + chunkSizeInWUs / 2),
			(chunkAABB.position + chunkAABB.size / 2).y,
			real_t((quad->slotPosZ() * chunkSizeInWUs) + m_worldQuadrant->getPos().getLowerZGridVertex() + chunkSizeInWUs / 2));

		Transform gt = m_viewer->get_global_transform();
		Transform t = gt * Transform(Basis(), quadCenterGlobal);
		quadCenterGlobal = t.origin;


		real_t splitDistance = chunkSizeInWUs * globals->splitScale();
		if (quad->isLeaf())
		{
			bool trackChunk = false;
			if (m_viewer->trackMouse() && getQuadrant()->getPos().getName() == m_viewer->getMouseQuadrantHitName())
				trackChunk = true;

			bool splitted = false;
			m_numLeaf++;
			if (quad->Lod() > 0)
			{
				real_t distance = quadCenterGlobal.distance_to(cameraPosGlobalCoord);
				quad->getChunk()->setDistanceFromCamera(distance);
				if (distance < splitDistance)
				{
					// Split
					quad->split();
					for (int i = 0; i < 4; i++)
					{
						quad->getChild(i)->setCameraPos(cameraPosGlobalCoord);
						if (trackChunk)
							quad->getChild(i)->getChunk()->checkMouseHit();
						quad->getChild(i)->getChunk()->setDistanceFromCamera(distance);
					}
					splitted = true;
					m_numSplits++;
				}
			}

			if (!splitted && trackChunk)
				quad->getChunk()->checkMouseHit();
		}
		else
		{
			bool allChildrenAreLeaf = true;
			for (int i = 0; i < 4; i++)
			{
				Quad* child = quad->getChild(i);
				internalUpdate(cameraPosGlobalCoord, child, updateStage, numSplitRequired);
				if (!child->isLeaf())
					allChildrenAreLeaf = false;
			}


			if (allChildrenAreLeaf)
			{
				real_t distance = quadCenterGlobal.distance_to(cameraPosGlobalCoord);
				if (distance > splitDistance)
				{
					// Join
					quad->clearChildren();
					quad->assignChunk();
					quad->getChunk()->setJustJoined(true);
					m_numJoins++;
					quad->getChunk()->checkMouseHit();
					quad->getChunk()->setDistanceFromCamera(distance);
				}
			}
		}

		// after all if quad is leaf: just joined or was leaf and it has not been splitted
		quad->setCameraPos(cameraPosGlobalCoord);
	}
	else if (updateStage == UpdateStage::Stage2)
	{
		if (quad->isLeaf())
		{
			PosInQuad posInQuad = quad->getPosInQuad();
			Chunk* chunk = quad->getChunk();
			assert(chunk != nullptr);
			
			if (chunk != nullptr)
			{
				switch (posInQuad)
				{
					case PosInQuad::First:
					{
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XMinusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZMinusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
						}
					}
					break;
					case PosInQuad::Second:
					{
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XPlusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZMinusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
						}
					}
					break;
					case PosInQuad::Third:
					{
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XMinusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZPlusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
						}
					}
					break;
					case PosInQuad::Forth:
					{
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XPlusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZPlusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
						}
					}
					break;
				}
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
				internalUpdate(cameraPosGlobalCoord, quad->getChild(i), updateStage, numSplitRequired);
		}

	}
	else
	{
		// (updateStage == UpdateStage::Stage3)
		if (quad->isLeaf())
		{
			if (quad->splitRequired() && quad->Lod() > 0)
			{
				// Split
				quad->split();
				for (int i = 0; i < 4; i++)
				{
					quad->getChild(i)->setCameraPos(cameraPosGlobalCoord);
				}
				m_numSplits++;
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
				internalUpdate(cameraPosGlobalCoord, quad->getChild(i), updateStage, numSplitRequired);
		}

		quad->setSplitRequired(false);
	}
}

bool QuadTree::isChunkOnBorderOfQuadrant(Chunk::ChunkPos pos, QuadrantPos& XMinusQuadrantPos, QuadrantPos& XPlusQuadrantPos, QuadrantPos& ZMinusQuadrantPos, QuadrantPos& ZPlusQuadrantPos)
{
	bool ret = false;

	if (pos.getSlotPosX() == 0)
	{
		XMinusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XMinus);
		ret = true;
	}

	if (pos.getSlotPosZ() == 0)
	{
		ZMinusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZMinus);
		ret = true;
	}

	int numChunksPerSide = m_viewer->Globals()->numChunksPerHeightmapSide(pos.getLod());

	if (pos.getSlotPosX() == (numChunksPerSide - 1))
	{
		XPlusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XPlus);
		ret = true;
	}

	if (pos.getSlotPosZ() == (numChunksPerSide - 1))
	{
		ZPlusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZPlus);
		ret = true;
	}

	return ret;
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir)
{
	Chunk* chunk = nullptr;

	switch (dir)
	{
	case Chunk::DirectionSlot::Center:
		chunk = getChunkAt(pos);
		break;
	case Chunk::DirectionSlot::XMinusChunk:
		//
		// - o
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
		break;
	case Chunk::DirectionSlot::XPlusChunk:
		//
		//   o -
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 0, -1));
		break;
	case Chunk::DirectionSlot::ZMinusChunk:
		//   -
		//   o
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
		break;
	case Chunk::DirectionSlot::ZPlusChunk:
		//
		//   o
		//   -
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 1, -1));
		break;
	case Chunk::DirectionSlot::XMinusQuadSecondChunk:
		//  
		//	- o =
		//	  = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
		break;
	case Chunk::DirectionSlot::XMinusQuadForthChunk:
		//  
		//	  o =
		//	- = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 1, -1));
		break;
	case Chunk::DirectionSlot::XPlusQuadFirstChunk:
		//  
		//	  o = -
		//	  = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 0, -1));
		break;
	case Chunk::DirectionSlot::XPlusQuadThirdChunk:
		//  
		//	  o = 
		//	  = = -
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 1, -1));
		break;
	case Chunk::DirectionSlot::ZMinusQuadThirdChunk:
		//    -
		//	  o =
		//	  = =
		//    
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
		break;
	case Chunk::DirectionSlot::ZMinusQuadForthChunk:
		//      -
		//	  o =
		//	  = =
		//    
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, -1, -1));
		break;
	case Chunk::DirectionSlot::ZPlusQuadFirstChunk:
		//    
		//	  o =
		//	  = =
		//    -
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 2, -1));
		break;
	case Chunk::DirectionSlot::ZPlusQuadSecondChunk:
		//      
		//	  o =
		//	  = =
		//      -
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 2, -1));
		break;
	//default:
	}

	return chunk;
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos)
{
	Chunk::MapChunk::iterator it = m_mapChunk.find(pos.getLod());
	if (it == m_mapChunk.end())
		return nullptr;
	else
	{
		Chunk::MapChunkPerLod::iterator it1 = m_mapChunk[pos.getLod()].find(pos);
		if (it1 == m_mapChunk[pos.getLod()].end())
			return nullptr;
		else
			return m_mapChunk[pos.getLod()][pos];
	}
}

void QuadTree::addChunk(Chunk* chunk)
{
	if (getChunkAt(chunk->getPos()))
		return;
	
	Chunk::MapChunk::iterator it = m_mapChunk.find(chunk->getLod());
	if (it == m_mapChunk.end())
	{
		Chunk::MapChunkPerLod m;
		m_mapChunk[chunk->getLod()] = m;
	}

	m_mapChunk[chunk->getLod()][chunk->getPos()] = chunk;
}

void QuadTree::addChunkUpdate(Chunk* chunk)
{
	if (chunk->isPendingUpdate())
		return;

	m_vectChunkUpdate.push_back(chunk);
	chunk->setPendingUpdate(true);
}

void QuadTree::clearChunkUpdate(void)
{
	m_vectChunkUpdate.clear();
}

void QuadTree::updateMaterialParams(void)
{
	if (!isValid())
		return;
	
	if (!isVisible())
		return;

	if (getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate())
	{
		TheWorld_Utils::TimerMs clock;
		clock.tick();
		getQuadrant()->getShaderTerrainData()->updateMaterialParams();
		clock.tock();	m_viewer->Globals()->debugPrint(String("ELAPSED - QUADRANT ") + m_worldQuadrant->getPos().getIdStr().c_str() + " TAG=" + m_tag.c_str() + " - updateMaterialParams " + std::to_string(clock.duration().count()).c_str() + " ms");
		getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate(false);
	}
}

void QuadTree::resetMaterialParams()
{
	if (materialParamsNeedReset())
	{
		m_worldQuadrant->getCollider()->deinit();
		m_worldQuadrant->getCollider()->init(m_GDN_Quadrant, 1, 1);
		m_worldQuadrant->getCollider()->enterWorld();
		m_worldQuadrant->getCollider()->setData();

		getQuadrant()->getShaderTerrainData()->resetMaterialParams();
		materialParamsNeedReset(false);
	}
}

bool QuadTree::materialParamsNeedReset(void)
{
	return getQuadrant()->getShaderTerrainData()->materialParamsNeedReset();
}

void QuadTree::materialParamsNeedReset(bool b)
{
	getQuadrant()->getShaderTerrainData()->materialParamsNeedReset(b);
}

bool QuadTree::materialParamsNeedUpdate(void)
{
	return getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate();
}

void QuadTree::materialParamsNeedUpdate(bool b)
{
	getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate(b);
}

Transform QuadTree::getInternalGlobalTransform(void)
{
	Transform t(Basis(), Vector3((real_t)getQuadrant()->getPos().getLowerXGridVertex(), 0, (real_t)getQuadrant()->getPos().getLowerZGridVertex()));
	Transform gt = m_viewer->getInternalGlobalTransform();
	Transform t1 = gt * t;
	return t1;
}

void QuadTree::dump(void)
{
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	globals->debugPrint("====================================================================================================================================");

	globals->debugPrint(String("DUMP QUADRANT ") + getQuadrant()->getPos().getIdStr().c_str() + " TAG=" + m_tag.c_str());

	if (isValid() && isVisible())
	{
		globals->debugPrint(String("Num vertices per chunk side: ") + String(std::to_string(globals->numVerticesPerChuckSide()).c_str())
			+ String(" - ") + String("Num vertices per grid size: ") + String(std::to_string(globals->heightmapResolution()).c_str())
			+ String(" - ") + String("Grid size [WUs]: ") + String(std::to_string(globals->heightmapSizeInWUs()).c_str())
			+ String(" - ") + String("Lod max depth: ") + String(std::to_string(globals->lodMaxDepth()).c_str()));

		globals->debugPrint(String("FROM LAST DUMP - Num quad split: ") + String(std::to_string(m_numSplits).c_str())
			+ String(" - ") + String("Num quad join: ") + String(std::to_string(m_numJoins).c_str())
			+ String(" - ") + String("Num leaves: ") + String(std::to_string(m_numLeaf).c_str()));

		globals->debugPrint(String("GRID POS (global)")
			+ String(" - ") + String("X = ") + String(std::to_string(getInternalGlobalTransform().origin.x).c_str())
			+ String(" - ") + String("Z = ") + String(std::to_string(getInternalGlobalTransform().origin.z).c_str()));

		int numChunks = 0;
		int numActiveChunks = 0;
		Chunk* cameraChunk = nullptr;
		for (int i = globals->lodMaxDepth(); i >= 0; i--)
		{
			int numChunksAtLod = 0;
			Chunk::MapChunk::iterator it = m_mapChunk.find(i);
			if (it != m_mapChunk.end())
				numChunksAtLod = int(m_mapChunk[i].size());
			int numChunksActiveAtLod = 0;
			for (Chunk::MapChunkPerLod::iterator it1 = m_mapChunk[i].begin(); it1 != m_mapChunk[i].end(); it1++)
			{
				if (it1->second->isActive())
					numChunksActiveAtLod++;
				if (it1->second->isCameraVerticalOnChunk())
					cameraChunk = it1->second;
			}
			globals->debugPrint("Num chunks at lod " + String(std::to_string(i).c_str()) + ": " + String(std::to_string(numChunksAtLod).c_str())
				+ String("\t") + " Active " + String(std::to_string(numChunksActiveAtLod).c_str()));
			numChunks += numChunksAtLod;
			numActiveChunks += numChunksActiveAtLod;
		}

		globals->debugPrint("Num chunks: " + String(std::to_string(numChunks).c_str()));
		globals->debugPrint("Num active chunks: " + String(std::to_string(numActiveChunks).c_str()));

		// DEBUGRIC
		//if (cameraChunk != nullptr)
		//{
		//	Ref<Mesh> cameraMesh = cameraChunk->getMesh();
		//	Array cameraArrays = cameraMesh->surface_get_arrays(0);
		//	PoolVector3Array cameraVerts = cameraArrays[Mesh::ARRAY_VERTEX];
		//	globals->debugPrint("camerMesh Verts: " + String(std::to_string(cameraVerts.size()).c_str()));
		//	Transform cameraT = cameraChunk->getMeshGlobalTransformApplied();
		//	globals->debugPrint("camerMesh t= " + String(cameraT));
		//	int numPointsPerSize = (int)sqrt(cameraVerts.size());
		//	String s;	int i = 0;
		//	for (int z = 0; z < numPointsPerSize; z++)
		//	{
		//		for (int x = 0; x < numPointsPerSize; x++)
		//		{
		//			s += cameraVerts[i];
		//			s += " ";
		//			i++;
		//		}
		//		globals->debugPrint("camerMesh r=" + String(std::to_string(z + 1).c_str()) + " " + s);
		//		s = "";
		//	}

		//	Chunk* cameraChunkXMinus = getChunkAt(cameraChunk->getPos(), Chunk::DirectionSlot::XMinusChunk);
		//	if (cameraChunkXMinus != nullptr)
		//	{
		//		Ref<Mesh> camerMeshXMinus = cameraChunkXMinus->getMesh();
		//		Array cameraArraysXMinus = camerMeshXMinus->surface_get_arrays(0);
		//		PoolVector3Array cameraVertsXMinus = cameraArraysXMinus[Mesh::ARRAY_VERTEX];
		//		globals->debugPrint("camerMeshXMinus Verts: " + String(std::to_string(cameraVertsXMinus.size()).c_str()));
		//		Transform cameraT = cameraChunkXMinus->getMeshGlobalTransformApplied();
		//		globals->debugPrint("camerMeshXMinus t= " + String(cameraT));
		//		int numPointsPerSize = (int)sqrt(cameraVertsXMinus.size());
		//		String s;	int i = 0;
		//		for (int z = 0; z < numPointsPerSize; z++)
		//		{
		//			for (int x = 0; x < numPointsPerSize; x++)
		//			{
		//				s += cameraVertsXMinus[i];
		//				s += " ";
		//				i++;
		//			}
		//			globals->debugPrint("camerMeshXMinus r=" + String(std::to_string(z + 1).c_str()) + " " + s);
		//			s = "";
		//		}
		//	}
		//	Chunk* cameraChunkXPlus = getChunkAt(cameraChunk->getPos(), Chunk::DirectionSlot::XPlusChunk);
		//	if (cameraChunkXPlus != nullptr)
		//	{
		//		Ref<Mesh> camerMeshXPlus = cameraChunkXPlus->getMesh();
		//		Array cameraArraysXPlus = camerMeshXPlus->surface_get_arrays(0);
		//		PoolVector3Array cameraVertsXPlus = cameraArraysXPlus[Mesh::ARRAY_VERTEX];
		//		globals->debugPrint("camerMeshXPlus Verts: " + String(std::to_string(cameraVertsXPlus.size()).c_str()));
		//		Transform cameraT = cameraChunkXPlus->getMeshGlobalTransformApplied();
		//		globals->debugPrint("camerMeshXPlus t= " + String(cameraT));
		//		int numPointsPerSize = (int)sqrt(cameraVertsXPlus.size());
		//		String s;	int i = 0;
		//		for (int z = 0; z < numPointsPerSize; z++)
		//		{
		//			for (int x = 0; x < numPointsPerSize; x++)
		//			{
		//				s += cameraVertsXPlus[i];
		//				s += " ";
		//				i++;
		//			}
		//			globals->debugPrint("camerMeshXPlus r=" + String(std::to_string(z + 1).c_str()) + " " + s);
		//			s = "";
		//		}
		//	}
		//}
		// DEBUGRIC

		Chunk::DumpChunkAction action;
		ForAllChunk(action);
	}
	else
	{
		if (!isValid())
			globals->debugPrint("QUADRANT NOT VALID YET");
		else
			globals->debugPrint("QUADRANT ACTUALLY NOT VISIBLE");
	}


	globals->debugPrint(String("COMPLETED DUMP QUADRANT ") + getQuadrant()->getPos().getIdStr().c_str() + " TAG=" + m_tag.c_str());

	globals->debugPrint("====================================================================================================================================");

	m_numSplits = 0;
	m_numJoins = 0;
}

ShaderTerrainData::ShaderTerrainData(GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
{
	m_viewer = viewer;
	m_quadTree = quadTree;
	m_materialParamsNeedUpdate = false;
	m_materialParamsNeedReset = false;
	m_heightMapTexModified = false;
	m_normalMapTexModified = false;
	//m_splat1MapTexModified = false;
	//m_colorMapTexModified = false;
}

ShaderTerrainData::~ShaderTerrainData()
{
}

void ShaderTerrainData::init(void)
{
	Ref<ShaderMaterial> mat = ShaderMaterial::_new();
	ResourceLoader* resLoader = ResourceLoader::get_singleton();
	String shaderPath = "res://shaders/lookdev.shader";
	//String shaderPath = "res://shaders/test.shader";		// SUPERDEBUGRIC
	Ref<Shader> shader = resLoader->load(shaderPath);
	mat->set_shader(shader);

	m_material = mat;
}

// it is expected that globals and World Datas are loaded
// TODORIC: maybe usefull for performance reasons specify which texture need update and which rect of the texture 
void ShaderTerrainData::resetMaterialParams(void)
{
	std::recursive_mutex& quadrantMutex = m_quadTree->getQuadrantMutex();
	std::lock_guard lock(quadrantMutex);
	
	int _resolution = m_viewer->Globals()->heightmapResolution() + 1;

	bool loadingHeighmapOK = false;
	m_heightMapImage = m_quadTree->getQuadrant()->getMeshCacheBuffer().readImage(loadingHeighmapOK, TheWorld_Utils::MeshCacheBuffer::ImageType::heightmap);
	bool loadingNormalmapOK = false;
	m_normalMapImage = m_quadTree->getQuadrant()->getMeshCacheBuffer().readImage(loadingNormalmapOK, TheWorld_Utils::MeshCacheBuffer::ImageType::normalmap);

	if (!loadingHeighmapOK || !loadingNormalmapOK)
	{
		// Creating Heightmap Map Texture
		{
			Ref<Image> image = Image::_new();
			image->create(_resolution, _resolution, false, Image::FORMAT_RH);
			m_heightMapImage = image;
		}

		// Creating Normal Map Texture
		{
			Ref<Image> image = Image::_new();
			image->create(_resolution, _resolution, false, Image::FORMAT_RGB8);
			m_normalMapImage = image;
		}

		{
			std::vector<TheWorld_Utils::GridVertex>& gridVertices = m_quadTree->getQuadrant()->getGridVertices();

			// Filling Heightmap Map Texture , Normal Map Texture
			assert(_resolution == m_heightMapImage->get_height());
			assert(_resolution == m_heightMapImage->get_width());
			assert(_resolution == m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize());
			float gridStepInWUs = m_viewer->Globals()->gridStepInWU();
			m_heightMapImage->lock();
			m_normalMapImage->lock();
			for (int z = 0; z < _resolution; z++)			// m_heightMapImage->get_height()
				for (int x = 0; x < _resolution; x++)		// m_heightMapImage->get_width()
				{
					float h = gridVertices[z * _resolution + x].altitude();
					m_heightMapImage->set_pixel(x, z, Color(h, 0, 0));
					//if (h != 0)	// DEBUGRIC
					//	m_viewer->Globals()->debugPrint("Altitude not null.X=" + String(std::to_string(x).c_str()) + " Z=" + std::to_string(z).c_str() + " H=" + std::to_string(h).c_str());

					// h = height of the point for which we are computing the normal
					// hr = height of the point on the rigth
					// hl = height of the point on the left
					// hf = height of the forward point (z growing)
					// hb = height of the backward point (z lessening)
					// step = step in WUs between points
					// we compute normal normalizing the vector (h - hr, step, h - hf) or (hl - h, step, hb - h)
					// according to https://hterrain-plugin.readthedocs.io/en/latest/ section "Procedural generation" it should be (h - hr, step, hf - h)
					Vector3 normal;
					Vector3 P((float)x, h, (float)z);
					if (x < _resolution - 1 && z < _resolution - 1)
					{
						float hr = gridVertices[z * _resolution + x + 1].altitude();
						float hf = gridVertices[(z + 1) * _resolution + x].altitude();
						normal = Vector3(h - hr, gridStepInWUs, h - hf).normalized();
						//{		// Verify
						//	Vector3 PR((float)(x + gridStepInWUs), hr, (float)z);
						//	Vector3 PF((float)x, hf, (float)(z + gridStepInWUs));
						//	Vector3 normal1 = (PF - P).cross(PR - P).normalized();
						//	if (!equal(normal1, normal))	// DEBUGRIC
						//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
						//}
					}
					else
					{
						if (x == _resolution - 1 && z == 0)
						{
							float hf = gridVertices[(z + 1) * _resolution + x].altitude();
							float hl = gridVertices[z * _resolution + x - 1].altitude();
							normal = Vector3(hl - h, gridStepInWUs, h - hf).normalized();
							//{		// Verify
							//	Vector3 PL((float)(x - gridStepInWUs), hl, (float)z);
							//	Vector3 PF((float)x, hf, (float)(z + gridStepInWUs));
							//	Vector3 normal1 = (PL - P).cross(PF - P).normalized();
							//	if (!equal(normal1, normal))	// DEBUGRIC
							//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
							//}
						}
						else if (x == 0 && z == _resolution - 1)
						{
							float hr = gridVertices[z * _resolution + x + 1].altitude();
							float hb = gridVertices[(z - 1) * _resolution + x].altitude();
							normal = Vector3(h - hr, gridStepInWUs, hb - h).normalized();
							//{		// Verify
							//	Vector3 PR((float)(x + gridStepInWUs), hr, (float)z);
							//	Vector3 PB((float)(x), hb, (float)(z - gridStepInWUs));
							//	Vector3 normal1 = (PR - P).cross(PB - P).normalized();
							//	if (!equal(normal1, normal))	// DEBUGRIC
							//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
							//}
						}
						else
						{
							float hl = gridVertices[z * _resolution + x - 1].altitude();
							float hb = gridVertices[(z - 1) * _resolution + x].altitude();
							normal = Vector3(hl - h, gridStepInWUs, hb - h).normalized();
							//{		// Verify
							//	Vector3 PB((float)x, hb, (float)(z - gridStepInWUs));
							//	Vector3 PL((float)(x - gridStepInWUs), hl, (float)z);
							//	Vector3 normal1 = (PB - P).cross(PL - P).normalized();
							//	if (!equal(normal1, normal))	// DEBUGRIC
							//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
							//}
						}
					}
					Color c = encodeNormal(normal);
					m_normalMapImage->set_pixel(x, z, c);
				}
			m_normalMapImage->unlock();
			m_heightMapImage->unlock();

			m_quadTree->getQuadrant()->getMeshCacheBuffer().writeImage(m_heightMapImage, TheWorld_Utils::MeshCacheBuffer::ImageType::heightmap);
			m_quadTree->getQuadrant()->getMeshCacheBuffer().writeImage(m_normalMapImage, TheWorld_Utils::MeshCacheBuffer::ImageType::normalmap);

			//{
			//	
			//	int64_t h1 = m_heightMapImage->get_height();
			//	int64_t w1 = m_heightMapImage->get_width();
			//	Image::Format f1 = m_heightMapImage->get_format();

			//	PoolByteArray datas1 = m_heightMapImage->get_data();
			//	File* file = File::_new();
			//	Error err = file->open("C:\\Users\\Riccardo\\appdata\\Roaming\\Godot\\app_userdata\\Godot Proj\\TheWorld\\Cache\\ST-5.000000_SZ-1025\\L-0\\prova.res", File::WRITE);
			//	int64_t i64 = datas1.size();
			//	file->store_64(i64);
			//	file->store_buffer(datas1);
			//	file->close();
			//	err = file->open("C:\\Users\\Riccardo\\appdata\\Roaming\\Godot\\app_userdata\\Godot Proj\\TheWorld\\Cache\\ST-5.000000_SZ-1025\\L-0\\prova.res", File::READ);
			//	i64 = file->get_64();
			//	PoolByteArray datas2 = file->get_buffer(i64);
			//	file->close();
			//	i64 = datas2.size();
			//	m_heightMapImage->create_from_data(_resolution, _resolution, false, Image::FORMAT_RH, datas2);

			//	int64_t h2 = m_heightMapImage->get_height();
			//	int64_t w2 = m_heightMapImage->get_width();
			//	Image::Format f2 = m_heightMapImage->get_format();
			//}
			
			
			//{
			//	int64_t h1 = m_heightMapImage->get_height();
			//	int64_t w1 = m_heightMapImage->get_width();
			//	Image::Format f1 = m_heightMapImage->get_format();
			//	m_heightMapImage->convert(godot::Image::FORMAT_RGB8);
			//	//m_heightMapImage = cache.readHeigthmap(loadingHeighmapOK);
			//	m_heightMapImage->convert(Image::FORMAT_RH);
			//	//m_normalMapImage = cache.readNormalmap(loadingNormalmapOK);
			//	//m_normalMapImage->convert(Image::FORMAT_RGB8);
			//	int64_t h = m_heightMapImage->get_height();
			//	int64_t w = m_heightMapImage->get_width();
			//	Image::Format f = m_heightMapImage->get_format();
			//	m_heightMapImage->lock();
			//	m_normalMapImage->lock();
			//	for (int z = 0; z < _resolution; z++)			// m_heightMapImage->get_height()
			//		for (int x = 0; x < _resolution; x++)		// m_heightMapImage->get_width()
			//		{
			//			Color c = m_heightMapImage->get_pixel(x, z);
			//			c = m_normalMapImage->get_pixel(x, z);
			//		}
			//	m_normalMapImage->unlock();
			//	m_heightMapImage->unlock();
			//}
		}
	}

	{
		Ref<ImageTexture> tex = ImageTexture::_new();
		tex->create_from_image(m_heightMapImage, Texture::FLAG_FILTER);
		m_heightMapTexture = tex;
		m_heightMapTexModified = true;
		//debugPrintTexture(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightMapTexture);	// DEBUGRIC
		}

	{
		Ref<ImageTexture> tex = ImageTexture::_new();
		tex->create_from_image(m_normalMapImage, Texture::FLAG_FILTER);
		m_normalMapTexture = tex;
		m_normalMapTexModified = true;
		//debugPrintTexture(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalMapTexture);	// DEBUGRIC
	}

	// Creating Splat Map Texture
	//{
	//	Ref<Image> image = Image::_new();
	//	image->create(_resolution, _resolution, false, Image::FORMAT_RGBA8);
	//	image->fill(Color(1, 0, 0, 0));
	//	m_splat1MapImage = image;
	//}

	// Filling Splat Map Texture
	//{
	//	Ref<ImageTexture> tex = ImageTexture::_new();
	//	tex->create_from_image(m_splat1MapImage, Texture::FLAG_FILTER);
	//	m_splat1MapTexture = tex;
	//	m_splat1MapImageModified = true;
	//}

	// Creating Color Map Texture
	//{
	//	Ref<Image> image = Image::_new();
	//	image->create(_resolution, _resolution, false, Image::FORMAT_RGBA8);
	//	image->fill(Color(1, 1, 1, 1));
	//	m_colorMapImage = image;
	//}

	// Filling Color Map Texture
	//{
	//	Ref<ImageTexture> tex = ImageTexture::_new();
	//	tex->create_from_image(m_colorMapImage, Texture::FLAG_FILTER);
	//	m_colorMapTexture = tex;
	//	m_colorMapImageModified = true;
	//}

	// _update_all_vertical_bounds ???	// TODORIC
	//	# RGF image where R is min heightand G is max height
	//	var _chunked_vertical_bounds : = Image.new()	// _chunked_vertical_bounds.create(csize_x, csize_y, false, Image.FORMAT_RGF)
	//	_update_vertical_bounds(0, 0, _resolution - 1, _resolution - 1)

	materialParamsNeedUpdate(true);
}

Color ShaderTerrainData::encodeNormal(Vector3 normal)
{
	normal = 0.5 * (normal + Vector3::ONE);
	return Color(normal.x, normal.z, normal.y);
}

void ShaderTerrainData::updateMaterialParams(void)
{
	// Completare da _update_material_params
	m_viewer->Globals()->debugPrint("Updating terrain material params");

	if (m_viewer->is_inside_tree())
	{
		Transform globalTransform = m_quadTree->getInternalGlobalTransform();
		//m_viewer->Globals()->debugPrint("getInternalGlobalTransform" + String(" t=") + String(globalTransform));	// DEBUGRIC

		Transform t = globalTransform.affine_inverse();
		//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_INVERSE_TRANSFORM) + String(" t=") + String(t));	// DEBUGRIC
		m_material->set_shader_param(SHADER_PARAM_INVERSE_TRANSFORM, t);

		Basis b = globalTransform.basis.inverse().transposed();
		//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_NORMAL_BASIS) + String(" b=") + String(b));	// DEBUGRIC
		m_material->set_shader_param(SHADER_PARAM_NORMAL_BASIS, b);

		float f = m_viewer->Globals()->gridStepInWU();
		//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GRID_STEP) + String(" grid_step=") + String(std::to_string(f).c_str()));	// DEBUGRIC
		m_material->set_shader_param(SHADER_PARAM_GRID_STEP, f);

		if (m_heightMapTexModified)
		{
			//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_HEIGHTMAP));	// DEBUGRIC
			m_material->set_shader_param(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightMapTexture);
			m_heightMapTexModified = false;
		}

		if (m_normalMapTexModified)
		{
			//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_NORMALMAP));	// DEBUGRIC
			m_material->set_shader_param(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalMapTexture);
			m_normalMapTexModified = false;
		}

		//Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node (grid) local coordinates of the camera pos
	}
}

void ShaderTerrainData::debugPrintTexture(std::string tex_name, Ref<Texture> tex)
{
	File* _file = File::_new();
	_file->open("res://" + String(tex_name.c_str()) + ".txt", File::WRITE);
	Ref<Image> _image = tex->get_data();
	int64_t width = _image->get_width();
	int64_t height = _image->get_height();
	Image::Format format = _image->get_format();
	std::string formatStr = std::to_string(format);
	if (format == Image::Format::FORMAT_RH)
		formatStr = "FORMAT_RH";
	else if (format == Image::Format::FORMAT_RGB8)
		formatStr = "FORMAT_RGB8";
	else if (format == Image::Format::FORMAT_RGBA8)
		formatStr = "FORMAT_RGBA8";
	std::string text = "Name=" + tex_name + " Format=" + formatStr + " W=" + std::to_string(width) + " H=" + std::to_string(height);
	_file->store_string(String(text.c_str()) + "\n");
	_image->lock();
	for (int h = 0; h < height; h++)
	{
		std::string row = "";
		for (int w = 0; w < width; w++)
		{
			row += std::to_string(w) + ":";
			if (format == Image::Format::FORMAT_RH)
				row += std::to_string(_image->get_pixel(w, h).r) + " ";
			else if (format == Image::Format::FORMAT_RGB8)
				row += std::to_string(_image->get_pixel(w, h).r) + "-" + std::to_string(_image->get_pixel(w, h).g) + "-" + std::to_string(_image->get_pixel(w, h).b) + " ";
			else if (format == Image::Format::FORMAT_RGBA8)
				row += std::to_string(_image->get_pixel(w, h).r) + "-" + std::to_string(_image->get_pixel(w, h).g) + "-" + std::to_string(_image->get_pixel(w, h).b) + "-" + std::to_string(_image->get_pixel(w, h).a) + " ";
			else
			{
				String s = String(_image->get_pixel(w, h));
				char* str = s.alloc_c_string();
				row += str;
				row += " ";
				godot::api->godot_free(str);
			}
		}
		text = "H=" + std::to_string(h) + " " + row;
		_file->store_string(String(text.c_str()) + "\n");
	}
	_image->unlock();
	_file->close();
}

QuadrantPos QuadrantPos::getQuadrantPos(enum class DirectionSlot dir, int numSlot)
{
	QuadrantPos q = *this;

	switch (dir)
	{
	case DirectionSlot::XMinus:
	{
		q.m_lowerXGridVertex -= (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::XPlus:
	{
		q.m_lowerXGridVertex += (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::ZMinus:
	{
		q.m_lowerZGridVertex -= (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::ZPlus:
	{
		q.m_lowerZGridVertex += (m_sizeInWU * numSlot);
	}
	break;
	}

	q.resetName();

	return q;
}

size_t QuadrantPos::distanceInPerimeter(QuadrantPos& q)
{
	size_t distanceOnX = (size_t)ceil(abs(q.getLowerXGridVertex() - getLowerXGridVertex()) / q.m_sizeInWU);
	size_t distanceOnZ = (size_t)ceil(abs(q.getLowerZGridVertex() - getLowerZGridVertex()) / q.m_sizeInWU);
	if (distanceOnX > distanceOnZ)
		return distanceOnX;
	else
		return distanceOnZ;
}

TheWorld_Utils::MeshCacheBuffer& Quadrant::getMeshCacheBuffer(void)
{
	return m_cache;
}

void Quadrant::populateGridVertices(float viewerPosX, float viewerPosZ, bool setCamera, float cameraDistanceFromTerrain)
{
	BYTE shortBuffer[256 + 1];
	//TheWorld_Utils::TimerMs clock;

	//{
	//	GridVertex v(0.12F, 0.1212F, 0.1313F, 1);
	//	v.serialize(shortBuffer, size);
	//	GridVertex v1(shortBuffer, size);
	//	assert(v.posX() == v1.posX());
	//	assert(v.altitude() == v1.altitude());
	//	assert(v.posZ() == v1.posZ());
	//	assert(v.lvl() == v1.lvl());
	//}

	size_t serializedVertexSize = 0;
	TheWorld_Utils::GridVertex v;
	// Serialize an empty GridVertex only to obtain the size of a serialized GridVertex
	v.serialize(shortBuffer, serializedVertexSize);

	float lowerXGridVertex = m_quadrantPos.getLowerXGridVertex();
	float lowerZGridVertex = m_quadrantPos.getLowerZGridVertex();
	float gridStepinWU = m_quadrantPos.getGridStepInWU();
	int numVerticesPerSize = m_quadrantPos.getNumVerticesPerSize();
	int lvl = m_quadrantPos.getLevel();
	std::string buffGridVertices;

	// look for cache in file system
	TheWorld_Utils::MeshCacheBuffer cache = getMeshCacheBuffer();
	std::string meshId = cache.getMeshIdFromMeshCache();

	if (meshId.length() > 0)
	{
		//clock.tick();
		m_quadTree->Viewer()->Globals()->Client()->MapManagerGetVertices(viewerPosX, viewerPosZ,
																		 lowerXGridVertex, lowerZGridVertex,
																		 1, // TheWorld_MapManager::MapManager::anchorType::upperleftcorner
																		 numVerticesPerSize, gridStepinWU, lvl, setCamera, cameraDistanceFromTerrain, meshId);
		//clock.tock();
		//m_quadTree->Viewer()->Globals()->debugPrint(String("ELAPSED - QUADRANT ") + getId().getId().c_str() + " TAG=" + getId().getTag().c_str() + " - client MapManagerGetVertices (quadrant found in cache) " + std::to_string(clock.duration().count()).c_str() + " ms");
	}
	else
	{
		//clock.tick();
		m_quadTree->Viewer()->Globals()->Client()->MapManagerGetVertices(viewerPosX, viewerPosZ,
																		 lowerXGridVertex, lowerZGridVertex,
																		 1, // TheWorld_MapManager::MapManager::anchorType::upperleftcorner
																		 numVerticesPerSize, gridStepinWU, lvl, setCamera, cameraDistanceFromTerrain, meshId);
		//clock.tock();
		//m_quadTree->Viewer()->Globals()->debugPrint(String("ELAPSED - QUADRANT ") + getId().getId().c_str() + " TAG=" + getId().getTag().c_str() + " - client MapManagerGetVertices " + std::to_string(clock.duration().count()).c_str() + " ms");
	}

	return;
}

void Quadrant::refreshGridVertices(std::string buffer, std::string meshId, std::string& meshIdFromBuffer)
{
	float minY = 0, maxY = 0;
	m_cache.refreshVerticesFromBuffer(buffer, meshIdFromBuffer, m_vectGridVertices, (void*)&m_heigths, minY, maxY);

	if (meshIdFromBuffer != meshId)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("MeshId from buffer not equal to meshIUd from server").c_str()));

	if (m_vectGridVertices.size() == 0)
		m_cache.readVerticesFromMeshCache(meshId, m_vectGridVertices, (void*)&m_heigths, minY, maxY);

	int areaSize = (m_viewer->Globals()->heightmapResolution() + 1) * (m_viewer->Globals()->heightmapResolution() + 1);

	if (m_vectGridVertices.size() != areaSize)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Grid Vertices not of the correct size").c_str()));

	Vector3 startPosition(m_quadrantPos.getLowerXGridVertex(), minY, m_quadrantPos.getLowerZGridVertex());
	Vector3 endPosition(startPosition.x + m_quadrantPos.getSizeInWU(), maxY, startPosition.z + m_quadrantPos.getSizeInWU());
	Vector3 size = endPosition - startPosition;

	m_globalCoordAABB.set_position(startPosition);
	m_globalCoordAABB.set_size(size);
	
	m_meshId = meshId;

	//m_collider->setData();
}
