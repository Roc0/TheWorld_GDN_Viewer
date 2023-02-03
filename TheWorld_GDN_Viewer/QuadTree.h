#pragma once
#include <Godot.hpp>
#include <Array.hpp>
#include <Node.hpp>
#include <ResourceLoader.hpp>
#include <ImageTexture.hpp>
#include <Shader.hpp>
#include <ShaderMaterial.hpp>
#include <OS.hpp>

#include <map>
#include <vector>
#include <array>
#include <memory>
#include <ctime>

#include "Chunk.h"
#include "Collider.h"
//#include <Utils.h>

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
			Center = -1,

			//
			// - o
			//
			XMinus = 0,

			//
			//   o -
			//
			XPlus = 1,

			//   -
			//   o
			//
			ZMinus = 2,

			//
			//   o
			//   -
			ZPlus = 3,

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
			m_name = "Quadrant-" + std::to_string((int)(m_lowerXGridVertex / gridSizeInWU)) + "-" + std::to_string((int)(m_lowerZGridVertex / gridSizeInWU));
		}

	private:
		// ID
		float m_lowerXGridVertex;
		float m_lowerZGridVertex;
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

	public:
		Quadrant(QuadrantPos& quadrantPos, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
		{
			m_quadrantPos = quadrantPos;
			m_viewer = viewer;
			m_quadTree = quadTree;
			std::string dir = GDN_TheWorld_Globals::getClientDataDir();
			m_cache = TheWorld_Utils::MeshCacheBuffer(dir, m_quadrantPos.getGridStepInWU(), m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getLevel(), m_quadrantPos.getLowerXGridVertex(), m_quadrantPos.getLowerZGridVertex());
			m_shaderTerrainData = make_unique<ShaderTerrainData>(viewer, quadTree);
			m_collider = make_unique<Collider>(quadTree);
			m_heigthsUpdated = false;
			m_colorsUpdated = false;
		}

		~Quadrant()
		{
			//TheWorld_Viewer_Utils::TimerMs clock;
			//clock.tick();
			//clock.tock();
			//godot::GDN_TheWorld_Globals::s_elapsed1 += clock.duration().count();
		}

		_declspec(dllexport) void populateGridVertices(float initialViewerPosX, float initialViewerPosZ, bool setCamera, float cameraDistanceFromTerrain);

		PoolRealArray& getHeights(void)
		{
			return m_heigths;
		}

		QuadrantPos getPos(void)
		{
			return m_quadrantPos;
		}

		void setMeshId(std::string meshId)
		{
			m_meshId = meshId;
		}

		void refreshGridVertices(std::string buffer, std::string meshId, std::string& meshIdFromBuffer, bool updateCache);

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

		size_t getIndexFromHeighmap(float posX, float posZ, size_t level);
		float getAltitudeFromHeigthmap(size_t index);
		float getPosXFromHeigthmap(size_t index);
		float getPosZFromHeigthmap(size_t index);

		TheWorld_Utils::MemoryBuffer& getFloat16HeightsBuffer(void)
		{
			return m_float16HeigthsBuffer;
		}

		//TheWorld_Utils::MemoryBuffer& getFloat32HeightsBuffer(void)
		//{
		//	return m_float32HeigthsBuffer;
		//}

		TheWorld_Utils::MemoryBuffer& getNormalsBuffer(void)
		{
			return m_normalsBuffer;
		}

		TerrainEdit* getTerrainEdit()
		{
			if (m_terrainEdit == nullptr)
				m_terrainEdit = make_unique<TerrainEdit>();
			return m_terrainEdit.get();
		}

		void setHeightsUpdated(bool b)
		{
			m_heigthsUpdated = b;
		}
		bool heightsUpdated(void)
		{
			return m_heigthsUpdated;
		}
		void setColorsUpdated(bool b)
		{
			m_colorsUpdated = b;
		}
		bool colorsUpdated(void)
		{
			return m_colorsUpdated;
		}

	private:
		TheWorld_Utils::MeshCacheBuffer& getMeshCacheBuffer(void);

	private:
		QuadrantPos m_quadrantPos;
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		TheWorld_Utils::MemoryBuffer m_float16HeigthsBuffer;	// each height is expressed as a 16-bit float (image with FORMAT_RH) and are serialized line by line (each line from x=0 to x=numVertexPerQuadrant, first line ==> z=0, last line z=numVertexPerQuadrant)
		TheWorld_Utils::MemoryBuffer m_float32HeigthsBuffer;	// each height is expressed as a 32-bit and are serialized as above
		TheWorld_Utils::MemoryBuffer m_normalsBuffer;			// each normal is expressed as a three bytes color (r=normal x, g=normal z, b=normal y) and are serialized in the same order as heigths
		bool m_heigthsUpdated;
		bool m_colorsUpdated;
		std::unique_ptr<TerrainEdit> m_terrainEdit;
		godot::PoolRealArray m_heigths;
		std::string m_meshId;
		TheWorld_Utils::MeshCacheBuffer m_cache;
		godot::AABB m_globalCoordAABB;
		std::unique_ptr<Collider> m_collider;
		std::unique_ptr<ShaderTerrainData> m_shaderTerrainData;
	};

	class ShaderTerrainData
	{

		// Shader Params
#define SHADER_PARAM_TERRAIN_HEIGHTMAP "u_terrain_heightmap"
#define SHADER_PARAM_TERRAIN_NORMALMAP "u_terrain_normalmap"
#define SHADER_PARAM_TERRAIN_COLORMAP "u_terrain_colormap"
#define SHADER_PARAM_INVERSE_TRANSFORM "u_terrain_inverse_transform"
#define SHADER_PARAM_NORMAL_BASIS "u_terrain_normal_basis"
#define SHADER_PARAM_GRID_STEP "u_grid_step_in_wu"

	public:
		ShaderTerrainData(GDN_TheWorld_Viewer* viewer, QuadTree* quadTree);
		~ShaderTerrainData();
		void init(void);
		bool materialParamsNeedReset(void)
		{
			return m_materialParamsNeedReset;
		}
		void materialParamsNeedReset(bool b)
		{
			m_materialParamsNeedReset = b;
		}
		void resetMaterialParams(void);
		bool materialParamsNeedUpdate(void)
		{
			return m_materialParamsNeedUpdate;
		}
		void materialParamsNeedUpdate(bool b)
		{
			m_materialParamsNeedUpdate = b;
		}
		void updateMaterialParams(void);
		Ref<Material> getMaterial(void)
		{
			return m_material;
		};

	private:
		//void debugPrintTexture(std::string tex_name, Ref<Texture> tex);
		//Color encodeNormal(Vector3 normal);

	private:
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		Ref<ShaderMaterial> m_material;
		bool m_materialParamsNeedUpdate;
		bool m_materialParamsNeedReset;

		Ref<Image> m_heightMapImage;
		Ref<Texture> m_heightMapTexture;
		bool m_heightMapTexModified;

		Ref<Image> m_normalMapImage;
		Ref<Texture> m_normalMapTexture;
		bool m_normalMapTexModified;

		//Ref<Image> m_splat1MapImage;
		//Ref<Texture> m_splat1MapTexture;
		//bool m_splat1MapTexModified;

		Ref<Image> m_colorMapImage;
		Ref<Texture> m_colorMapTexture;
		bool m_colorMapTexModified;
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
		uninitialized = 0,
		getVerticesInProgress = 1,
		initialized = 2,
		toErase = 3
	};

	class QuadTree
	{
	public:
		QuadTree(GDN_TheWorld_Viewer* viewer, QuadrantPos quadrantId);
		~QuadTree();

		void onGlobalTransformChanged(void);

		void init(float viewerPosX, float viewerPosZ, bool setCamera = false, float cameraDistanceFromTerrain = 0.00);
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
		bool statusGetVerticesInProgress(void)
		{
			return status() == QuadrantStatus::getVerticesInProgress;
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
			if (m_status == QuadrantStatus::getVerticesInProgress)
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
		}
		std::string getTag(void)
		{
			return m_tag;
		}
		Transform getInternalGlobalTransform(void);
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

		void setEditModeSel(bool b)
		{
			m_editModeSel = b;
		}
		bool editModeSel(void)
		{
			return m_editModeSel;
		}

	private:
		void internalUpdate(Vector3 cameraPosGlobalCoord, Quad* quadTreeNode, enum class UpdateStage updateStage, int& numSplitRequired);

	private:
		enum class QuadrantStatus m_status;
		std::recursive_mutex m_mtxQuadrant;
		TheWorld_Viewer_Utils::MsTimePoint m_lastStatusChange;
		bool m_isVisible;
		string m_tag;
		std::unique_ptr<Quad> m_root;
		GDN_TheWorld_Viewer* m_viewer;
		Chunk::MapChunk m_mapChunk;
		std::vector<Chunk*> m_vectChunkUpdate;
		Quadrant* m_worldQuadrant;
		std::timespec m_refreshTime;
		GDN_TheWorld_Quadrant* m_GDN_Quadrant;
		bool m_editModeSel;

		// Statistics
		int m_numSplits;
		int m_numJoins;
		int m_numLeaf;
	};
}
