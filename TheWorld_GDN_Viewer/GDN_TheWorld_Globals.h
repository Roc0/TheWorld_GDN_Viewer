#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>

#include "assert.h"

#include <MapManager.h>

#define THEWORLD_CHUNK_SIZE_SHIFT				5
#define THEWORLD_BITMAP_RESOLUTION_SHIFT		10

namespace godot
{

	class TheWorld_MapManager::MapManager;

	class GDN_TheWorld_Globals : public Node
	{
		GODOT_CLASS(GDN_TheWorld_Globals, Node)

	public:
		GDN_TheWorld_Globals();
		~GDN_TheWorld_Globals();

		static void _register_methods()
		{
			register_method("_ready", &GDN_TheWorld_Globals::_ready);
			register_method("_process", &GDN_TheWorld_Globals::_process);
			register_method("_input", &GDN_TheWorld_Globals::_input);

			register_method("get_num_vertices_per_chunk_side", &GDN_TheWorld_Globals::numVerticesPerChuckSide);
			register_method("get_bitmap_resolution", &GDN_TheWorld_Globals::bitmapResolution);
			register_method("get_lod_max_depth", &GDN_TheWorld_Globals::lodMaxDepth);
			register_method("get_num_lods", &GDN_TheWorld_Globals::numLods);
			register_method("get_chunks_per_bitmap_side", &GDN_TheWorld_Globals::numChunksPerBitmapSide);
			register_method("get_grid_step_in_wu", &GDN_TheWorld_Globals::gridStepInBitmapWU);
		}

		//
		// Godot Standard Functions
		//
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);

		TheWorld_MapManager::MapManager* mapManager(void) { return m_mapManager; };

		// The world is rapresented by one or more mesh : 1 mesh for every chunk which contains the grid of vertices whose elevations are all contained in a bitmap.
		// The elevations are contained in a bitmap (for shader purposes) whose side has a number of vertices (bitmapResolution) which is multiple of the number of vertices of every chunk (numVerticesPerChuckSide); all th vertices
		//		of the bitmpa relate to a quad tree of chunks and as the meshes don't know about their positions in global coordinate system
		// The chunks are organized in quads which means that every chunk can be split in 4 chunk (uppepr-left, upper-right, lesser-left, lesser-right) at every split the lod value is decremented by 1 until lod value = 0 at which
		//		the chunk has the most defined mesh while at lod = numLods - 1 has the less defined mesh
		//		Every chunk has a fixed number of vertices for every side at every lod (numVerticesPerChuckSide), at higher lods the vertices are more spaced, at lod 0 the vertices are at the maximum density
		//		The lod value is an index whose range is from 0 (the maximum level of details in which the vertex are less spaced and the details are more defined at his level the highest number of chunk is required to fullfill
		//		the elevation bitmap) to max depth (maxDepth) = number of lods (numLods) - 1 (the maximum value of the index which matches the less defined mesh in which the vertices are more spaced and only 1 chunk is required
		//		to cover the elevation bitmap)
		//		The meshes of the chunks are expressed in local coord and the chunk is responsible to position them in he real world know their positions in global coordinate system
		// In short: meshes and bitmap are expressed in local coordinate system, the bitmap is related to a quad tree of chunks and the chunks know the position aof every mesh in global coordinate system as the shaders know
		//		the elvation of every vertex in the bitmap (???)
		// At maximum lod (numLods - 1) the world is rapresented by a single chunk/mesh which can be split until lod = 0 for the maximun number of chunks, the point is that not every chunk of the world rapresentaion are at the
		//		same lod : the nearest are at maximun lod (lod value = 0), the farther at lod = numLods - 1
		// The maximum lod value (lodMaxDepth) is the maximum number of split - 1 that can be done so that all the vertices of all the chunks can be contained in the bitmap : it is computed as the power of 2 of the maximum number
		//		of chunks of a side of the bitmap (bitmapResolution / numVerticesPerChuckSide), the number of lods (numLods) is equal to lodMaxDepth + 1

		// Number of vertices of the side of a chunk (-1) which is fixed at every lod and is a multiple of 2
		int numVerticesPerChuckSide(void) { return m_numVerticesPerChuckSide; }					// Chunk num vertices -1
		
		// Number of vertices of the side of the bitmap (-1) with the elevations which is fixed and is a multiple of the number of vertices of the side of a chunk (numVerticesPerChuckSide) and is for this a multiple of 2 too
		int bitmapResolution(void) { return m_bitmapResolution; }	// Resolution of the bitmap = num point of the bitmap -1;

		// Max number of quad split that can be done so that the bitmap can contain all the vertices
		int lodMaxDepth(void) {	return m_lodMaxDepth; }

		// Max number of level of detail possible so that the bitmap can contain all the vertices
		int numLods(void) { return m_numLods; }

		// The number of chunks required to cover every side of the bitmap at the specified lod value
		int numChunksPerBitmapSide(int lod)
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (bitmapResolution() / numVerticesPerChuckSide()) >> lod;
		}
		
		// The number of grid vertices (-1) which separate the vertices of the mesh at the specified lod value
		int strideInGrid(int lod) { return gridStepInBitmap(lod); }
		int gridStepInBitmap(int lod)
		{
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			//int ret = bitmapResolution() / (numChunksPerBitmapSide(lod) * numVerticesPerChuckSide());
			//int ret1 = 1 << lod;
			//assert(ret == ret1);
			return (1 << lod);
		}

		// The number of World Units which separate the vertices of the mesh at the specified lod value
		float gridStepInBitmapWU(int lod)
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (gridStepInBitmap(lod) * m_mapManager->gridStepInWU());
		}

	private:
		int m_numVerticesPerChuckSide;
		int m_bitmapResolution;
		int m_lodMaxDepth;
		int m_numLods;
		TheWorld_MapManager::MapManager *m_mapManager;
	};

}


