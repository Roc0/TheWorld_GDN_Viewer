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

	m_quadTree = m_viewer->getQuadTree();
	
	if (m_chunk == nullptr)
		createChunk();
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
	/*if (m_children[0].get() == nullptr)
		return true;
	else
		return false;*/
}

void Quad::split(void)
{
	assert(m_lod > 0);
	
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
	for (int i = 0; i < 4; i++)
	{
		m_children[i]->recycleChunk();
		m_children[i].reset();
	}
}

void Quad::createChunk(void)
{
	assert(m_chunk == nullptr);
	
	m_chunk = m_quadTree->getChunkAt(Chunk::ChunkPos(m_slotPosX, m_slotPosZ, m_lod));

	if (m_chunk == nullptr)
	{
		// TODORIC Material stuff

		// create chunk for current quad because this is the first time that is needed for current lod and pos
		m_chunk = new Chunk(m_slotPosX, m_slotPosZ, m_lod, m_viewer, nullptr);
		m_chunk->parentTransformChanged(m_viewer->internalTransformGlobalCoord());
		m_quadTree->addChunk(m_chunk);
	}

	m_quadTree->addChunkUpdate(m_chunk);
	m_chunk->setActive(true);
}
	
QuadTree::QuadTree(GDN_TheWorld_Viewer* viewer)
{
	m_viewer = viewer;
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
		it->second.clear();
	}
}

void QuadTree::update(Vector3 cameraPosViewerNodeLocalCoord, Vector3 cameraPosGlobalCoord)
{
	return internalUpdate(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord, m_root.get());
}

void QuadTree::internalUpdate(Vector3 cameraPosViewerNodeLocalCoord, Vector3 cameraPosGlobalCoord, Quad* quad)
{
	GDN_TheWorld_Globals* globals = m_viewer->Globals();
	int gridStep = globals->gridStepInBitmap(quad->Lod());	// The number of World Grid vertices (-1) or bitmap vertices (-1) which separate the vertices of the mesh at the specified lod value 
	int chunkSize = gridStep * globals->numVerticesPerChuckSide();	// chunk size in number of World Grid vertices
	Vector3 chunkCenter = real_t(chunkSize) * (Vector3(real_t(quad->slotPosX()), 0, real_t(quad->slotPosZ())) + Vector3(0.5, 0, 0.5));
	real_t splitDistance = chunkSize * globals->splitScale();
	if (quad->isLeaf())
	{
		if (quad->Lod() > 0 && chunkCenter.distance_to(cameraPosViewerNodeLocalCoord) < splitDistance)
			quad->split();
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
			quad->clearChildren();
			quad->createChunk();
			quad->getChunk()->setJustJoined(true);
		}
	}
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir)
{
	Chunk* chunk = nullptr;

	switch (dir)
	{
	case Chunk::DirectionSlot::Center:
		chunk = getChunkAt(pos);
	case Chunk::DirectionSlot::Left:
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
	case Chunk::DirectionSlot::Right:
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 0, -1));
	case Chunk::DirectionSlot::Bottom:
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
	case Chunk::DirectionSlot::Top:
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 1, -1));
	case Chunk::DirectionSlot::LeftTop:
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
	case Chunk::DirectionSlot::LeftBottom:
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 1, -1));
	case Chunk::DirectionSlot::RightTop:
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 0, -1));
	case Chunk::DirectionSlot::RightBottom:
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 1, -1));
	case Chunk::DirectionSlot::BottomLeft:
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
	case Chunk::DirectionSlot::BottomRight:
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, -1, -1));
	case Chunk::DirectionSlot::TopLeft:
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 2, -1));
	case Chunk::DirectionSlot::TopRight:
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 2, -1));
	default:
		return chunk;
	}
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