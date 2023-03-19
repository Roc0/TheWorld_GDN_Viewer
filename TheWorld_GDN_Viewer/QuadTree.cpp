//#include "pch.h"
#include "QuadTree.h"
#include "Viewer_Utils.h"

#include <File.hpp>
#include <OS.hpp>

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Quadrant.h"
#include "Profiler.h"
#include "half.h"

#include "assert.h"
#include <filesystem>

using namespace godot;
namespace fs = std::filesystem;

//extern int g_num = 0;

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

	Ref<Material> mat = m_quadTree->getCurrentMaterial();

	if (m_chunk == nullptr)
	{
		// create chunk for current quad because this is the first time that is needed for current lod and pos
		if (_DEBUG_AAB)
			m_chunk = new ChunkDebug(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);
		else
			m_chunk = new Chunk(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);

		// Set Viewer Pos in Global Coord
		//m_chunk->setParentGlobalTransform(m_viewer->internalTransformGlobalCoord());
		m_quadTree->addChunk(m_chunk);
	}
	else
		m_chunk->setMaterial(mat);

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
	m_editModeSel = false;
	m_lookDev = ShaderTerrainData::LookDev::NotSet;
}

void QuadTree::refreshTerrainData(float viewerPosX, float viewerPosZ, bool setCamera, float cameraDistanceFromTerrain)
{
	my_assert(statusInitialized() || statusRefreshTerrainDataNeeded());
	if (!(statusInitialized() || statusRefreshTerrainDataNeeded()))
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not comply: " + std::to_string((int)status())).c_str()));

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrant);

	setStatus(QuadrantStatus::refreshTerrainDataInProgress);

	m_worldQuadrant->populateGridVertices(viewerPosX, viewerPosZ, setCamera, cameraDistanceFromTerrain);
}

