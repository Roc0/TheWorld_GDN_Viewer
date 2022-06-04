#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>
#include <SceneTree.hpp>
#include <Viewport.hpp>

#include "assert.h"
#include "vector"

#include <plog/Log.h>
#include <MapManager.h>
#include "Utils.h"

#define THEWORLD_VIEWER_CHUNK_SIZE_SHIFT				5
#define THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT		10

#define PLOG_DEFAULT_LEVEL plog::Severity::info
#define PLOG_DEBUG_LEVEL plog::Severity::verbose

#define VISUAL_SERVER_WIREFRAME_MODE	true
#define DEFAULT_DEBUG_ENABLED			true

#define THEWORLD_GLOBALS_NODE_NAME	"GDN_TheWorld_Globals"
#define THEWORLD_VIEWER_NODE_NAME	"GDN_TheWorld_Viewer"

namespace godot
{

	static bool equal(Vector3 v1, Vector3 v2, const float epsilon = 0.00001)
	{
		return ::equal((float)v1.x, (float)v2.x, epsilon) && ::equal((float)v1.y, (float)v2.y, epsilon) && ::equal((float)v1.z, (float)v2.z, epsilon);
	}

	class GDN_TheWorld_Viewer;
	class TheWorld_MapManager::MapManager;

	class GDN_TheWorld_Globals : public Node
	{
		GODOT_CLASS(GDN_TheWorld_Globals, Node)

		//const float c_splitScale = 1.0F;
		//const float c_splitScale = 2.0F;
		const float c_splitScale = 1.5F;

	public:
		const static Color g_color_white;
		const static Color g_color_red;
		const static Color g_color_dark_red;
		const static Color g_color_red_salmon;
		const static Color g_color_pink;
		const static Color g_color_light_pink;
		const static Color g_color_pink_amaranth;
		const static Color g_color_pink_bisque;
		const static Color g_color_pink_cerise;
		const static Color g_color_green;
		const static Color g_color_green1;
		const static Color g_color_pale_green;
		const static Color g_color_light_green;
		const static Color g_color_darksea_green;
		const static Color g_color_aquamarine_green;
		const static Color g_color_blue;
		const static Color g_color_yellow;
		const static Color g_color_yellow_almond;
		const static Color g_color_yellow_amber;
		const static Color g_color_yellow_apricot;
		const static Color g_color_cyan;
		const static Color g_color_light_cyan;
		const static Color g_color_black;

