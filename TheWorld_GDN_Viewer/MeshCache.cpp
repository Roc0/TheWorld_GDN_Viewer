//#include "pch.h"
#include "MeshCache.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"

#include <PoolArrays.hpp>

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
		//	https://gist.github.com/bnolan/01a7d240774f1e02056d6607e5b621da

		int stride = m_viewer->Globals()->strideInGrid(lod);

		PoolVector3Array positions;
		positions.resize((int)pow(numVerticesPerChuckSide + 1, 2));
		for (real_t z = 0; z < numVerticesPerChuckSide + 1; z++)
			for (real_t x = 0; x < numVerticesPerChuckSide + 1; x++)
				positions.append(Vector3(x * stride, 0, z * stride));
		
		PoolIntArray indices;
		void makeFlatChunk_makeIndices(PoolIntArray& indices, int numVerticesPerChuckSide, int seams);
		makeFlatChunk_makeIndices(indices, numVerticesPerChuckSide, seams);

		godot::Array arrays;
		arrays.resize(ArrayMesh::ARRAY_MAX);
		arrays[ArrayMesh::ARRAY_VERTEX] = positions;
		arrays[ArrayMesh::ARRAY_INDEX] = indices;

		ArrayMesh* mesh = ArrayMesh::_new();
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

		return mesh;
	}
}

void makeFlatChunk_makeIndices(godot::PoolIntArray& indices, int numVerticesPerChuckSide, int seams)
{
	// TODO
	return;
}