void QuadTree::init(float viewerPosX, float viewerPosZ, bool setCamera, float cameraDistanceFromTerrain)
{
	my_assert(status() == QuadrantStatus::uninitialized);
	if (status() != QuadrantStatus::uninitialized)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not comply: " + std::to_string((int)status())).c_str()));

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrant);

	m_viewer->getMainProcessingMutex().lock();
	m_GDN_Quadrant = GDN_TheWorld_Quadrant::_new();
	m_viewer->getMainProcessingMutex().unlock();
	m_GDN_Quadrant->init(this);
	std::string quadrantNodeName = m_worldQuadrant->getPos().getName();
	m_GDN_Quadrant->set_name(quadrantNodeName.c_str());
	m_viewer->add_child(m_GDN_Quadrant);
	onGlobalTransformChanged();

	setStatus(QuadrantStatus::getTerrainDataInProgress);

	m_worldQuadrant->populateGridVertices(viewerPosX, viewerPosZ, setCamera, cameraDistanceFromTerrain);

	getQuadrant()->getShaderTerrainData()->init();

	m_root = make_unique<Quad>(0, 0, m_viewer->Globals()->lodMaxDepth(), PosInQuad::Root, m_viewer, this);
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
	//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree Desc 1 ") + __FUNCTION__, "ALL");

	//godot::GDN_TheWorld_Globals::s_num++;
	//TheWorld_Viewer_Utils::TimerMs clock;
	//clock.tick();

	if (m_GDN_Quadrant)
	{
		m_GDN_Quadrant->deinit();
		Node* parent = m_GDN_Quadrant->get_parent();
		if (parent)
			parent->remove_child(m_GDN_Quadrant);
		m_GDN_Quadrant->queue_free();
		m_GDN_Quadrant = nullptr;
	}

	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed1 += clock.duration().count();

	//clock.tick();
	m_root.reset();
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed2 += clock.duration().count();

	//clock.tick();
	clearChunkUpdate();
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed3 += clock.duration().count();

	// Delete all Chunks
	//clock.tick();
	for (Chunk::MapChunk::iterator it = m_mapChunk.begin(); it != m_mapChunk.end(); it++)
	{
		for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
		{
			delete it1->second;
		}
		it->second.clear();
	}
	m_mapChunk.clear();
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed4 += clock.duration().count();

	//clock.tick();
	if (m_worldQuadrant)
		delete m_worldQuadrant;
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed5 += clock.duration().count();
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
		AABB chunkAABB = quad->getChunkAABB();
		Vector3 quadCenterGlobal(real_t((quad->slotPosX() * chunkSizeInWUs) + m_worldQuadrant->getPos().getLowerXGridVertex() + chunkSizeInWUs / 2),
			(chunkAABB.position + chunkAABB.size / 2).y,
			real_t((quad->slotPosZ() * chunkSizeInWUs) + m_worldQuadrant->getPos().getLowerZGridVertex() + chunkSizeInWUs / 2));

		Transform gt = m_viewer->get_global_transform();
		Transform t = gt * Transform(Basis(), quadCenterGlobal);
		quadCenterGlobal = t.origin;

		real_t splitDistance = chunkSizeInWUs * globals->splitScale();
		//real_t splitDistance = m_worldQuadrant->getPos().getSizeInWU() * globals->splitScale();
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
			
			//std::string id = chunk->getIdStr();
			//if ((chunk->getLod() == 0 && chunk->getSlotPosX() == 31 && chunk->getSlotPosZ() == 31) || id == "LOD:0-X:31-Z:31")
			//{
			//	PLOG_DEBUG << "eccolo";
			//}

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
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (First) - " << " XMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZMinusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (First) - " << " ZMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
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
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Second) - " << " XPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZMinusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Second) - " << " ZMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
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
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Third) - " << " XMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZPlusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Third) - " << " ZPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
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
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Forth) - " << " XPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZPlusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr)
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Forth) - " << " ZPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
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
			bool trackChunk = false;
			if (m_viewer->trackMouse() && getQuadrant()->getPos().getName() == m_viewer->getMouseQuadrantHitName())
				trackChunk = true;

			if (quad->splitRequired() && quad->Lod() > 0)
			{
				// Split
				quad->split();
				for (int i = 0; i < 4; i++)
				{
					quad->getChild(i)->setCameraPos(cameraPosGlobalCoord);
					if (trackChunk)
						quad->getChild(i)->getChunk()->checkMouseHit();
					quad->getChild(i)->getChunk()->setDistanceFromCamera(0);
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

bool  QuadTree::updateMaterialParams(void)
{
	bool updated = false;

	if (!isValid())
		return updated;
	
	if (getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4 ") + __FUNCTION__, "QuadTree updateMaterialParams");

		if (isVisible())
		{
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4.1 ") + __FUNCTION__, "QuadTree collider stuff");

				m_worldQuadrant->getCollider()->deinit();
				m_worldQuadrant->getCollider()->init(m_GDN_Quadrant, 1, 1);
				m_worldQuadrant->getCollider()->enterWorld();
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4.1.1 ") + __FUNCTION__, "QuadTree set data for collider");
					m_worldQuadrant->getCollider()->setData();
				}
			}

			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4.2 ") + __FUNCTION__, "QuadTree update shader");

				getQuadrant()->getShaderTerrainData()->updateMaterialParams(m_lookDev);
			}

			updated = true;
		}

		getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate(false);

		//if (!getQuadrant()->needUploadToServer())
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4.2 ") + __FUNCTION__, "QuadTree releaseMemoryForTerrainValues");
			getQuadrant()->releaseTerrainValuesMemory(isVisible());
		}
		
	}

	return updated;
}

void QuadTree::setLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev)
{
	assert(lookDev != ShaderTerrainData::LookDev::NotSet);
	if (lookDev == ShaderTerrainData::LookDev::NotSet)
		return;

	if (m_lookDev == lookDev)
		return;

	getQuadrant()->setHeightsUpdated(true);
	getQuadrant()->setNormalsUpdated(true);

	materialParamsNeedReset(true);
	
	Chunk::SetMaterialChunkAction action(getQuadrant()->getShaderTerrainData()->getLookDevMaterial());
	ForAllChunk(action);

	m_lookDev = lookDev;
}

void QuadTree::setRegularMaterial(void)
{
	materialParamsNeedReset(true);

	Chunk::SetMaterialChunkAction action(getQuadrant()->getShaderTerrainData()->getRegularMaterial());
	ForAllChunk(action);

	m_lookDev = ShaderTerrainData::LookDev::NotSet;
}

Ref<Material> QuadTree::getCurrentMaterial(void)
{
	if (m_lookDev == ShaderTerrainData::LookDev::NotSet)
		return getQuadrant()->getShaderTerrainData()->getRegularMaterial();
	else
		return getQuadrant()->getShaderTerrainData()->getLookDevMaterial();
}

