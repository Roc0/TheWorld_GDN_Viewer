#pragma once

#include <map>
#include <vector>
#include <array>
#include <memory>
#include <ctime>

#include "Chunk.h"
#include "Collider.h"
//#include <Utils.h>
#include "Tools.h"

#pragma warning(push, 0)
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#pragma warning(pop)

#define _DEBUG_AAB	1

namespace godot
{
	class GDN_TheWorld_Viewer;
	class GDN_TheWorld_Quadrant;
	class QuadTree;
	class TerrainEdit;

	class QuadrantPos
	{
	public:
		enum class DirectionSlot
		{
			Center = -1

			//
			// - o
			//
			, XMinus = 0
			, WestXMinus = 0

			//
			//   o -
			//
			, XPlus = 1
			, EastXPlus = 1

			//   -
			//   o
			//
			, ZMinus = 2
			, NorthZMinus = 2

			//
			//   o
			//   -
			, ZPlus = 3
			, SouthZPlus = 3

			// - 
			//   o
			//   
			, ZMinusXMinus = 4
			, NorthwestZMinusXMinus = 4

			//     - 
			//   o
			//   
			, ZMinusXPlus = 5
			, NortheastZMinusXPlus = 5

			//     
			//   o
			// -    
			, ZPlusXMinus = 6
			, SouthwestZPlusXMinus = 6

			//      
			//   o
			//     -
			, ZPlusXPlus = 7
			, SoutheastZPlusXPlus = 7
		};

		QuadrantPos()
		{
			reset();
			//m_lowerXGridVertex = m_lowerZGridVertex = m_gridStepInWU = 0;
			//m_numVerticesPerSize = m_level = 0;
			//m_sizeInWU = 0;
			//m_initialized = false;
		}

		void reset(void)
		{
			m_lowerXGridVertex = m_lowerZGridVertex = m_gridStepInWU = 0;
			m_numVerticesPerSize = m_level = 0;
			m_sizeInWU = 0;
			m_tag.clear();
			m_name.clear();
			m_initialized = false;
		}

		bool empty(void)
		{
			return m_initialized == false;
		}

		QuadrantPos(const QuadrantPos& quadrantPos)
		{
			*this = quadrantPos;
			//m_lowerXGridVertex = quadrantPos.m_lowerXGridVertex;
			//m_lowerZGridVertex = quadrantPos.m_lowerZGridVertex;
			//m_numVerticesPerSize = quadrantPos.m_numVerticesPerSize;
			//m_level = quadrantPos.m_level;
			//m_gridStepInWU = quadrantPos.m_gridStepInWU;
			//m_sizeInWU = quadrantPos.m_sizeInWU;
			//m_tag = quadrantPos.m_tag;
			//m_name = quadrantPos.m_name;
			//m_initialized = true;
		}

		// x, z pos a point contained in the quadrant in global coord: calculating starting point of the quadrant in global coord
		QuadrantPos(float x, float z, int level, int numVerticesPerSize, float gridStepInWU)
		{
			float gridSizeInWU = (numVerticesPerSize - 1) * gridStepInWU;
			//float f = x / gridSizeInWU;
			//f = floor(f);
			//f = f * gridSizeInWU;
			m_lowerXGridVertex = floor(x / gridSizeInWU) * gridSizeInWU;
			m_lowerZGridVertex = floor(z / gridSizeInWU) * gridSizeInWU;
			m_numVerticesPerSize = numVerticesPerSize;
			m_level = level;
			m_gridStepInWU = gridStepInWU;
			m_sizeInWU = (m_numVerticesPerSize - 1) * m_gridStepInWU;
			m_initialized = true;
			resetName();
		}

		bool operator<(const QuadrantPos& quadrantPos) const
		{
			assert(m_initialized == true);
			assert(quadrantPos.m_initialized == true);
			assert(m_level == quadrantPos.m_level);
			if (m_level < quadrantPos.m_level)
				return true;
			if (m_level > quadrantPos.m_level)
				return false;
			// m_level == quadrantPos.m_level

			assert(m_sizeInWU == quadrantPos.m_sizeInWU);
			if (m_sizeInWU < quadrantPos.m_sizeInWU)
				return true;
			if (m_sizeInWU > quadrantPos.m_sizeInWU)
				return false;
			// m_sizeInWU == quadrantPos.m_sizeInWU

			if (m_lowerZGridVertex < quadrantPos.m_lowerZGridVertex)
				return true;
			if (m_lowerZGridVertex > quadrantPos.m_lowerZGridVertex)
				return false;
			// m_lowerZGridVertex == quadrantPos.m_lowerZGridVertex

			return m_lowerXGridVertex < quadrantPos.m_lowerXGridVertex;
		}

