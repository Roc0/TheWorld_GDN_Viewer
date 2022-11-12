#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <ResourceLoader.hpp>
#include <ImageTexture.hpp>
#include <Shader.hpp>
#include <ShaderMaterial.hpp>

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
		bool materialParamsNeedUpdate(void) { return m_materialParamsNeedUpdate; }
		void materialParamsNeedUpdate(bool b) { m_materialParamsNeedUpdate = b; }
		void updateMaterialParams(void);
		void resetMaterialParams(void);
		Ref<Material> getMaterial(void) { return m_material; };

	private:
		void debugPrintTexture(std::string tex_name, Ref<Texture> tex);
		Color encodeNormal(Vector3 normal);

	private:
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		Ref<ShaderMaterial> m_material;
		bool m_materialParamsNeedUpdate;

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
		AABB getChunkAABB(void) { return m_chunkAABB; }
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

	class QuadTree
	{
	public:
		QuadTree(GDN_TheWorld_Viewer* viewer, TheWorld_MapManager::MapManager::QuadrantId quadrantId);
		~QuadTree();

		void init(float& viewerPosX, float& viewerPosZ);
		
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
		TheWorld_MapManager::MapManager::Quadrant* getQuadrant(void) {	return m_worldQuadrant;	}
		void updateMaterialParams(void);
		void resetMaterialParams(void);
		bool materialParamsNeedUpdate(void);
		void materialParamsNeedUpdate(bool b);
		ShaderTerrainData& getShaderTerrainData(void) { return m_shaderTerrainData; }
		bool isValid(void) { return m_isValid; }
		void setValid(bool b = true) { m_isValid = b; }
		bool isVisible(void) { return m_isVisible; }
		void setVisible(bool b) { m_isVisible = b; }
		void refreshTime(std::timespec& time) { m_refreshTime = time; }
		std::timespec getRefreshTime(void) { return m_refreshTime; }
		void setTag(string tag) { m_tag = tag; }
		Transform internalTransformGlobalCoord(void);

	private:
		void internalUpdate(Vector3 cameraPosGlobalCoord, Quad* quadTreeNode);

	private:
		bool m_isValid;
		bool m_isVisible;
		string m_tag;
		std::unique_ptr<Quad> m_root;
		GDN_TheWorld_Viewer* m_viewer;
		Chunk::MapChunk m_mapChunk;
		std::vector<Chunk*> m_vectChunkUpdate;
		TheWorld_MapManager::MapManager::Quadrant* m_worldQuadrant;
		ShaderTerrainData m_shaderTerrainData;
		std::timespec m_refreshTime;

		// Statistics
		int m_numSplits;
		int m_numJoins;
		int m_numLeaf;
	};
}