		enum class ChunkDebugMode
		{
			DoNotSet = -1,
			NoDebug = 0,
			WireframeOnAABB = 1,
			WireframeSquare = 2
		};
		static ChunkDebugMode rotateChunkDebugMode(ChunkDebugMode mode)
		{
			if (mode == GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
				return GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB;
			if (mode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB)
				return GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare;
			if (mode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
				return GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
			else
				return GDN_TheWorld_Globals::ChunkDebugMode::DoNotSet;
		}

		GDN_TheWorld_Globals();
		~GDN_TheWorld_Globals();
		void init(void);
		void deinit(void);
		bool resize(int chunkSizeShift, int heightmapResolutionShift, bool force = false);

		static void _register_methods()
		{
			register_method("_ready", &GDN_TheWorld_Globals::_ready);
			register_method("_process", &GDN_TheWorld_Globals::_process);
			register_method("_input", &GDN_TheWorld_Globals::_input);
			register_method("_notification", &GDN_TheWorld_Globals::_notification);

			register_method("set_debug_enabled", &GDN_TheWorld_Globals::setDebugEnabled);
			register_method("is_debug_enabled", &GDN_TheWorld_Globals::isDebugEnabled);
			register_method("debug_print", &GDN_TheWorld_Globals::debugPrint);
			register_method("error_print", &GDN_TheWorld_Globals::errorPrint);
			register_method("warning_print", &GDN_TheWorld_Globals::warningPrint);
			register_method("print", &GDN_TheWorld_Globals::print);

			register_method("set_app_in_error", &GDN_TheWorld_Globals::setAppInError);
			register_method("get_app_in_error", &GDN_TheWorld_Globals::getAppInError);
			register_method("get_app_in_error_code", &GDN_TheWorld_Globals::getAppInErrorCode);
			register_method("get_app_in_error_message", &GDN_TheWorld_Globals::getAppInErrorMsg);

			register_method("get_num_vertices_per_chunk_side", &GDN_TheWorld_Globals::numVerticesPerChuckSide);
			register_method("get_bitmap_resolution", &GDN_TheWorld_Globals::heightmapResolution);
			register_method("get_lod_max_depth", &GDN_TheWorld_Globals::lodMaxDepth);
			register_method("get_num_lods", &GDN_TheWorld_Globals::numLods);
			register_method("get_chunks_per_bitmap_side", &GDN_TheWorld_Globals::numChunksPerHeightmapSide);
			register_method("get_grid_step_in_wu", &GDN_TheWorld_Globals::gridStepInHeightmapWUs);

			register_method("viewer", &GDN_TheWorld_Globals::Viewer);
		}

		//
		// Godot Standard Functions
		//
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);
		void _notification(int p_what);

		void debugPrint(String message)
		{
			if (m_isDebugEnabled)
			{
				String msg = "DEBUG - " + message;
				Godot::print(msg);
				char* m = msg.alloc_c_string();
				PLOGD << m;
				godot::api->godot_free(m);
			}
		}

		void warningPrint(String message)
		{
			String msg = "WARNING - " + message;
			Godot::print(msg);
			char* m = msg.alloc_c_string();
			PLOGW << m;
			godot::api->godot_free(m);
		}

		void errorPrint(String message)
		{
			String msg = "ERROR - " + message;
			Godot::print(msg);
			char* m = msg.alloc_c_string();
			PLOGE << m;
			godot::api->godot_free(m);
		}

		void print(String message)
		{
			Godot::print(message);
			char* m = message.alloc_c_string();
			PLOGI << m;
			godot::api->godot_free(m);
		}

		TheWorld_MapManager::MapManager* mapManager(void) {
			return m_mapManager;
		};

		// WORLD GRID
		// The world is rapresented by a grid map (World Grid) streamed from external source which defines also the distance of each vertex of the grid map in WUs. Such a grid is a flat grid of vertices whose elevations
		//		are contained in a heightmap.
		// 
		// THE HEIGHTMAP: ELEVATIONS
		// The elevations are contained in a heightmap (for shader purposes) each for every vertex of the World Grid. The side of the heightmap has a number of vertices (heightmapResolution) which is multiple of the number
		//		of vertices of every chunk (numVerticesPerChuckSide); all th vertices of the heightmap relate to a quad tree of chunks and such vertices don't know about their positions in global coordinate system but
		//		only to their relative position in the chunk: to compute it, it is necessary to know the chunk position in global coords and its lod (which expresses how the vertices are spaced in number of World Grid vertices).
		//
		// CHUNKS AND QUADS
		// The world is subdivided in chunks by one or more mesh : 1 mesh for every chunk which contains the grid of vertices whose elevations are all contained in a heightmap (mostly in a part of it only at lower resolution
		//		there is only a chunk in which tge vertices are more spaced).
		// The world is subdivided in chunks organized in quads which means that every chunk can be split in 4 chunk (upper-left, upper-right, lesser-left, lesser-right) at every split the lod value is decremented by 1
		//		until lod value = 0 at which the chunk has the most defined mesh while at lod = numLods - 1 has the less defined mesh.
		//		Every chunks contain a mesh which contains the grid of vertices whose elevations are all contained in a heightmap (mostly in a part of it only at lower resolution there is only a chunk in which the vertices
		//		are more spaced).
		//		Every chunk has a fixed number of vertices for every side at every lod (numVerticesPerChuckSide), at higher lods the vertices are more spaced (in terms of World Grid vertices), at lod 0 the vertices are at
		//		the maximum density. The lod value is an index whose range is from 0 (the maximum level of details in which the vertex are less spaced and the details are more defined at his level the highest number of
		//		chunk is required to fullfill the elevation heightmap and the granularity of the vertices is the same of the World Grid) to max depth (maxDepth) = number of lods (numLods) - 1 (the maximum value of the index
		//		which matches the less defined mesh in which the vertices are more spaced and only 1 chunk is required to cover the elevation hryghtmap).
		//		The meshes of the chunks are expressed in local coord and the chunk is responsible to position them in he real world knowing their positions in global coordinate system.
		//
		// IN SHORT
		// In short: meshes and hryghtmap are expressed in local coordinate system, the heightmap is related to a quad tree of chunks and the chunks know the position aof every mesh in global coordinate system as the shaders know
		//		the elvation of every vertex in the heightmap (???)
		// At maximum lod (numLods - 1) the world is rapresented by a single chunk/mesh which can be split until lod = 0 for the maximun number of chunks, the point is that not every chunk of the world rapresentaion are at the
		//		same lod : the nearest are at maximun lod (lod value = 0), the farther at lod = numLods - 1
		// The maximum lod value (lodMaxDepth) is the maximum number of split - 1 that can be done so that all the vertices of all the chunks can be contained in the heightmap : it is computed as the power of 2 of the maximum number
		//		of chunks of a side of the heightmap (heightmapResolution / numVerticesPerChuckSide), the number of lods (numLods) is equal to lodMaxDepth + 1

		// Number of vertices of the side of a chunk (-1) which is fixed (not a function of the lod) and is a multiple of 2
		int numVerticesPerChuckSide(void)
		{
			return m_numVerticesPerChuckSide; /* m_numVerticesPerChuckSide = 32 con THEWORLD_VIEWER_CHUNK_SIZE_SHIFT = 5 */
		}	// Chunk num vertices -1
		
		float gridStepInWU(void)
		{
			return m_mapManager->gridStepInWU();
		}
			
		// Number of vertices of the side of the heightmap (-1) with the elevations which is fixed and is a multiple of the number of vertices of the side of a chunk (numVerticesPerChuckSide) and is for this a multiple of 2 too
		int heightmapResolution(void) { return m_heightmapResolution; /* m_heightmapResolution = 1024 con THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT = 10 */ }	// Resolution of the heightmap = num point of the heightmap -1;
		// Size of the heightmap in WUs
		float heightmapSizeInWUs(void) {
			return (heightmapResolution() + 1) * m_mapManager->gridStepInWU();
		}

		// Max value of the lod index (numLods - 1)
		int lodMaxDepth(void) {	return m_lodMaxDepth; /* m_lodMaxDepth = 5 con THEWORLD_VIEWER_CHUNK_SIZE_SHIFT = 5 e THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT = 10 */ }

		// Max number of quad split that can be done until the the granularity of the vertices is the same of the World Grid map and of the heightmap (lodMaxDepth + 1)
		int numLods(void) { return m_numLods; /* m_numLods = 6 m_lodMaxDepth = 5 */ }

		// The number of chunks required to cover every side of the heightmap at the specified lod value
		int numChunksPerHeightmapSide(int lod)
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (heightmapResolution() / numVerticesPerChuckSide()) >> lod;
		}
		
