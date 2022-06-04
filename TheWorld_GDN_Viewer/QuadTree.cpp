//#include "pch.h"
#include "QuadTree.h"

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

#include "assert.h"

using namespace godot;

// QuadTree coordinates are local to Viewer Node which contains the grid of quads: the first chunk (posX = posZ = 0) has its lower vertex in the origin of the Viwer Node

Quad::Quad(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Chunk* chunk)
{
	m_slotPosX = slotPosX;
	m_slotPosZ = slotPosZ;
	m_chunk = chunk;
	m_lod = lod;
	m_viewer = viewer;
	m_chunkSizeInWUs = 0;

	m_quadTree = m_viewer->getQuadTree();
	
	if (m_chunk == nullptr)
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
		int x = (m_slotPosX * 2) + (i & 1);
		int y = (m_slotPosZ * 2) + ((i & 2) >> 1);
		m_children[i] = make_unique<Quad>((m_slotPosX * 2) + (i & 1), (m_slotPosZ * 2) + ((i & 2) >> 1), m_lod - 1, m_viewer);
	}
	
	recycleChunk();
}

void Quad::recycleChunk(void)
{
	if (m_chunk)
	{
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
		Ref<Material> mat = m_viewer->getShaderTerrainData().getMaterial();
		if (_DEBUG_AAB)
			m_chunk = new ChunkDebug(m_slotPosX, m_slotPosZ, m_lod, m_viewer, mat);
		else
			m_chunk = new Chunk(m_slotPosX, m_slotPosZ, m_lod, m_viewer, mat);

		m_chunk->setParentGlobalTransform(m_viewer->internalTransformGlobalCoord());
		m_quadTree->addChunk(m_chunk);
	}

	m_quadTree->addChunkUpdate(m_chunk);
	m_chunk->setActive(true);
	m_chunkAABB = m_chunk->getAABB();
	m_chunkSizeInWUs = m_chunk->getChunkSizeInWUs();
}

void Quad::setCameraPos(Vector3 localToGriddCoordCameraLastPos, Vector3 globalCoordCameraLastPos)
{
	if (isLeaf())
	{
		Chunk* chunk = getChunk();
		if (chunk != nullptr)
			chunk->setCameraPos(localToGriddCoordCameraLastPos, globalCoordCameraLastPos);
	}
}
	
QuadTree::QuadTree(GDN_TheWorld_Viewer* viewer)
{
	m_viewer = viewer;
	m_numSplits = 0;
	m_numJoins = 0;
	m_numLeaf = 0;
}

void QuadTree::init(void)
{
	m_root = make_unique<Quad>(0, 0, m_viewer->Globals()->lodMaxDepth(), m_viewer);
}

QuadTree::~QuadTree()
{
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
}

void QuadTree::ForAllChunk(Chunk::ChunkAction& chunkAction)
{
	for (Chunk::MapChunk::iterator it = m_mapChunk.begin(); it != m_mapChunk.end(); it++)
	{
		for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
		{
			chunkAction.exec(it1->second);
		}
	}
}

void QuadTree::update(Vector3 cameraPosViewerNodeLocalCoord, Vector3 cameraPosGlobalCoord)
{
	m_numLeaf = 0;
	internalUpdate(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord, m_root.get());
	assert(!m_root->isLeaf() || m_root->getChunk() != nullptr);
}

