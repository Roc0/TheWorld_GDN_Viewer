#pragma once
#include <Godot.hpp>
#include <Node.hpp>

#include <map>
#include <vector>
#include <array>
#include <memory>

#include "Chunk.h"

namespace godot
{
	class GDN_TheWorld_Viewer;
	class QuadTree;

	class Quad
	{
	public:
		Quad(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Chunk* chunk = nullptr);
		~Quad();

		int slotPosX()
		{
			return m_slotPosX;
		}
		int slotPosZ()
		{
			return m_slotPosZ;
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
		Chunk* getChunk(void) { return m_chunk; }

	private:
		std::array<std::unique_ptr<Quad>, 4> m_children;
		Chunk* m_chunk;
		int m_slotPosX;	// express the orizzontal (X) and vertical (Z) position of the quad (and of the corrispondig chunk) in the grid of chunks
		int m_slotPosZ;	// at the specific lod : 0 the first chunk, 1 the following to the max number of chunks on a size for the specific lod
		int m_lod;
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
	};


	class QuadTree
	{
	public:
		QuadTree(GDN_TheWorld_Viewer* viewer);
		~QuadTree();

		void init(void);
		
		void update(Vector3 viewerPosLocalCoord, Vector3 viewerPosGlobalCoord);
		Chunk* getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir);
		Chunk* getChunkAt(Chunk::ChunkPos pos);
		void addChunk(Chunk* chunk);
		void addChunkUpdate(Chunk* chunk);
		void clearChunkUpdate(void);
		std::vector<Chunk*>& getChunkUpdate(void) { return m_vectChunkUpdate; }
		void ForAllChunk(Chunk::ChunkAction& chunkAction);
		void dump(void);

	private:
		void internalUpdate(Vector3 cameraPosViewerNodeLocalCoord, Vector3 viewerPosGlobalCoord, Quad* quadTreeNode);

	private:
		std::unique_ptr<Quad> m_root;
		GDN_TheWorld_Viewer* m_viewer;
		Chunk::MapChunk m_mapChunk;
		std::vector<Chunk*> m_vectChunkUpdate;

		// Statistics
		int m_numSplits;
		int m_numJoin;
		int m_numLeaf;
	};
}