		// The number of World Grid vertices (-1) or heightmap vertices (-1) which separate the vertices of the mesh at the specified lod value
		int strideInWorldGrid(int lod) { return gridStepInHeightmap(lod); }
		int gridStepInHeightmap(int lod)
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			//int ret = heightmapResolution() / (numChunksPerHeightmapSide(lod) * numVerticesPerChuckSide());
			//int ret1 = 1 << lod;
			//assert(ret == ret1);
			return (1 << lod);
		}

		// The number of World Units which separate the vertices of the mesh at the specified lod value
		float strideInWorldGridWUs(int lod) { return gridStepInHeightmapWUs(lod); }
		float gridStepInHeightmapWUs(int lod)
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (gridStepInHeightmap(lod) * m_mapManager->gridStepInWU());
		}

		float splitScale(void) { return c_splitScale; }

		bool isDebugEnabled(void) { return m_isDebugEnabled; }
		void setDebugEnabled(bool b = true);

		void setAppInError(int errorCode, String errorText)
		{
			char* m = errorText.alloc_c_string();
			_setAppInError(errorCode, m);
			godot::api->godot_free(m);
		}
		void _setAppInError(int errorCode, std::string errorText)
		{
			m_lastErrorCode = errorCode;
			m_lastErrorText = errorText;
			m_errorText.push_back(errorText);
			m_bAppInError = true;
			PLOG_ERROR << errorText;
		}
		bool getAppInError(void)
		{
			return _getAppInError();
		}
		int getAppInErrorCode(void)
		{
			int lastErrorCode = 0;
			string message;
			_getAppInError(lastErrorCode, message);
			return lastErrorCode;
		}
		String getAppInErrorMsg(void)
		{
			int lastErrorCode = 0;
			string message;
			_getAppInError(lastErrorCode, message);
			return message.c_str();
		}
		bool _getAppInError(void) { return m_bAppInError; }
		bool _getAppInError(int& lastErrorCode, std::string& lastErrorText)
		{
			lastErrorCode = m_lastErrorCode;
			lastErrorText = m_lastErrorText;
			return m_bAppInError;
		}

		GDN_TheWorld_Viewer* Viewer(bool useCache = true);

	private:
		bool m_initialized;
		bool m_isDebugEnabled;
		int m_chunkSizeShift;
		int m_heightmapResolutionShift;
		int m_numVerticesPerChuckSide;
		int m_heightmapResolution;
		int m_lodMaxDepth;
		int m_numLods;
		TheWorld_MapManager::MapManager *m_mapManager;
		bool m_bAppInError;
		int m_lastErrorCode;
		std::string m_lastErrorText;
		std::vector<std::string> m_errorText;

		// Node cache
		GDN_TheWorld_Viewer* m_viewer;
	};
}

#define THEWORLD_VIEWER_GENERIC_ERROR				1