		bool operator==(const QuadrantPos& quadrantPos) const
		{
			if (m_lowerXGridVertex == quadrantPos.m_lowerXGridVertex
				&& m_lowerZGridVertex == quadrantPos.m_lowerZGridVertex
				&& m_numVerticesPerSize == quadrantPos.m_numVerticesPerSize
				&& m_level == quadrantPos.m_level
				&& m_gridStepInWU == quadrantPos.m_gridStepInWU)
				return true;
			else
				return false;
		}

		void operator=(const QuadrantPos& quadrantPos)
		{
			m_lowerXGridVertex = quadrantPos.m_lowerXGridVertex;
			m_lowerZGridVertex = quadrantPos.m_lowerZGridVertex;
			m_numVerticesPerSize = quadrantPos.m_numVerticesPerSize;
			m_level = quadrantPos.m_level;
			m_gridStepInWU = quadrantPos.m_gridStepInWU;
			m_sizeInWU = quadrantPos.m_sizeInWU;
			m_tag = quadrantPos.m_tag;
			m_name = quadrantPos.m_name;
			//m_initialized = true;
			m_initialized = quadrantPos.m_initialized;
			//return *this;
		}

		std::string getIdStr(void)
		{
			return "ST" + to_string(m_gridStepInWU) + "_SZ" + to_string(m_numVerticesPerSize) + "_L" + to_string(m_level) + "_X" + to_string(m_lowerXGridVertex) + "_Z" + to_string(m_lowerZGridVertex);
		}

		float getLowerXGridVertex()
		{
			return m_lowerXGridVertex; 
		};
		float getLowerZGridVertex() 
		{
			return m_lowerZGridVertex; 
		};
		int getNumVerticesPerSize()
		{
			return m_numVerticesPerSize;
		};
		int getLevel()
		{
			return m_level;
		};
		float getGridStepInWU() 
		{
			return m_gridStepInWU; 
		};
		float getSizeInWU()
		{
			return m_sizeInWU; 
		};
		_declspec(dllexport) QuadrantPos getQuadrantPos(enum class DirectionSlot dir, int numSlot = 1);
		void setTag(std::string tag)
		{
			m_tag = tag;
		}
		std::string getTag(void)
		{
			return m_tag;
		}
		_declspec(dllexport) size_t distanceInPerimeter(QuadrantPos& q);
		bool isInitialized(void)
		{
			return m_initialized;
		}

		std::string getName(void)
		{
			return m_name;
		}

	private:
		void resetName(void)
		{
			float gridSizeInWU = (m_numVerticesPerSize - 1) * m_gridStepInWU;
			//m_name = "Quadrant-" + std::to_string((int)(m_lowerXGridVertex / gridSizeInWU)) + "-" + std::to_string((int)(m_lowerZGridVertex / gridSizeInWU));
			m_name = "Quadrant " + std::to_string((int)(m_lowerXGridVertex / gridSizeInWU)) + "," + std::to_string((int)(m_lowerZGridVertex / gridSizeInWU));
		}

	private:
		// ID
		float m_lowerXGridVertex;	// starting point in the X axis of the quadrant in global coord
		float m_lowerZGridVertex;	// starting point in the Z axis of the quadrant in global coord
		int m_numVerticesPerSize;
		int m_level;
		float m_gridStepInWU;
		// ID

		bool m_initialized;

		float m_sizeInWU;
		std::string m_tag;
		std::string m_name;
	};

	class Quadrant
	{
		friend class ShaderTerrainData;
		friend class GDN_TheWorld_Viewer;
		friend class GDN_TheWorld_Edit;
		friend class QuadTree;

	public:
		Quadrant(QuadrantPos& quadrantPos, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree);

		~Quadrant();

		_declspec(dllexport) void populateGridVertices(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced);

		PackedFloat32Array& getHeightsForCollider(bool reload = true)
		{
			if (m_heightsForCollider.size() == 0 && reload)
			{
				int numVertices = getPos().getNumVerticesPerSize() * getPos().getNumVerticesPerSize();
				m_heightsForCollider.resize((int)numVertices);
				memcpy((char*)m_heightsForCollider.ptrw(), getFloat32HeightsBuffer().ptr(), getFloat32HeightsBuffer().size());
			}
			return m_heightsForCollider;
		}

		QuadrantPos getPos(void)
		{
			return m_quadrantPos;
		}

		void setMeshId(std::string meshId)
		{
			m_meshId = meshId;
		}

		void refreshGridVerticesFromServer(std::string& buffer, std::string meshId, std::string& meshIdFromBuffer, bool updateCache);