bool QuadTree::resetMaterialParams(bool force)
{
	bool reset = false;

	if (force)
	{
		getQuadrant()->getShaderTerrainData()->resetMaterialParams(m_lookDev);
		materialParamsNeedReset(false);
		reset = true;
	}
	else if (materialParamsNeedReset() && !getQuadrant()->internalDataLocked())
	{
		getQuadrant()->getShaderTerrainData()->resetMaterialParams(m_lookDev);
		materialParamsNeedReset(false);
		reset = true;
	}

	return reset;
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
	m_colorMapTexModified = false;
}

ShaderTerrainData::~ShaderTerrainData()
{
	m_regularMaterial.unref();
	m_lookDevMaterial.unref();
	//m_heightMapImage.unref();
	m_heightMapTexture.unref();
	//m_normalMapImage.unref();
	m_normalMapTexture.unref();
	//m_colorMapImage.unref();
	m_colorMapTexture.unref();
}

void ShaderTerrainData::init(void)
{
	m_viewer->getMainProcessingMutex().lock();
	Ref<ShaderMaterial> mat = ShaderMaterial::_new();
	m_viewer->getMainProcessingMutex().unlock();
	ResourceLoader* resLoader = ResourceLoader::get_singleton();
	String shaderPath = "res://shaders/regular.shader";
	Ref<Shader> shader = resLoader->load(shaderPath);
	mat->set_shader(shader);
	m_regularMaterial = mat;
}

Ref<Material> ShaderTerrainData::getLookDevMaterial(void)
{
	if (m_lookDevMaterial == nullptr)
	{
		m_viewer->getMainProcessingMutex().lock();
		Ref<ShaderMaterial> mat = ShaderMaterial::_new();
		m_viewer->getMainProcessingMutex().unlock();
		ResourceLoader* resLoader = ResourceLoader::get_singleton();
		String shaderPath = "res://shaders/lookdev.shader";
		Ref<Shader> shader = resLoader->load(shaderPath);
		mat->set_shader(shader);
		m_lookDevMaterial = mat;
	}

	return m_lookDevMaterial;
}

