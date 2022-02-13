//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

#include "assert.h"
#include "QuadTree.h"

using namespace godot;

// QuadTree coordinates are local to Viewer Node : the first chunk (posX = posZ = 0) has its lower vertex in the origin of the Viwer Node

Quad::Quad(int posX, int posZ, Chunk* chunk, int lod)
{
	m_posX = posX;
	m_posZ = posZ;
	m_chunk = chunk;
	m_lod = lod;

	if (m_chunk)
		createChunk();
}

Quad::~Quad()
{
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
		int x = (m_posX * 2) + (i & 1);
		int y = (m_posZ * 2) + ((i & 2) >> 1);
		m_children[i] = make_unique<Quad>((m_posX * 2) + (i & 1), (m_posZ * 2) + ((i & 2) >> 1), nullptr, m_lod - 1);
	}
	
	recycleChunk();
}

void Quad::recycleChunk(void)
{
	if (m_chunk)
	{
		// TODO: recycle chunk
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
	
	// TODO: create chunk for current quad
}

QuadTree::QuadTree(GDN_TheWorld_Viewer* viewer)
{
	m_viewer = viewer;
	m_root = make_unique<Quad>(0, 0, nullptr, m_viewer->Globals()->lodMaxDepth());
}

QuadTree::~QuadTree()
{
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
	Vector3 chunkCenter = real_t(chunkSize) * (Vector3(real_t(quad->PosX()), 0, real_t(quad->PosZ())) + Vector3(0.5, 0, 0.5));
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
		}
	}
}
