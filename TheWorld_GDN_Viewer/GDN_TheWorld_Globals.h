#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>
#include <SceneTree.hpp>
#include <Viewport.hpp>

#include "assert.h"

#include <plog/Log.h>
#include <MapManager.h>
#include "Utils.h"

#define THEWORLD_VIEWER_CHUNK_SIZE_SHIFT			5
#define THEWORLD_VIEWER_BITMAP_RESOLUTION_SHIFT		10

#define PLOG_LEVEL plog::info

#define THEWORLD_GLOBALS_NODE_NAME	"GDN_TheWorld_Globals"
#define THEWORLD_VIEWER_NODE_NAME	"GDN_TheWorld_Viewer"

namespace godot
{

	class GDN_TheWorld_Viewer;
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

			register_method("set_debug_enabled", &GDN_TheWorld_Globals::setDebugEnabled);
			register_method("is_debug_enabled", &GDN_TheWorld_Globals::isDebugEnabled);
			register_method("debug_print", &GDN_TheWorld_Globals::debugPrint);
			
			register_method("get_num_vertices_per_chunk_side", &GDN_TheWorld_Globals::numVerticesPerChuckSide);
			register_method("get_bitmap_resolution", &GDN_TheWorld_Globals::bitmapResolution);
			register_method("get_lod_max_depth", &GDN_TheWorld_Globals::lodMaxDepth);
			register_method("get_num_lods", &GDN_TheWorld_Globals::numLods);
			register_method("get_chunks_per_bitmap_side", &GDN_TheWorld_Globals::numChunksPerBitmapSide);
			register_method("get_grid_step_in_wu", &GDN_TheWorld_Globals::gridStepInBitmapWUs);