// it is expected that globals and World Datas are loaded
// TODORIC: maybe usefull for performance reasons specify which texture need update and which rect of the texture 
void ShaderTerrainData::resetMaterialParams(LookDev lookdev)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3 ") + __FUNCTION__, "itQuadTree->second->resetMaterialParams");

	std::recursive_mutex& quadrantMutex = m_quadTree->getQuadrantMutex();
	std::lock_guard<std::recursive_mutex> lock(quadrantMutex);
	
	int _resolution = m_viewer->Globals()->heightmapResolution() + 1;

	// if heights updated create texture for shader (m_heightMapTexture), it will be freed we passed to the shader
	if (m_quadTree->getQuadrant()->heightsUpdated())
	{
		if (m_quadTree->getQuadrant()->getFloat16HeightsBuffer().size() > 0)
		{
			m_viewer->getMainProcessingMutex().lock();
			godot::Ref<godot::Image> image = godot::Image::_new();
			m_viewer->getMainProcessingMutex().unlock();

			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.1 ") + __FUNCTION__, "Create Image from heights buffer");

				size_t uint16_t_size = sizeof(uint16_t);	// the size of an half ==> float_16
				godot::PoolByteArray data;
				size_t heightmapBufferSizeInBytes = _resolution * _resolution * uint16_t_size;
				data.resize((int)heightmapBufferSizeInBytes);
				{
					godot::PoolByteArray::Write w = data.write();
					size_t heightsBufferSize = m_quadTree->getQuadrant()->getFloat16HeightsBuffer().size();
					assert(heightmapBufferSizeInBytes == heightsBufferSize);
					memcpy((char*)w.ptr(), m_quadTree->getQuadrant()->getFloat16HeightsBuffer().ptr(), heightsBufferSize);
				}
				image->create_from_data(_resolution, _resolution, false, Image::FORMAT_RH, data);
				//m_heightMapImage = image;
			}

			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.2 ") + __FUNCTION__, "Create Texture from heights image");

				m_viewer->getMainProcessingMutex().lock();
				Ref<ImageTexture> tex = ImageTexture::_new();
				m_viewer->getMainProcessingMutex().unlock();
				//tex->create_from_image(m_heightMapImage, Texture::FLAG_FILTER);
				tex->create_from_image(image, Texture::FLAG_FILTER);
				m_heightMapTexture = tex;
				m_heightMapTexModified = true;
				//debugPrintTexture(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightMapTexture);	// DEBUGRIC
			}

			image.unref();
		}

		m_quadTree->getQuadrant()->setHeightsUpdated(false);
	}

	// if normals updated create texture for shader (m_normalMapTexture), it will be freed we passed to the shader. If normals have not been computed we fill the texturo with 0,0,0,0
	if (m_quadTree->getQuadrant()->normalsUpdated())
	{
		m_viewer->getMainProcessingMutex().lock();
		godot::Ref<godot::Image> image = godot::Image::_new();
		m_viewer->getMainProcessingMutex().unlock();

		TheWorld_Utils::MemoryBuffer& normalsBuffer = m_quadTree->getQuadrant()->getNormalsBuffer();
		size_t normalsBufferSize = normalsBuffer.size();

		if (normalsBufferSize == 0)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.3 ") + __FUNCTION__, "Create empty Image for normals");
			image->create(_resolution, _resolution, false, Image::FORMAT_RGB8);
			image->fill(godot::Color(0, 0, 0, 1.0));
		}
		else
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.3 ") + __FUNCTION__, "Create Image from normal buffer");

			godot::PoolByteArray data;
			size_t normalmapBufferSizeInBytes = _resolution * _resolution * sizeof(struct TheWorld_Utils::_RGB);
			data.resize((int)normalmapBufferSizeInBytes);
			assert(normalmapBufferSizeInBytes == normalsBufferSize);
			{
				godot::PoolByteArray::Write w = data.write();
				memcpy((char*)w.ptr(), normalsBuffer.ptr(), normalsBufferSize);
			}
			image->create_from_data(_resolution, _resolution, false, Image::FORMAT_RGB8, data);
		}

		{
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.4 ") + __FUNCTION__, "Create Texture from normal image");

			m_viewer->getMainProcessingMutex().lock();
			Ref<ImageTexture> tex = ImageTexture::_new();
			m_viewer->getMainProcessingMutex().unlock();
			//tex->create_from_image(m_normalMapImage, Texture::FLAG_FILTER);
			tex->create_from_image(image, Texture::FLAG_FILTER);
			m_normalMapTexture = tex;
			m_normalMapTexModified = true;
			//debugPrintTexture(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalMapTexture);	// DEBUGRIC
		}

		image.unref();

		m_quadTree->getQuadrant()->setNormalsUpdated(false);
	}

	// TODORIC: the shader must modify visualization for selected quad
	if (m_quadTree->getQuadrant()->colorsUpdated())
	{
		m_viewer->getMainProcessingMutex().lock();
		Ref<Image> image = Image::_new();
		m_viewer->getMainProcessingMutex().unlock();

		{
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.5 ") + __FUNCTION__, "Create color image");

			// Creating Color Map Texture
			image->create(_resolution, _resolution, false, Image::FORMAT_RGBA8);
			//image->fill(Color(1, 1, 1, 1));
			// Filling Color Map Texture
			if (m_quadTree->editModeSel())
				image->fill(godot::GDN_TheWorld_Globals::g_color_yellow_amber);
			else
				image->fill(godot::GDN_TheWorld_Globals::g_color_white);
			//m_colorMapImage = image;
		}

		{
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.6 ") + __FUNCTION__, "Create Texture from color image");

			m_viewer->getMainProcessingMutex().lock();
			Ref<ImageTexture> tex = ImageTexture::_new();
			m_viewer->getMainProcessingMutex().unlock();
			//tex->create_from_image(m_colorMapImage, Texture::FLAG_FILTER);
			tex->create_from_image(image, Texture::FLAG_FILTER);
			m_colorMapTexture = tex;
			m_colorMapTexModified = true;
		}

		m_quadTree->getQuadrant()->setColorsUpdated(false);

		image.unref();
	}

	// Creating Splat Map Texture
	//{
	//	m_viewer->getMainProcessingMutex().lock();
	//	Ref<Image> image = Image::_new();
	//	m_viewer->getMainProcessingMutex().unlock();
	//	image->create(_resolution, _resolution, false, Image::FORMAT_RGBA8);
	//	image->fill(Color(1, 0, 0, 0));
	//	m_splat1MapImage = image;
	//}

	// Filling Splat Map Texture
	//{
	//	m_viewer->getMainProcessingMutex().lock();
	//	Ref<ImageTexture> tex = ImageTexture::_new();
	//	m_viewer->getMainProcessingMutex().unlock();
	//	tex->create_from_image(m_splat1MapImage, Texture::FLAG_FILTER);
	//	m_splat1MapTexture = tex;
	//	m_splat1MapImageModified = true;
	//}

	// _update_all_vertical_bounds ???	// TODORIC
	//	# RGF image where R is min heightand G is max height
	//	var _chunked_vertical_bounds : = Image.new()	// _chunked_vertical_bounds.create(csize_x, csize_y, false, Image.FORMAT_RGF)
	//	_update_vertical_bounds(0, 0, _resolution - 1, _resolution - 1)

	materialParamsNeedUpdate(true);
}

