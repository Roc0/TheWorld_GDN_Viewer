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

#define _DEBUG_AAB	1

namespace godot
{
	class GDN_TheWorld_Viewer;
	class QuadTree;

	class QuadrantId
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

		QuadrantId()
		{
			m_lowerXGridVertex = m_lowerZGridVertex = m_gridStepInWU = 0;
			m_numVerticesPerSize = m_level = 0;
			m_sizeInWU = 0;
			m_initialized = false;
		}

		QuadrantId(const QuadrantId& quadrantId)
		{
			m_lowerXGridVertex = quadrantId.m_lowerXGridVertex;
			m_lowerZGridVertex = quadrantId.m_lowerZGridVertex;
			m_numVerticesPerSize = quadrantId.m_numVerticesPerSize;
			m_level = quadrantId.m_level;
			m_gridStepInWU = quadrantId.m_gridStepInWU;
			m_sizeInWU = quadrantId.m_sizeInWU;
			m_tag = quadrantId.m_tag;
			m_initialized = true;
		}

		QuadrantId(float x, float z, int level, int numVerticesPerSize, float gridStepInWU)
		{
			float gridSizeInWU = (numVerticesPerSize - 1) * gridStepInWU;
			float f = x / gridSizeInWU;
			f = floor(f);
			f = f * gridSizeInWU;
			m_lowerXGridVertex = floor(x / gridSizeInWU) * gridSizeInWU;
			m_lowerZGridVertex = floor(z / gridSizeInWU) * gridSizeInWU;
			m_numVerticesPerSize = numVerticesPerSize;
			m_level = level;
			m_gridStepInWU = gridStepInWU;
			m_sizeInWU = (m_numVerticesPerSize - 1) * m_gridStepInWU;
			m_initialized = true;
		}

		bool operator<(const QuadrantId& quadrantId) const
		{
			assert(m_level == quadrantId.m_level);
			if (m_level < quadrantId.m_level)
				return true;
			if (m_level > quadrantId.m_level)
				return false;
			// m_level == quadrantId.m_level

			assert(m_sizeInWU == quadrantId.m_sizeInWU);
			if (m_sizeInWU < quadrantId.m_sizeInWU)
				return true;
			if (m_sizeInWU > quadrantId.m_sizeInWU)
				return false;
			// m_sizeInWU == quadrantId.m_sizeInWU

			if (m_lowerZGridVertex < quadrantId.m_lowerZGridVertex)
				return true;
			if (m_lowerZGridVertex > quadrantId.m_lowerZGridVertex)
				return false;
			// m_lowerZGridVertex == quadrantId.m_lowerZGridVertex

			return m_lowerXGridVertex < quadrantId.m_lowerXGridVertex;
		}

		bool operator==(const QuadrantId& quadrantId) const
		{
			if (m_lowerXGridVertex == quadrantId.m_lowerXGridVertex
				&& m_lowerZGridVertex == quadrantId.m_lowerZGridVertex
				&& m_numVerticesPerSize == quadrantId.m_numVerticesPerSize
				&& m_level == quadrantId.m_level
				&& m_gridStepInWU == quadrantId.m_gridStepInWU)
				return true;
			else
				return false;
		}

		QuadrantId operator=(const QuadrantId& quadrantId)
		{
			m_lowerXGridVertex = quadrantId.m_lowerXGridVertex;
			m_lowerZGridVertex = quadrantId.m_lowerZGridVertex;
			m_numVerticesPerSize = quadrantId.m_numVerticesPerSize;
			m_level = quadrantId.m_level;
			m_gridStepInWU = quadrantId.m_gridStepInWU;
			m_sizeInWU = quadrantId.m_sizeInWU;
			m_tag = quadrantId.m_tag;
			m_initialized = true;
			return *this;
		}

		std::string getId(void)
		{
			return "ST" + to_string(m_gridStepInWU) + "_SZ" + to_string(m_numVerticesPerSize) + "_L" + to_string(m_level) + "_X" + to_string(m_lowerXGridVertex) + "_Z" + to_string(m_lowerZGridVertex);
		}