			register_method("viewer", &GDN_TheWorld_Globals::Viewer);
		}

		//
		// Godot Standard Functions
		//
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);

		void debugPrint(String message)
		{
			if (m_isDebugEnabled)
			{
				Godot::print(message);
				char* m = message.alloc_c_string();
				PLOGI << m;
				godot::api->godot_free(m);
			}
		}

		TheWorld_MapManager::MapManager* mapManager(void) { return m_mapManager; };

		// WORLD GRID
		// The world is rapresented by a grid map (World Grid) streamed from external source which defines also the distance of each vertex of the grid map in WUs. Such a grid is a flat grid of vertices whose elevations
		//		are contained in a bitmap.
		// 
		// THE BITMAP: ELEVATIONS
		// The elevations are contained in a bitmap (for shader purposes) each for every vertex of the World Grid. The side of the bitmap has a number of vertices (bitmapResolution) which is multiple of the number
		//		of vertices of every chunk (numVerticesPerChuckSide); all th vertices of the bitmap relate to a quad tree of chunks and such vertices don't know about their positions in global coordinate system but
		//		only to their relative position in the chunk: to compute it, it is necessary to know the chunk position in global coords and its lod (which expresses how the vertices are spaced in number of World Grid vertices).
		//
		// CHUNKS AND QUADS
		// The world is subdivided in chunks by one or more mesh : 1 mesh for every chunk which contains the grid of vertices whose elevations are all contained in a bitmap (mostly in a part of it only at lower resolution
		//		there is only a chunk in which tge vertices are more spaced).
		// The world is subdivided in chunks organized in quads which means that every chunk can be split in 4 chunk (upper-left, upper-right, lesser-left, lesser-right) at every split the lod value is decremented by 1
		//		until lod value = 0 at which the chunk has the most defined mesh while at lod = numLods - 1 has the less defined mesh.
		//		Every chunks contain a mesh which contains the grid of vertices whose elevations are all contained in a bitmap (mostly in a part of it only at lower resolution there is only a chunk in which the vertices
		//		are more spaced).
		//		Every chunk has a fixed number of vertices for every side at every lod (numVerticesPerChuckSide), at higher lods the vertices are more spaced (in terms of World Grid vertices), at lod 0 the vertices are at
		//		the maximum density. The lod value is an index whose range is from 0 (the maximum level of details in which the vertex are less spaced and the details are more defined at his level the highest number of
		//		chunk is required to fullfill the elevation bitmap and the granularity of the vertices is the same of the World Grid) to max depth (maxDepth) = number of lods (numLods) - 1 (the maximum value of the index
		//		which matches the less defined mesh in which the vertices are more spaced and only 1 chunk is required to cover the elevation bitmap).
		//		The meshes of the chunks are expressed in local coord and the chunk is responsible to position them in he real world knowing their positions in global coordinate system.
		//
		// IN SHORT
		// In short: meshes and bitmap are expressed in local coordinate system, the bitmap is related to a quad tree of chunks and the chunks know the position aof every mesh in global coordinate system as the shaders know
		//		the elvation of every vertex in the bitmap (???)
		// At maximum lod (numLods - 1) the world is rapresented by a single chunk/mesh which can be split until lod = 0 for the maximun number of chunks, the point is that not every chunk of the world rapresentaion are at the
		//		same lod : the nearest are at maximun lod (lod value = 0), the farther at lod = numLods - 1
		// The maximum lod value (lodMaxDepth) is the maximum number of split - 1 that can be done so that all the vertices of all the chunks can be contained in the bitmap : it is computed as the power of 2 of the maximum number
		//		of chunks of a side of the bitmap (bitmapResolution / numVerticesPerChuckSide), the number of lods (numLods) is equal to lodMaxDepth + 1

		// Number of vertices of the side of a chunk (-1) which is fixed (not a function of the lod) and is a multiple of 2
		int numVerticesPerChuckSide(void)
		{
			return m_numVerticesPerChuckSide; /*m_numVerticesPerChuckSide = 32 con THEWORLD_VIEWER_CHUNK_SIZE_SHIFT = 5*/
		}	// Chunk num vertices -1
		
		// Number of vertices of the side of the bitmap (-1) with the elevations which is fixed and is a multiple of the number of vertices of the side of a chunk (numVerticesPerChuckSide) and is for this a multiple of 2 too
		int bitmapResolution(void) { return m_bitmapResolution; /*m_bitmapResolution = 1024 con THEWORLD_VIEWER_BITMAP_RESOLUTION_SHIFT = 10*/ }	// Resolution of the bitmap = num point of the bitmap -1;
		// Size of the bitmap in WUs
		float bitmapSizeInWUs(void) { return (bitmapResolution() + 1) * m_mapManager->gridStepInWU(); }

		// Max value of the lod index (numLods - 1)
		int lodMaxDepth(void) {	return m_lodMaxDepth; /*m_lodMaxDepth = 5 con THEWORLD_VIEWER_CHUNK_SIZE_SHIFT = 5 e THEWORLD_VIEWER_BITMAP_RESOLUTION_SHIFT = 10*/ }

		// Max number of quad split that can be done until the the granularity of the vertices is the same of the World Grid map and of the bitmap (lodMaxDepth + 1)
		int numLods(void) { return m_numLods; /*m_numLods = 6 m_lodMaxDepth = 5*/ }

		// The number of chunks required to cover every side of the bitmap at the specified lod value
		int numChunksPerBitmapSide(int lod)
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (bitmapResolution() / numVerticesPerChuckSide()) >> lod;
		}
		
		// The number of World Grid vertices (-1) or bitmap vertices (-1) which separate the vertices of the mesh at the specified lod value
		int strideInWorldGrid(int lod) { return gridStepInBitmap(lod); }
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
		float strideInWorldGridWUs(int lod) { return gridStepInBitmapWUs(lod); }
		float gridStepInBitmapWUs(int lod)
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (gridStepInBitmap(lod) * m_mapManager->gridStepInWU());
		}

		bool isDebugEnabled(void) { return m_isDebugEnabled; }
		void setDebugEnabled(bool b = true) { m_isDebugEnabled = b; }

		void setAppInError(int errorCode, string errorText) {
			m_lastErrorCode = errorCode;
			m_lastErrorText = errorText;
			m_bAppInError = true;
			PLOG_ERROR << errorText;
		}
		bool getAppInError(void) { return m_bAppInError; }
		bool getAppInError(int& lastErrorCode, string& lastErrorText) {
			lastErrorCode = m_lastErrorCode;
			lastErrorText = m_lastErrorText;
			return m_bAppInError;
		}

		GDN_TheWorld_Viewer* Viewer(bool useCache = true);

	private:
		bool m_isDebugEnabled;
		int m_numVerticesPerChuckSide;
		int m_bitmapResolution;
		int m_lodMaxDepth;
		int m_numLods;
		TheWorld_MapManager::MapManager *m_mapManager;
		bool m_bAppInError;
		int m_lastErrorCode;
		string m_lastErrorText;

		// Node cache
		GDN_TheWorld_Viewer* m_viewer;
	};
}

#define THEWORLD_VIEWER_GENERIC_ERROR				1