//Color ShaderTerrainData::encodeNormal(Vector3 normal)
//{
//	normal = 0.5 * (normal + Vector3::ONE);
//	return Color(normal.x, normal.z, normal.y);
//}

void ShaderTerrainData::updateMaterialParams(LookDev lookdev)
{
	// Completare da _update_material_params
	//m_viewer->Globals()->debugPrint("Updating terrain material params");

	Ref<ShaderMaterial> currentMaterial;
	if (lookdev == LookDev::NotSet)
		currentMaterial = m_regularMaterial;
	else
		currentMaterial = m_lookDevMaterial;

	if (m_viewer->is_inside_tree())
	{
		Transform globalTransform = m_quadTree->getInternalGlobalTransform();
		//m_viewer->Globals()->debugPrint("getInternalGlobalTransform" + String(" t=") + String(globalTransform));	// DEBUGRIC

		Transform t = globalTransform.affine_inverse();
		//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_INVERSE_TRANSFORM) + String(" t=") + String(t));	// DEBUGRIC
		currentMaterial->set_shader_param(SHADER_PARAM_INVERSE_TRANSFORM, t);

		// This is needed to properly transform normals if the terrain is scaled
		Basis b = globalTransform.basis.inverse().transposed();
		//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_NORMAL_BASIS) + String(" b=") + String(b));	// DEBUGRIC
		currentMaterial->set_shader_param(SHADER_PARAM_NORMAL_BASIS, b);

		float f = m_viewer->Globals()->gridStepInWU();
		//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GRID_STEP) + String(" grid_step=") + String(std::to_string(f).c_str()));	// DEBUGRIC
		currentMaterial->set_shader_param(SHADER_PARAM_GRID_STEP, f);

		bool selected = m_quadTree->editModeSel();
		float sel = 0.0f;
		if (selected)
			sel = 1.0f;
		//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_EDITMODE_SELECTED) + String(" quad_selected=") + String(std::to_string(sel).c_str()));	// DEBUGRIC
		currentMaterial->set_shader_param(SHADER_PARAM_EDITMODE_SELECTED, sel);

		if (m_heightMapTexModified && m_heightMapTexture != nullptr)
		{
			//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_HEIGHTMAP));	// DEBUGRIC
			currentMaterial->set_shader_param(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightMapTexture);
			m_heightMapTexModified = false;
		}

		if (m_normalMapTexModified && m_normalMapTexture != nullptr)
		{
			//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_NORMALMAP));	// DEBUGRIC
			currentMaterial->set_shader_param(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalMapTexture);
			m_normalMapTexModified = false;
		}

		if (m_colorMapTexModified && m_colorMapTexture != nullptr)
		{
			//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_COLORMAP));	// DEBUGRIC
			currentMaterial->set_shader_param(SHADER_PARAM_TERRAIN_COLORMAP, m_colorMapTexture);
			m_colorMapTexModified = false;
		}

		if (lookdev == LookDev::Heights && m_heightMapTexture != nullptr)
		{
			currentMaterial->set_shader_param(SHADER_PARAM_LOOKDEV_MAP, m_heightMapTexture);
		}

		if (lookdev == LookDev::Normals && m_normalMapTexture != nullptr)
		{
			currentMaterial->set_shader_param(SHADER_PARAM_LOOKDEV_MAP, m_normalMapTexture);
		}

		m_heightMapTexture.unref();
		m_normalMapTexture.unref();
		m_colorMapTexture.unref();

		//Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node (grid) local coordinates of the camera pos
	}
}

