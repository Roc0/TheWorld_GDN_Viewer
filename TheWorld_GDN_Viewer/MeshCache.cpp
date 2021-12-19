//#include "pch.h"
#include "MeshCache.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"

//using namespace godot;
namespace godot
{
	MeshCache::MeshCache(Node* viewer)
	{
		m_viewer = viewer;
	}

	MeshCache::~MeshCache()
	{
	}

	void MeshCache::initCache(int numVerticesPerChuckSide, int numLods)
	{
		for (int idxSeam = 0; idxSeam < SEAM_CONFIG_COUNT; idxSeam++)
		{
			for (int idxLod = 0; idxLod < ((GDN_TheWorld_Viewer*)(m_viewer))->Globals()->numLods(); idxLod++)
			{
			}
		}
	}
}