		float getLowerXGridVertex() { return m_lowerXGridVertex; };
		float getLowerZGridVertex() { return m_lowerZGridVertex; };
		int getNumVerticesPerSize() { return m_numVerticesPerSize; };
		int getLevel() { return m_level; };
		float getGridStepInWU() { return m_gridStepInWU; };
		float getSizeInWU() { return m_sizeInWU; };
		_declspec(dllexport) QuadrantId getQuadrantId(enum class DirectionSlot dir, int numSlot = 1);
		void setTag(std::string tag) { m_tag = tag; }
		std::string getTag(void) { return m_tag; }
		_declspec(dllexport) size_t distanceInPerimeter(QuadrantId& q);
		bool isInitialized(void) { return m_initialized; }

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
	};

	class Quadrant
	{
	public:
		//Quadrant(MapManager* mapManager)
		//{
		//	m_mapManager = mapManager;
		//}

		Quadrant(QuadrantId& quadrantId, QuadTree* quadTree)
		{
			m_quadrantId = quadrantId;
			m_quadTree = quadTree;
		}

		~Quadrant()
		{
			m_vectGridVertices.clear();
		}

		void implementId(QuadrantId& quadrantId)
		{
			m_quadrantId = quadrantId;
		}

		_declspec(dllexport) void populateGridVertices(float initialViewerPosX, float initialViewerPosZ, bool setCamera, float cameraDistanceFromTerrain);

		std::vector<TheWorld_Utils::GridVertex>& getGridVertices(void)
		{
			return m_vectGridVertices;
		}

		QuadrantId getId(void) { return m_quadrantId; }

		void setMeshId(std::string meshId)
		{
			m_meshId = meshId;
		}

		TheWorld_Utils::MeshCacheBuffer getMeshCacheBuffer(void);
		//std::string getCacheFileName();

	private:
		QuadrantId m_quadrantId;
		std::vector<TheWorld_Utils::GridVertex> m_vectGridVertices;
		std::string m_meshId;
		QuadTree* m_quadTree;
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
		ShaderTerrainData();
		~ShaderTerrainData();
		void init(GDN_TheWorld_Viewer* viewer, QuadTree* quadTree);
		bool materialParamsNeedReset(void)
		{
			return m_materialParamsNeedReset;
		}
		void materialParamsNeedReset(bool b)
		{
			m_materialParamsNeedReset = b;
		}
		void resetMaterialParams(TheWorld_Utils::MeshCacheBuffer& cache);
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
		void debugPrintTexture(std::string tex_name, Ref<Texture> tex);
		Color encodeNormal(Vector3 normal);

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

		//Ref<Image> m_colorMapImage;
		//Ref<Texture> m_colorMapTexture;
		//bool m_colorMapTexModified;
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

		bool isLeaf(void);
		Quad *getChild(int idx);
		void recycleChunk(void);
		void split(void);
		void clearChildren(void);
		void assignChunk(void);
		Chunk* getChunk(void) { return m_chunk; }
		AABB getChunkAABB(void)
		{
			AABB aabb;
			if (m_chunkAABB == aabb)
			{
				m_chunkAABB = m_chunk->getAABB();
			}
			return m_chunkAABB;
		}
		float getChunkSizeInWUs(void) { return m_chunkSizeInWUs; }
		void setCameraPos(Vector3 globalCoordCameraLastPos);

	private:
		enum PosInQuad m_posInQuad;
		std::array<std::unique_ptr<Quad>, 4> m_children;
		Chunk* m_chunk;
		int m_slotPosX;	// express the orizzontal (X) and vertical (Z) position of the quad (and of the corrispondig chunk) in the grid of chunks
		int m_slotPosZ;	// at the current (of the quad) lod : 0 the first chunk, 1 the following to the max number of chunks on a size for the specific lod
		int m_lod;
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		AABB m_chunkAABB;
		float m_chunkSizeInWUs;
	};

	enum class QuadrantStatus
	{
		uninitialized = 0,
		getVerticesInProgress = 1,
		initialized = 2
	};

	class QuadTree
	{
	public:
		QuadTree(GDN_TheWorld_Viewer* viewer, QuadrantId quadrantId);
		~QuadTree();

		void init(float viewerPosX, float viewerPosZ, bool setCamera = false, float cameraDistanceFromTerrain = 0.00);
		
		void update(Vector3 cameraPosGlobalCoord);
		Chunk* getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir);
		Chunk* getChunkAt(Chunk::ChunkPos pos);
		void addChunk(Chunk* chunk);
		void addChunkUpdate(Chunk* chunk);
		void clearChunkUpdate(void);
		std::vector<Chunk*>& getChunkUpdate(void) { return m_vectChunkUpdate; }
		void ForAllChunk(Chunk::ChunkAction& chunkAction);
		void dump(void);
		int getNumSplits(void) {
			if (!isValid())
				return 0;
			if (!isVisible())
				return 0;
			return m_numSplits;
		}
		int getNumJoins(void) { 
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
		Quadrant* getQuadrant(void) { return m_worldQuadrant; }
		void resetMaterialParams(void);
		bool materialParamsNeedReset(void);
		void materialParamsNeedReset(bool b, TheWorld_Utils::MeshCacheBuffer* cache);
		void updateMaterialParams(void);
		bool materialParamsNeedUpdate(void);
		void materialParamsNeedUpdate(bool b);
		ShaderTerrainData& getShaderTerrainData(void)
		{
			return m_shaderTerrainData;
		}
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
		enum class QuadrantStatus currentStatus(void)
		{
			return m_status;
		}
		enum class QuadrantStatus status(void)
		{
			if (m_status == QuadrantStatus::getVerticesInProgress)
			{
				TheWorld_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				long long elapsedFromStatusChange = (now - m_lastStatusChange).count();
				if (elapsedFromStatusChange > (THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE * 2))
					m_status = QuadrantStatus::uninitialized;
			}
			return m_status;
		}
		void setStatus(enum class QuadrantStatus status)
		{
			m_lastStatusChange = std::chrono::time_point_cast<TheWorld_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
			m_status = status;
		}
		bool isValid(void)
		{
			return status() == QuadrantStatus::initialized;
		}
		bool isVisible(void) { return m_isVisible; }
		void setVisible(bool b) { m_isVisible = b; }
		void refreshTime(std::timespec& time) { m_refreshTime = time; }
		std::timespec getRefreshTime(void) { return m_refreshTime; }
		void setTag(string tag)
		{
			m_tag = tag;
		}
		std::string getTag(void)
		{
			return m_tag;
		}
		Transform internalTransformGlobalCoord(void);
		GDN_TheWorld_Viewer* Viewer(void) { return m_viewer; }
		std::recursive_mutex& getQuadrantMutex(void)
		{
			return m_mtxQuadrant;
		}

	private:
		void internalUpdate(Vector3 cameraPosGlobalCoord, Quad* quadTreeNode);

	private:
		enum class QuadrantStatus m_status;
		std::recursive_mutex m_mtxQuadrant;
		TheWorld_Utils::MsTimePoint m_lastStatusChange;
		bool m_isVisible;
		string m_tag;
		std::unique_ptr<Quad> m_root;
		GDN_TheWorld_Viewer* m_viewer;
		Chunk::MapChunk m_mapChunk;
		std::vector<Chunk*> m_vectChunkUpdate;
		Quadrant* m_worldQuadrant;
		ShaderTerrainData m_shaderTerrainData;
		std::timespec m_refreshTime;
		TheWorld_Utils::MeshCacheBuffer m_cache;

		// Statistics
		int m_numSplits;
		int m_numJoins;
		int m_numLeaf;
	};
}