//void ShaderTerrainData::debugPrintTexture(std::string tex_name, Ref<Texture> tex)
//{
//	File* _file = File::_new();
//	_file->open("res://" + String(tex_name.c_str()) + ".txt", File::WRITE);
//	Ref<Image> _image = tex->get_data();
//	int64_t width = _image->get_width();
//	int64_t height = _image->get_height();
//	Image::Format format = _image->get_format();
//	std::string formatStr = std::to_string(format);
//	if (format == Image::Format::FORMAT_RH)
//		formatStr = "FORMAT_RH";
//	else if (format == Image::Format::FORMAT_RGB8)
//		formatStr = "FORMAT_RGB8";
//	else if (format == Image::Format::FORMAT_RGBA8)
//		formatStr = "FORMAT_RGBA8";
//	std::string text = "Name=" + tex_name + " Format=" + formatStr + " W=" + std::to_string(width) + " H=" + std::to_string(height);
//	_file->store_string(String(text.c_str()) + "\n");
//	_image->lock();
//	for (int h = 0; h < height; h++)
//	{
//		std::string row = "";
//		for (int w = 0; w < width; w++)
//		{
//			row += std::to_string(w) + ":";
//			if (format == Image::Format::FORMAT_RH)
//				row += std::to_string(_image->get_pixel(w, h).r) + " ";
//			else if (format == Image::Format::FORMAT_RGB8)
//				row += std::to_string(_image->get_pixel(w, h).r) + "-" + std::to_string(_image->get_pixel(w, h).g) + "-" + std::to_string(_image->get_pixel(w, h).b) + " ";
//			else if (format == Image::Format::FORMAT_RGBA8)
//				row += std::to_string(_image->get_pixel(w, h).r) + "-" + std::to_string(_image->get_pixel(w, h).g) + "-" + std::to_string(_image->get_pixel(w, h).b) + "-" + std::to_string(_image->get_pixel(w, h).a) + " ";
//			else
//			{
//				String s = String(_image->get_pixel(w, h));
//				char* str = s.alloc_c_string();
//				row += str;
//				row += " ";
//				godot::api->godot_free(str);
//			}
//		}
//		text = "H=" + std::to_string(h) + " " + row;
//		_file->store_string(String(text.c_str()) + "\n");
//	}
//	_image->unlock();
//	_file->close();
//}

