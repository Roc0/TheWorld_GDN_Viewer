#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>

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

		int numVerticesPerChuckSide(void) { return m_numVerticesPerChuckSide; }					// Chunk num vertices -1
		int bitmapResolution(void) { return m_bitmapResolution; }	// Resolution of the bitmap = num point of the bitmap -1;
		int lodMaxDepth(void) {	return m_lodMaxDepth; }
		int numLods(void) { return m_numLods; }
		int numChunksPerBitmapSide(int lod)
		{
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (bitmapResolution() / numVerticesPerChuckSide()) >> lod;
		}
		float gridStepInBitmapWU(int lod)
		{
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (gridStepInBitmap(lod) * m_mapManager->gridStepInWU());
		}
		float gridStepInBitmap(int lod)
		{
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (bitmapResolution() / (numChunksPerBitmapSide(lod) * (numVerticesPerChuckSide())));
		}

	private:
		int m_numVerticesPerChuckSide;
		int m_bitmapResolution;
		int m_lodMaxDepth;
		int m_numLods;
		TheWorld_MapManager::MapManager *m_mapManager;
	};

}


