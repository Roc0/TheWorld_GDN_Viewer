#pragma once
#include <Godot.hpp>
#include <Node.hpp>

#include <array>
#include <memory>

#include "Chunk.h"

namespace godot
{
	class GDN_TheWorld_Viewer;

	class Quad
	{
	public:
		Quad(int posX, int posZ, Chunk* chunk, int lod);
		~Quad();

		int PosX()
		{
			return m_posX;
		}
		int PosZ()
		{
			return m_posZ;
		}
		int Lod()
		{
			return m_lod;
		}

		bool isLeaf(void);
		Quad *getChild(int idx);
		void recycleChunk(void);
		void split(void);
		void clearChildren(void);
		void createChunk(void);

	private:
		std::array<std::unique_ptr<Quad>, 4> m_children;
		Chunk* m_chunk;
		int m_posX;	// express the orizzontal (X) and vertical (Z) position of the chunk in the grid of chunks
		int m_posZ;	// at the specific lod : 0 the first chunk, 1 the following to the max number of chunks for the specific lod
		int m_lod;
	};


	class QuadTree
	{
	public:
		QuadTree(GDN_TheWorld_Viewer* viewer);
		~QuadTree();

		void update(Vector3 viewerPosLocalCoord, Vector3 viewerPosGlobalCoord);

	private:
		void internalUpdate(Vector3 cameraPosViewerNodeLocalCoord, Vector3 viewerPosGlobalCoord, Quad* quadTreeNode);

	private:
		std::unique_ptr<Quad> m_root;
		GDN_TheWorld_Viewer* m_viewer;
	};
}