QuadrantPos QuadrantPos::getQuadrantPos(enum class DirectionSlot dir, int numSlot)
{
	QuadrantPos pos = *this;

	switch (dir)
	{
	case DirectionSlot::XMinus:
	{
		pos.m_lowerXGridVertex -= (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::XPlus:
	{
		pos.m_lowerXGridVertex += (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::ZMinus:
	{
		pos.m_lowerZGridVertex -= (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::ZPlus:
	{
		pos.m_lowerZGridVertex += (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::ZMinusXMinus:
	{
		QuadrantPos pos1 = getQuadrantPos(DirectionSlot::ZMinus, numSlot);
		pos = pos1.getQuadrantPos(DirectionSlot::XMinus, numSlot);
	}
	break;
	case DirectionSlot::ZMinusXPlus:
	{
		QuadrantPos pos1 = getQuadrantPos(DirectionSlot::ZMinus, numSlot);
		pos = pos1.getQuadrantPos(DirectionSlot::XPlus, numSlot);
	}
	break;
	case DirectionSlot::ZPlusXMinus:
	{
		QuadrantPos pos1 = getQuadrantPos(DirectionSlot::ZPlus, numSlot);
		pos = pos1.getQuadrantPos(DirectionSlot::XMinus, numSlot);
	}
	break;
	case DirectionSlot::ZPlusXPlus:
	{
		QuadrantPos pos1 = getQuadrantPos(DirectionSlot::ZPlus, numSlot);
		pos = pos1.getQuadrantPos(DirectionSlot::XPlus, numSlot);
	}
	break;
	}

	pos.resetName();

	return pos;
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

size_t Quadrant::getIndexFromHeighmap(float posX, float posZ, size_t level)
{
	if (posX < m_quadrantPos.getLowerXGridVertex() || posX > m_quadrantPos.getLowerXGridVertex() + m_quadrantPos.getSizeInWU()
		|| posZ < m_quadrantPos.getLowerZGridVertex() || posZ > m_quadrantPos.getLowerZGridVertex() + m_quadrantPos.getSizeInWU())
		return -1;

	size_t numStepsXAxis = size_t((posX - m_quadrantPos.getLowerXGridVertex()) / m_quadrantPos.getGridStepInWU());
	size_t numStepsZAxis = size_t((posZ - m_quadrantPos.getLowerZGridVertex()) / m_quadrantPos.getGridStepInWU());

	return numStepsZAxis * m_quadrantPos.getNumVerticesPerSize() + numStepsXAxis;
}

float Quadrant::getAltitudeFromHeigthmap(size_t index)
{
	//float* p = (float*)getFloat32HeightsBuffer().ptr();
	//return *(p + index);
	
	uint16_t* p = (uint16_t*)getFloat16HeightsBuffer().ptr();
	TheWorld_Utils::FLOAT_32 f;
	f.u32 = half_to_float(*(p + index));
	return f.f32;
}

float Quadrant::getPosXFromHeigthmap(size_t index)
{
	size_t numStepsXAxis = index - (index / m_quadrantPos.getNumVerticesPerSize()) * m_quadrantPos.getNumVerticesPerSize();
	return m_quadrantPos.getLowerXGridVertex() + numStepsXAxis * m_quadrantPos.getGridStepInWU();
}

float Quadrant::getPosZFromHeigthmap(size_t index)
{
	size_t numStepsZAxis = size_t(index / m_quadrantPos.getNumVerticesPerSize());
	return m_quadrantPos.getLowerZGridVertex() + numStepsZAxis * m_quadrantPos.getGridStepInWU();
}

void Quadrant::populateGridVertices(float viewerPosX, float viewerPosZ, bool setCamera, float cameraDistanceFromTerrain)
{
	//BYTE shortBuffer[256 + 1];
	//TheWorld_Viewer_Utils::TimerMs clock;

	//{
	//	GridVertex v(0.12F, 0.1212F, 0.1313F, 1);
	//	v.serialize(shortBuffer, size);
	//	GridVertex v1(shortBuffer, size);
	//	assert(v.posX() == v1.posX());
	//	assert(v.altitude() == v1.altitude());
	//	assert(v.posZ() == v1.posZ());
	//	assert(v.lvl() == v1.lvl());
	//}

	//size_t serializedVertexSize = 0;
	//TheWorld_Utils::GridVertex v;
	// Serialize an empty GridVertex only to obtain the size of a serialized GridVertex
	//v.serialize(shortBuffer, serializedVertexSize);

	float lowerXGridVertex = m_quadrantPos.getLowerXGridVertex();
	float lowerZGridVertex = m_quadrantPos.getLowerZGridVertex();
	float gridStepinWU = m_quadrantPos.getGridStepInWU();
	int numVerticesPerSize = m_quadrantPos.getNumVerticesPerSize();
	int lvl = m_quadrantPos.getLevel();
	std::string buffGridVertices;

	// look for cache in file system
	TheWorld_Utils::MeshCacheBuffer cache = getMeshCacheBuffer();
	std::string meshId = cache.getMeshIdFromCache();

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

void Quadrant::refreshGridVerticesFromServer(std::string& buffer, std::string meshId, std::string& meshIdFromBuffer, bool updateCache)
{
	float minAltitude = 0, maxAltitude = 0;
	
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.1 ") + __FUNCTION__, "m_cache.refreshMapsFromBuffer");

		TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
		m_cache.refreshMapsFromBuffer(buffer, meshIdFromBuffer, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, updateCache);
		if (terrainEditValuesBuffer.size() > 0)
			getTerrainEdit()->deserialize(terrainEditValuesBuffer);
	}

	if (meshIdFromBuffer.size() > 0)
	{
		assert(meshIdFromBuffer == meshId);
		if (meshIdFromBuffer != meshId)
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("MeshId from buffer not equal to meshId from server").c_str()));
	}

	setEmpty(false);

	TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
	if (meshIdFromBuffer.size() == 0 || m_float16HeigthsBuffer.empty())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.2 ") + __FUNCTION__, "m_cache.readMapsFromMeshCache");

		bool empty = m_cache.refreshMapsFromCache(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer);
		if (terrainEditValuesBuffer.size() > 0)
			getTerrainEdit()->deserialize(terrainEditValuesBuffer);

		if (empty)
			setEmpty(true);
	}

	//if (m_float16HeigthsBuffer.empty())
	//{
	//	TheWorld_Utils::MemoryBuffer tempBuffer;

	//	{
	//		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.3 ") + __FUNCTION__, "m_cache.setEmptyBuffer");
	//		m_cache.setEmptyBuffer(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), meshIdFromBuffer, tempBuffer);
	//	}

	//	{
	//		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.2 ") + __FUNCTION__, "m_cache.readMapsFromMeshCache");

	//		m_cache.refreshMapsFromBuffer(tempBuffer, meshIdFromBuffer, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, false);
	//		if (terrainEditValuesBuffer.size() > 0)
	//			getTerrainEdit()->deserialize(terrainEditValuesBuffer);
	//	}

	//	setEmpty(true);
	//}
	//else
	//	setEmpty(false);

	assert(!m_float16HeigthsBuffer.empty());

	if (getTerrainEdit()->needUploadToServer)
		setNeedUploadToServer(true);
	else
		setNeedUploadToServer(false);

	setHeightsUpdated(true);
	setColorsUpdated(true);

	if (m_normalsBuffer.size() > 0)
		setNormalsUpdated(true);

	int areaSize = (m_viewer->Globals()->heightmapResolution() + 1) * (m_viewer->Globals()->heightmapResolution() + 1);

	//BYTE shortBuffer[256 + 1];
	size_t uint16_t_size = sizeof(uint16_t);	// the size of an half ==> float_16
	//TheWorld_Utils::serializeToByteStream<uint16_t>(0, shortBuffer, uint16_t_size);
	size_t float_size = sizeof(float);	// the size of a float
	//TheWorld_Utils::serializeToByteStream<float>(0, shortBuffer, float_size);

	size_t numFloat16Heights = m_float16HeigthsBuffer.size() / uint16_t_size;
	size_t numFloat32Heights = m_float32HeigthsBuffer.size() / float_size;
	size_t numNormals = m_normalsBuffer.size() / sizeof(struct TheWorld_Utils::_RGB);

	assert(numFloat16Heights == numFloat32Heights);
	if (numNormals > 0)
		assert(numFloat16Heights == numNormals);
	assert(numFloat16Heights == areaSize);
	if (numFloat16Heights != numFloat32Heights || (numNormals > 0 && numFloat16Heights != numNormals) || numFloat16Heights != areaSize)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Grid Vertices not of the correct size").c_str()));

	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("SetColliderHeights ") + __FUNCTION__, "ALL");
		m_heightsForCollider.resize((int)numFloat32Heights);
		godot::PoolRealArray::Write w = m_heightsForCollider.write();
		memcpy((char*)w.ptr(), m_float32HeigthsBuffer.ptr(), m_float32HeigthsBuffer.size());
		//{
		//	uint16_t* ptrFrom = (uint16_t*)m_float16HeigthsBuffer.ptr();
		//	m_heights.resize((int)numFloat16Heights);
		//	godot::PoolRealArray::Write w = m_heights.write();
		//	float* ptrTo = w.ptr();
		//	for (size_t idx = 0; idx < numFloat16Heights; idx++)
		//	{
		//		TheWorld_Utils::FLOAT_32 f;
		//		f.u32 = half_to_float(*ptrFrom);
		//		*ptrTo = f.f32;
		//		ptrFrom++;
		//		ptrTo++;
		//	}
		//}
	}

	Vector3 startPosition(m_quadrantPos.getLowerXGridVertex(), minAltitude, m_quadrantPos.getLowerZGridVertex());
	Vector3 endPosition(startPosition.x + m_quadrantPos.getSizeInWU(), maxAltitude, startPosition.z + m_quadrantPos.getSizeInWU());
	Vector3 size = endPosition - startPosition;

	m_globalCoordAABB.set_position(startPosition);
	m_globalCoordAABB.set_size(size);
	
	m_meshId = meshId;

	//m_collider->setData();
}