void QuadTree::internalUpdate(Vector3 cameraPosViewerNodeLocalCoord, Vector3 cameraPosGlobalCoord, Quad* quad)
{
	// cameraPosViewerNodeLocalCoord are in grid local coordinates (WUs)

	GDN_TheWorld_Globals* globals = m_viewer->Globals();
	//float chunkSizeInWUs = globals->gridStepInBitmapWUs(quad->Lod()) * globals->numVerticesPerChuckSide();									// chunk size in World Units
	float chunkSizeInWUs = quad->getChunkSizeInWUs();
	Vector3 chunkCenter = real_t(chunkSizeInWUs) * (Vector3(real_t(quad->slotPosX()), 0, real_t(quad->slotPosZ())) + Vector3(0.5, 0, 0.5));	// chunk center in local coord. respect Viewer Node (or the grid of chunks)
	AABB chunkAABB = quad->getChunkAABB();
	chunkCenter.y = (chunkAABB.position + chunkAABB.size / 2).y;
	real_t splitDistance = chunkSizeInWUs * globals->splitScale();
	if (quad->isLeaf())
	{
		
		m_numLeaf++;
		if (quad->Lod() > 0)
		{
			real_t distance = chunkCenter.distance_to(cameraPosViewerNodeLocalCoord);
			if (distance < splitDistance)
			{
				// Split
				quad->split();
				for (int i = 0; i < 4; i++)
					quad->getChild(i)->setCameraPos(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord);;
				m_numSplits++;
			}
		}
	}
	else
	{
		bool allChildrenAreLeaf = true;
		for (int i = 0; i < 4; i++)
		{
			Quad* child = quad->getChild(i);
			internalUpdate(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord, child);
			if (!child->isLeaf())
				allChildrenAreLeaf = false;
		}
		
		if (allChildrenAreLeaf && chunkCenter.distance_to(cameraPosViewerNodeLocalCoord) > splitDistance)
		{
			// Join
			quad->clearChildren();
			quad->assignChunk();
			quad->getChunk()->setJustJoined(true);
			m_numJoins++;
		}
	}
	
	// after all if quad is leaf: just joined or was leaf and it has not been splitted
	quad->setCameraPos(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord);
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir)
{
	Chunk* chunk = nullptr;

	switch (dir)
	{
	case Chunk::DirectionSlot::Center:
		chunk = getChunkAt(pos);
		break;
	case Chunk::DirectionSlot::Left:
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
		break;
	case Chunk::DirectionSlot::Right:
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 0, -1));
		break;
	case Chunk::DirectionSlot::Bottom:
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
		break;
	case Chunk::DirectionSlot::Top:
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 1, -1));
		break;
	case Chunk::DirectionSlot::LeftTop:
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
		break;
	case Chunk::DirectionSlot::LeftBottom:
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 1, -1));
		break;
	case Chunk::DirectionSlot::RightTop:
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 0, -1));
		break;
	case Chunk::DirectionSlot::RightBottom:
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 1, -1));
		break;
	case Chunk::DirectionSlot::BottomLeft:
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
		break;
	case Chunk::DirectionSlot::BottomRight:
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, -1, -1));
		break;
	case Chunk::DirectionSlot::TopLeft:
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 2, -1));
		break;
	case Chunk::DirectionSlot::TopRight:
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

void QuadTree::dump(void)
{
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	globals->debugPrint(String("Num vertices per chunk side: ") + String(to_string(globals->numVerticesPerChuckSide()).c_str())
				+ String(" - ") + String("Num vertices pert grid size: ") + String(to_string(globals->heightmapResolution()).c_str())
				+ String(" - ") + String("Grid size [WUs]: ") + String(to_string(globals->heightmapSizeInWUs()).c_str())
				+ String(" - ") + String("Lod max depth: ") + String(to_string(globals->lodMaxDepth()).c_str()));

	globals->debugPrint(String("FROM LAST DUMP - Num quad split: ") + String(to_string(m_numSplits).c_str())
				+ String(" - ") + String("Num quad join: ") + String(to_string(m_numJoins).c_str())
				+ String(" - ") + String("Num leaves: ") + String(to_string(m_numLeaf).c_str()));

	globals->debugPrint(String("GRID POS (global)")
		+ String(" - ") + String("X = ") + String(to_string(m_viewer->get_global_transform().origin.x).c_str())
		+ String(" - ") + String("Z = ") + String(to_string(m_viewer->get_global_transform().origin.z).c_str()));

	int numChunks = 0;
	int numActiveChunks = 0;
	for (int i = globals->lodMaxDepth(); i >= 0; i--)
	{
		int numChunksAtLod = 0;
		Chunk::MapChunk::iterator it = m_mapChunk.find(i);
		if (it != m_mapChunk.end())
			numChunksAtLod = int(m_mapChunk[i].size());
		int numChunksActiveAtLod = 0;
		for (Chunk::MapChunkPerLod::iterator it1 = m_mapChunk[i].begin(); it1 != m_mapChunk[i].end(); it1++)
			if (it1->second->isActive())
				numChunksActiveAtLod++;
		globals->debugPrint("Num chunks at lod " + String(to_string(i).c_str()) + ": " + String(to_string(numChunksAtLod).c_str())
			+ String("\t") + " Active " + String(to_string(numChunksActiveAtLod).c_str()));
		numChunks += numChunksAtLod;
		numActiveChunks += numChunksActiveAtLod;
	}

	globals->debugPrint("Num chunks: " + String(to_string(numChunks).c_str()));
	globals->debugPrint("Num active chunks: " + String(to_string(numActiveChunks).c_str()));
	Chunk::DumpChunkAction action;
	ForAllChunk(action);


	m_numSplits = 0;
	m_numJoins = 0;
}
