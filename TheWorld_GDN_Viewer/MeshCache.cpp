//#include "pch.h"
#include "MeshCache.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"

//using namespace godot;
namespace godot
{
	MeshCache::MeshCache(GDN_TheWorld_Viewer* viewer)
	{
		m_viewer = viewer;
	}

	MeshCache::~MeshCache()
	{
		resetCache();
	}

	void MeshCache::initCache(int numVerticesPerChuckSide, int numLods)
	{
		resetCache();

		m_meshCache.resize(SEAM_CONFIG_COUNT);
		
		for (int idxSeams = 0; idxSeams < SEAM_CONFIG_COUNT; idxSeams++)
		{
			m_meshCache[idxSeams].resize(m_viewer->Globals()->numLods());

			for (int idxLod = 0; idxLod < m_viewer->Globals()->numLods(); idxLod++)
			{
				m_meshCache[idxSeams][idxLod] = makeFlatChunk(numVerticesPerChuckSide, idxLod, idxSeams);
			}
		}
	}

	void MeshCache::resetCache(void)
	{
		for (int idxSeams = 0; idxSeams < m_meshCache.size(); idxSeams++)
		{
			for (int idxLod = 0; idxLod < m_meshCache[idxSeams].size(); idxLod++)
			{
				m_meshCache[idxSeams][idxLod]->call_deferred("free");
			}
			m_meshCache[idxSeams].clear();
		}
		m_meshCache.clear();
	}

	ArrayMesh* MeshCache::makeFlatChunk(int numVerticesPerChuckSide, int lod, int seams)
	{
		ArrayMesh* mesh = ArrayMesh::_new();

		int stride = m_viewer->Globals()->strideInGrid(lod);

		return mesh;
	}
}