		AABB& getGlobalCoordAABB(void)
		{
			return m_globalCoordAABB;
		}

		ShaderTerrainData* getShaderTerrainData(void)
		{
			return m_shaderTerrainData.get();
		}

		Collider* getCollider(void)
		{
			return m_collider.get();
		}

		size_t getIndexFromMap(float posX, float posZ);
		float getAltitudeFromHeigthmap(size_t index);
		float getPosXFromHeigthmap(size_t index);
		float getPosZFromHeigthmap(size_t index);
		godot::Vector3 getNormalFromNormalmap(size_t index, bool& ok);

		TheWorld_Utils::MemoryBuffer& getFloat16HeightsBuffer(bool reloadFromCache = true)
		{
			if (m_float16HeigthsBuffer.empty() && reloadFromCache)
			{
				TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
				float minAltitude, maxAltitude;
				bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
			}
			return m_float16HeigthsBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getFloat32HeightsBuffer(bool reloadFromCache = true)
		{
			if (m_float32HeigthsBuffer.empty() && reloadFromCache)
			{
				TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
				float minAltitude, maxAltitude;
				bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
			}
			return m_float32HeigthsBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getNormalsBuffer(bool reloadFromCache = true, bool force = false)
		{
			if (m_normalsBuffer.empty() && reloadFromCache)
			{
				if (!getTerrainEdit()->normalsNeedRegen || force)
				{
					TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
					float minAltitude, maxAltitude;
					bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
				}
			}
			return m_normalsBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getSplatmapBuffer(bool reloadFromCache = true)
		{
			if (m_splatmapBuffer.empty() && reloadFromCache)
			{
				if (!getTerrainEdit()->extraValues.splatmapNeedRegen)
				{
					TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
					float minAltitude, maxAltitude;
					bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
				}
			}
			return m_splatmapBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getColormapBuffer(bool reloadFromCache = true)
		{
			if (m_colormapBuffer.empty() && reloadFromCache)
			{
				if (!getTerrainEdit()->extraValues.emptyColormap)
				{
					TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
					float minAltitude, maxAltitude;
					bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
				}
			}
			return m_colormapBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getGlobalmapBuffer(bool reloadFromCache = true)
		{
			if (m_globalmapBuffer.empty() && reloadFromCache)
			{
				if (!getTerrainEdit()->extraValues.emptyGlobalmap)
				{
					TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
					float minAltitude, maxAltitude;
					bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
				}
			}
			return m_globalmapBuffer;
		}

		void fillQuadrantData(TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData& quadrantData)
		{
			quadrantData.meshId = getMeshCacheBuffer().getMeshId();
			quadrantData.terrainEdit = getTerrainEdit();
			quadrantData.minHeight = getTerrainEdit()->minHeight;
			quadrantData.maxHeight = getTerrainEdit()->maxHeight;
			TheWorld_Utils::MemoryBuffer& heights16Buffer = getFloat16HeightsBuffer();
			quadrantData.heights16Buffer = &heights16Buffer;
			TheWorld_Utils::MemoryBuffer& heights32Buffer = getFloat32HeightsBuffer();
			quadrantData.heights32Buffer = &heights32Buffer;
			TheWorld_Utils::MemoryBuffer& normalsBuffer = getNormalsBuffer();
			quadrantData.normalsBuffer = &normalsBuffer;
			TheWorld_Utils::MemoryBuffer& splatmapBuffer = getSplatmapBuffer();
			quadrantData.splatmapBuffer = &splatmapBuffer;
			TheWorld_Utils::MemoryBuffer& colormapBuffer = getColormapBuffer();
			quadrantData.colormapBuffer = &colormapBuffer;
			TheWorld_Utils::MemoryBuffer& globalmapBuffer = getGlobalmapBuffer();
			quadrantData.globalmapBuffer = &globalmapBuffer;
			quadrantData.heightsUpdated = heightsUpdated();
			quadrantData.normalsUpdated = normalsUpdated();
			quadrantData.texturesUpdated = textureUpdated();
		}

		void resetHeightsBuffer(void)
		{
			getFloat16HeightsBuffer(false).clear();
			getFloat32HeightsBuffer(false).clear();
			setEmpty(true);
			setHeightsUpdated(true);

			resetNormalsBuffer();
			resetColormapBuffer();
		}

		void resetNormalsBuffer(void)
		{
			getNormalsBuffer(false).clear();
			getTerrainEdit()->normalsNeedRegen = true;
			setNormalsUpdated(true);

			resetSplatmapBuffer();
		}

		void resetSplatmapBuffer(void)
		{
			getSplatmapBuffer(false).clear();
			getTerrainEdit()->extraValues.splatmapNeedRegen = true;
			setSplatmapUpdated(true);
			
			resetGlobalmapBuffer();
		}

		void resetColormapBuffer(void)
		{
			getColormapBuffer(false).clear();
			getTerrainEdit()->extraValues.emptyColormap = true;
			setColorsUpdated(true);
		}

		void resetGlobalmapBuffer(void)
		{
			getGlobalmapBuffer(false).clear();
			getTerrainEdit()->extraValues.emptyGlobalmap = true;
			setGlobalmapUpdated(true);
		}

		TheWorld_Utils::TerrainEdit* getTerrainEdit()
		{
			if (m_terrainEdit == nullptr)
			{
				//m_terrainEdit = make_unique<TheWorld_Utils::TerrainEdit>(TheWorld_Utils::TerrainEdit::TerrainType::campaign_1);
				m_terrainEdit = make_unique<TheWorld_Utils::TerrainEdit>();
			}
			return m_terrainEdit.get();
		}

		void setNeedUploadToServer(bool b)
		{
			m_needUploadToServer = b;
		}

		bool needUploadToServer(void)
		{
			return m_needUploadToServer;
		}

		void lockInternalData(void)
		{
			m_internalDataLocked = true;
		}

		void unlockInternalData(void)
		{
			m_internalDataLocked = false;
		}

		bool internalDataLocked()
		{
			return m_internalDataLocked;
		}

		void setHeightsUpdated(bool b)
		{
			m_heightsUpdated = b;
			if (b)
			{
				m_splatmapUpdated = true;
				m_texturesUpdated = true;
			}
		}
		bool heightsUpdated(void)
		{
			return m_heightsUpdated;
		}
		void setNormalsUpdated(bool b)
		{
			m_normalsUpdated = b;
		}
		bool normalsUpdated(void)
		{
			return m_normalsUpdated;
		}
		void setSplatmapUpdated(bool b)
		{
			m_splatmapUpdated = b;
		}
		bool splatmapUpdated(void)
		{
			return m_splatmapUpdated;
		}
		void setColorsUpdated(bool b)
		{
			m_colorsUpdated = b;
		}
		bool colorsUpdated(void)
		{
			return m_colorsUpdated;
		}
		void setGlobalmapUpdated(bool b)
		{
			m_globalmapUpdated = b;
		}
		bool globalmapUpdated(void)
		{
			return m_globalmapUpdated;
		}
		void setTextureUpdated(bool b)
		{
			m_texturesUpdated = b;
		}
		bool textureUpdated(void)
		{
			return m_texturesUpdated;
		}
		void setEmpty(bool b)
		{
			m_empty = b;
		}
		bool empty(void)
		{
			return m_empty;
		}

		void releaseTerrainValuesMemory(bool visible)
		{
			if (needUploadToServer() || internalDataLocked())
				return;
			
			if (!visible)
				m_float16HeigthsBuffer.clear();	// needed to calc aabb of chunks during split/join (Quadrant::getAltitudeFromHeigthmap): it is kept for performance reason to keep it for visible quadrants and avoid to reload from disk for new chunks
			m_float32HeigthsBuffer.clear();
			m_normalsBuffer.clear();
			m_splatmapBuffer.clear();
			m_colormapBuffer.clear();
			m_globalmapBuffer.clear();
			m_heightsForCollider.resize(0);
		}

		size_t calcMemoryOccupation(void)
		{
			size_t occupation = m_float16HeigthsBuffer.reserved() + m_float32HeigthsBuffer.reserved() + m_normalsBuffer.reserved() + m_splatmapBuffer.reserved() + m_colormapBuffer.reserved() + m_globalmapBuffer.reserved() + (m_terrainEdit == nullptr ? 0 : m_terrainEdit->size) + m_heightsForCollider.size() * sizeof(real_t);
			return occupation;
		}

	private:
		TheWorld_Utils::MeshCacheBuffer& getMeshCacheBuffer(void);

	private:
		QuadrantPos m_quadrantPos;
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		TheWorld_Utils::MeshCacheBuffer m_cache;
		godot::AABB m_globalCoordAABB;
		std::unique_ptr<Collider> m_collider;
		std::unique_ptr<ShaderTerrainData> m_shaderTerrainData;
		bool m_needUploadToServer;
		bool m_internalDataLocked;
		bool m_empty;

		// Terrain Values
		std::string m_meshId;
		TheWorld_Utils::MemoryBuffer m_float16HeigthsBuffer;	// each height is expressed as a 16-bit float (image with FORMAT_RH) and are serialized line by line (each line from x=0 to x=numVertexPerQuadrant, first line ==> z=0, last line z=numVertexPerQuadrant)
		TheWorld_Utils::MemoryBuffer m_float32HeigthsBuffer;	// each height is expressed as a 32-bit and are serialized as above
		TheWorld_Utils::MemoryBuffer m_normalsBuffer;			// each normal is expressed as a three bytes color (r=normal x, g=normal z, b=normal y) and are serialized in the same order as heights
		TheWorld_Utils::MemoryBuffer m_splatmapBuffer;			// for each vertex there is a four bytes color (r=lowElevationTex %, g=dirtTex %, b=highElevationTex %, a=slope %) and are serialized in the same order as heights
		TheWorld_Utils::MemoryBuffer m_colormapBuffer;			// Vertices color
		TheWorld_Utils::MemoryBuffer m_globalmapBuffer;
		std::unique_ptr<TheWorld_Utils::TerrainEdit> m_terrainEdit;
		godot::PackedFloat32Array m_heightsForCollider;
		// Terrain Values

		bool m_heightsUpdated;
		bool m_normalsUpdated;
		bool m_splatmapUpdated;
		bool m_colorsUpdated;
		bool m_globalmapUpdated;
		bool m_texturesUpdated;
		//bool m_terrainValuesCanBeCleared;
	};

	class ShaderTerrainData
	{
	public:
		class GroundTexture
		{
		public:
			GroundTexture()
			{
				//m_albedo_bump_tex = nullptr;
				//m_normal_roughness_tex = nullptr;
				initialized = false;
			}
		
		public:
			bool initialized;
			Ref<ImageTexture> m_albedo_bump_tex;
			Ref<ImageTexture> m_normal_roughness_tex;
			Ref<Texture2D> m_tex;
		};

// Shader Params
#define SHADER_PARAM_TERRAIN_HEIGHTMAP	"u_terrain_heightmap"
#define SHADER_PARAM_TERRAIN_NORMALMAP	"u_terrain_normalmap"
#define SHADER_PARAM_TERRAIN_SPLATMAP	"u_terrain_splatmap"
#define SHADER_PARAM_TERRAIN_COLORMAP	"u_terrain_colormap"
#define SHADER_PARAM_TERRAIN_GLOBALMAP	"u_terrain_globalmap"

#define SHADER_PARAM_INVERSE_TRANSFORM	"u_terrain_inverse_transform"
#define SHADER_PARAM_NORMAL_BASIS		"u_terrain_normal_basis"
#define SHADER_PARAM_GRID_STEP			"u_grid_step"

#define SHADER_PARAM_EDITMODE_SELECTED	"u_editmode_selected"
#define SHADER_PARAM_MOUSE_HIT			"u_mouse_hit"
#define SHADER_PARAM_MOUSE_HIT_RADIUS	"u_mouse_hit_radius"
#define SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS	"u_quad_border_line_thickness"

#define SHADER_PARAM_VERTICES_PER_CHUNK	"u_num_vertices_per_chunk"
#define SHADER_PARAM_LOOKDEV_LOD		"u_lod"
#define	SHADER_PARAM_LOD_GRID_STEP		"u_lod_grid_step"
#define	SHADER_PARAM_LOD_CHUNK_SIZE		"u_lod_chunk_size"

#define SHADER_PARAM_GROUND_ALBEDO_BUMP_0		"u_ground_albedo_bump_0"
#define SHADER_PARAM_GROUND_ALBEDO_BUMP_1		"u_ground_albedo_bump_1"
#define SHADER_PARAM_GROUND_ALBEDO_BUMP_2		"u_ground_albedo_bump_2"
#define SHADER_PARAM_GROUND_ALBEDO_BUMP_3		"u_ground_albedo_bump_3"

#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_0	"u_ground_normal_roughness_0"
#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_1	"u_ground_normal_roughness_1"
#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_2	"u_ground_normal_roughness_2"
#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_3	"u_ground_normal_roughness_3"

#define SHADER_PARAM_GROUND_UV_SCALE_PER_TEXTURE	"u_ground_uv_scale_per_texture"
#define SHADER_PARAM_DEPTH_BLENDING					"u_depth_blending"
#define SHADER_PARAM_TRIPLANAR						"u_triplanar"
#define SHADER_PARAM_TILE_REDUCTION_PER_TEXTURE		"u_tile_reduction_per_texture"
#define SHADER_PARAM_GLOBALMAP_BLEND_START			"u_globalmap_blend_start"
#define SHADER_PARAM_GLOBALMAP_BLEND_DISTANCE		"u_globalmap_blend_distance"
#define SHADER_PARAM_COLORMAP_OPACITY_PER_TEXTURE	"u_colormap_opacity_per_texture"

#define SHADER_PARAM_LOOKDEV_MAP		"u_map"

#define SHADER_PARAM_SLOPE_VERTICAL_FACTOR	"u_slope_vertical_factor"
#define SHADER_PARAM_SLOPE_FLAT_FACTOR		"u_slope_flat_factor"
#define SHADER_PARAM_DIRT_ON_ROCKS_FACTOR	"u_dirt_on_rocks_factor"
#define SHADER_PARAM_HIGH_ELEVATION_FACTOR	"u_high_elevation_factor"
#define SHADER_PARAM_LOW_ELEVATION_FACTOR	"u_low_elevation_factor"
#define SHADER_PARAM_MAX_HEIGHT				"u_max_heigth"
#define SHADER_PARAM_MIN_HEIGHT				"u_min_heigth"

	public:

		enum class LookDev {
			NotSet = 0
			, Heights = 1
			, Normals = 2
			, Splat = 3
			, Color = 4
			, Global = 5
			, ChunkLod = 6
			, Universe = 7
			, ShaderArt = 8
			, Lightning = 9
			, StarNest = 10
			, FlaringStar = 11
			, AnimatedDiamond = 12
			, PlasmaWaves = 13
		};

		ShaderTerrainData(GDN_TheWorld_Viewer* viewer, QuadTree* quadTree);
		~ShaderTerrainData();
		void init(void);
		static godot::Ref<godot::Image> readGroundTexture(godot::String fileName, bool& ok, GDN_TheWorld_Viewer* viewer = nullptr);
		static void getGroundTexture(godot::String fileName, GroundTexture* groundTexture, GDN_TheWorld_Viewer* viewer = nullptr);
		bool materialParamsNeedReset(void)
		{
			return m_materialParamsNeedReset;
		}
		void materialParamsNeedReset(bool b)
		{
			m_materialParamsNeedReset = b;
		}
		void resetMaterialParams(LookDev lookdev);
		bool materialParamsNeedUpdate(void)
		{
			return m_materialParamsNeedUpdate;
		}
		void materialParamsNeedUpdate(bool b)
		{
			m_materialParamsNeedUpdate = b;
		}
		void updateMaterialParams(LookDev lookdev);
		Ref<Material> getRegularMaterial(bool reload = false);
		Ref<Material> getLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev, bool reload = false);
		std::map<int, godot::Ref<godot::ShaderMaterial>>& getLookDevChunkLodMaterials(void);
		godot::Ref<godot::Material> getLookDevChunkLodMaterial(int lod, bool reload = false);
		static void freeLookDevSubViewport(void);
		static void releaseGlobals(void);
		static void getGroundTextures(godot::String dirName, GDN_TheWorld_Viewer* viewer = nullptr);
		static std::map<std::string, std::unique_ptr<GroundTexture>>& getGroundTextures(void)
		{
			return s_groundTextures;
		}
		static bool groundTexturesNeedProcessing(void)
		{
			return s_groundTexturesNeedProcessing;
		}
		static void groundTexturesNeedProcessing(bool b)
		{
			s_groundTexturesNeedProcessing = b;
		}
		void mouseHitChanged(godot::Vector2 mouseHit);

	private:
		//void debugPrintTexture(std::string tex_name, Ref<Texture> tex);
		//Color encodeNormal(Vector3 normal);

	private:
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		godot::Ref<godot::ShaderMaterial> m_regularMaterial;
		godot::Ref<godot::ShaderMaterial> m_lookDevMaterial;
		//static godot::SubViewport* g_lookDevSubviewport;
		static GDN_TheWorld_ShaderTexture* g_lookDevShaderTexture;
		std::map<int, godot::Ref<godot::ShaderMaterial>> m_lookDevChunkLodMaterials;
		bool m_materialParamsNeedUpdate;
		bool m_materialParamsNeedReset;

		Ref<Texture> m_heightMapTexture;
		bool m_heightMapTexModified;

		Ref<Texture> m_normalMapTexture;
		bool m_normalMapTexModified;

		Ref<Texture> m_splatMapTexture;
		bool m_splatMapTexModified;

		Ref<Texture> m_colorMapTexture;
		bool m_colorMapTexModified;

		Ref<Texture> m_globalMapTexture;
		bool m_globalMapTexModified;

		static std::map<std::string, std::unique_ptr<GroundTexture>> s_groundTextures;
		static bool s_groundTexturesNeedProcessing;
		//static std::recursive_mutex s_groundTexturesMutex;
	};
	
	class Quad
	{
	public:

		Quad(int slotPosX, int slotPosZ, int lod, enum PosInQuad posInQuad, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree);
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
		enum PosInQuad getPosInQuad(void)
		{
			return m_posInQuad;
		}

		bool isLeaf(void);
		Quad *getChild(int idx);
		void recycleChunk(void);
		void split(void);
		void clearChildren(void);
		void assignChunk(void);
		Chunk* getChunk(void) 
		{
			return m_chunk; 
		}
		AABB getChunkAABB(void)
		{
			AABB aabb;
			if (m_chunkAABB == aabb)
			{
				if (m_chunk != nullptr)
					m_chunkAABB = m_chunk->getAABB();
				else
					return aabb;
			}
			return m_chunkAABB;
		}
		void resetChachedChunkAABB()
		{
			m_chunkAABB = AABB();
		}
		float getChunkSizeInWUs(void)
		{
			return m_chunkSizeInWUs; 
		}
		void setCameraPos(Vector3 globalCoordCameraLastPos);
		void setSplitRequired(bool b)
		{
			if (b)
				m_splitRequired = b;
			else
				m_splitRequired = b;
		}
		bool splitRequired(void)
		{
			return m_splitRequired;
		}


	private:
		enum PosInQuad m_posInQuad;
		std::array<std::unique_ptr<Quad>, 4> m_children;
		Chunk* m_chunk;
		int m_slotPosX;	// expresses the orizzontal (X) and vertical (Z) position of the quad (and of the corrispondig chunk) in the grid of chunks
		int m_slotPosZ;	// at the current (of the quad) lod : 0 the first chunk, 1 the following to the max number of chunks on a size for the specific lod
		int m_lod;
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		AABB m_chunkAABB;
		float m_chunkSizeInWUs;
		bool m_splitRequired;
	};

	enum class QuadrantStatus
	{
		refreshTerrainDataNeeded = -2
		,refreshTerrainDataInProgress = -1
		,uninitialized = 0
		,getTerrainDataInProgress = 1
		,initialized = 2
		,toErase = 3
	};

	class QuadTree
	{
	public:
		QuadTree(GDN_TheWorld_Viewer* viewer, QuadrantPos quadrantId);
		~QuadTree();

		void onGlobalTransformChanged(void);

		void init(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced);
		void refreshTerrainData(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrain);
		enum class UpdateStage
		{
			Stage1 = 1,
			Stage2 = 2,
			Stage3 = 3
		};
		void update(Vector3 cameraPosGlobalCoord, enum class UpdateStage updateStage, int& numSplitRequired);
		Chunk* getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir);
		Chunk* getChunkAt(Chunk::ChunkPos pos);
		bool isChunkOnBorderOfQuadrant(Chunk::ChunkPos pos, QuadrantPos& XMinusQuadrantPos, QuadrantPos& XPlusQuadrantPos, QuadrantPos& ZMinusQuadrantPos, QuadrantPos& ZPlusQuadrantPos);
		void addChunk(Chunk* chunk);
		void addChunkUpdate(Chunk* chunk);
		void clearChunkUpdate(void);
		std::vector<Chunk*>& getChunkUpdate(void)
		{
			return m_vectChunkUpdate; 
		}
		void ForAllChunk(Chunk::ChunkAction& chunkAction);
		void dump(void);
		int getNumSplits(void) 
		{
			if (!isValid())
				return 0;
			if (!isVisible())
				return 0;
			return m_numSplits;
		}
		int getNumJoins(void)
		{ 
			if (!isValid())
				return 0;
			if (!isVisible())
				return 0;
			return m_numJoins;
		}
		int getNumChunks(void)
		{
			if (!isValid())
				return 0;

			if (!isVisible())
				return 0;

			int num = 0;
			//for (int i = 0; i < (int)m_mapChunk.size(); i++)
			//	num += (int)m_mapChunk[i].size();
			Chunk::MapChunk::iterator itMapChunk;
			for (Chunk::MapChunk::iterator itMapChunk = m_mapChunk.begin(); itMapChunk != m_mapChunk.end(); itMapChunk++)
				num += (int)itMapChunk->second.size();
			return num;
		}
		int getNumActiveChunks(void)
		{
			if (!isValid())
				return 0;

			if (!isVisible())
				return 0;

			int num = 0;
			Chunk::MapChunk::iterator itMapChunk;
			for (Chunk::MapChunk::iterator itMapChunk = m_mapChunk.begin(); itMapChunk != m_mapChunk.end(); itMapChunk++)
				for (Chunk::MapChunkPerLod::iterator itMapChunkPerLod = itMapChunk->second.begin(); itMapChunkPerLod != itMapChunk->second.end(); itMapChunkPerLod++)
					if (itMapChunkPerLod->second->isActive())
						num++;
			return num;
		}
		Quadrant* getQuadrant(void)
		{
			return m_worldQuadrant; 
		}
		void setLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev, bool reload = false);
		void setRegularMaterial(bool reload = false);
		Ref<Material> getCurrentMaterial(int lod);
		void reloadCurrentMaterial(void);
		bool resetMaterialParams(bool force = false);
		bool materialParamsNeedReset(void);
		void materialParamsNeedReset(bool b);
		bool updateMaterialParams(void);
		bool materialParamsNeedUpdate(void);
		void materialParamsNeedUpdate(bool b);
		bool statusInitialized(void)
		{
			return status() == QuadrantStatus::initialized;
		}
		bool statusGetTerrainDataInProgress(void)
		{
			return status() == QuadrantStatus::getTerrainDataInProgress;
		}
		bool statusRefreshTerrainDataNeeded(void)
		{
			bool b = status() == QuadrantStatus::refreshTerrainDataNeeded;
			if (b)
			{
				b = status() == QuadrantStatus::refreshTerrainDataNeeded;
			}
			return b;
		}
		bool statusRefreshTerrainDataInProgress(void)
		{
			return status() == QuadrantStatus::refreshTerrainDataInProgress;
		}
		bool statusUninitialized(void)
		{
			return status() == QuadrantStatus::uninitialized;
		}
		bool statusToErase(void)
		{
			return status() == QuadrantStatus::toErase;
		}
		enum class QuadrantStatus currentStatus(void)
		{
			return m_status;
		}
		enum class QuadrantStatus status(void)
		{
			if (m_status == QuadrantStatus::getTerrainDataInProgress)
			{
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				long long elapsedFromStatusChange = (now - m_lastStatusChange).count();
				if (elapsedFromStatusChange > (THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE * 2))
					m_status = QuadrantStatus::uninitialized;
			}
			return m_status;
		}
		void setStatus(enum class QuadrantStatus status)
		{
			m_lastStatusChange = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
			m_status = status;
		}
		bool isValid(void)
		{
			return status() == QuadrantStatus::initialized;
		}
		bool isVisible(void)
		{
			if (statusToErase())
				return false;

			return m_isVisible; 
		}
		void setVisible(bool b)
		{
			m_isVisible = b; 
		}
		void refreshTime(std::timespec& time)
		{
			m_refreshTime = time; 
		}
		std::timespec getRefreshTime(void) 
		{
			return m_refreshTime; 
		}
		void setTag(string tag)
		{
			m_tag = tag;
			m_worldQuadrant->m_quadrantPos.setTag(tag);
		}
		std::string getTag(void)
		{
			return m_tag;
		}
		Transform3D getInternalGlobalTransform(void);
		GDN_TheWorld_Viewer* Viewer(void)
		{
			return m_viewer; 
		}
		std::recursive_mutex& getQuadrantMutex(void)
		{
			return m_mtxQuadrant;
		}

		GDN_TheWorld_Quadrant* getGDNQuadrant(void)
		{
			return m_GDN_Quadrant;
		}

		Chunk::MapChunk& getMapChunk(void)
		{
			return m_mapChunk;
		}

		void setEditModeQuadrantSelected(bool b)
		{
			m_editModeQuadrantSelected = b;
		}

		bool editModeQuadrantSelected(void)
		{
			return m_editModeQuadrantSelected;
		}

		enum class ShaderTerrainData::LookDev getLookDev(void)
		{
			return m_lookDev;
		}

		void mouseHitChanged(godot::Vector3 mouseHit, bool hit);
		void getAdjacentQuadrants(std::list<QuadTree*>& adjacentQuadrantsHit);

	private:
		void internalUpdate(Vector3 cameraPosGlobalCoord, Quad* quadTreeNode, enum class UpdateStage updateStage, int& numSplitRequired);

	private:
		enum class QuadrantStatus m_status;
		std::recursive_mutex m_mtxQuadrant;
		TheWorld_Viewer_Utils::MsTimePoint m_lastStatusChange;
		bool m_isVisible;
		std::string m_tag;
		std::unique_ptr<Quad> m_root;
		GDN_TheWorld_Viewer* m_viewer;
		Chunk::MapChunk m_mapChunk;
		std::vector<Chunk*> m_vectChunkUpdate;
		Quadrant* m_worldQuadrant;
		std::timespec m_refreshTime;
		GDN_TheWorld_Quadrant* m_GDN_Quadrant;
		bool m_editModeQuadrantSelected = false;
		bool m_hitByMouse = false;
		enum class ShaderTerrainData::LookDev m_lookDev;

		// Statistics
		int m_numSplits;
		int m_numJoins;
		int m_numLeaf;
	};
}
