#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <VisualServer.hpp>
#include <ArrayMesh.hpp>
#include <AABB.hpp>
#include <Material.hpp>
#include <SpatialMaterial.hpp>

#include <map>
#include <string>

#include "GDN_TheWorld_Globals.h"

namespace godot
{
#define DEBUG_WIRECUBE_MESH			"debug_wirecube_mesh_lod_"
#define DEBUG_WIRESQUARE_MESH		"debug_wiresquare_mesh_lod_"

	class GDN_TheWorld_Viewer;

	class Chunk
	{
	public:
		enum class DirectionSlot
		{
			Center = -1,
			Left = 0,
			Right = 1,
			Bottom = 2,
			Top = 3,
			LeftTop = 4,
			LeftBottom = 5,
			RightTop = 6,
			RightBottom = 7,
			BottomLeft = 8,
			BottomRight = 9,
			TopLeft = 10,
			TopRight = 11
		};

		class ChunkAction
		{
		public:
			ChunkAction() {}
			virtual ~ChunkAction() {}
			virtual void exec(Chunk* chunk) = 0;
		private:
		};

		class SwitchDebugModeAction : public ChunkAction
		{
		public:
			SwitchDebugModeAction(enum class GDN_TheWorld_Globals::ChunkDebugMode mode) { m_mode = mode; }
			virtual ~SwitchDebugModeAction() {}
			virtual void exec(Chunk* chunk)
			{
				if (m_mode != GDN_TheWorld_Globals::ChunkDebugMode::DoNotSet)
				{
					chunk->setDebugMode(m_mode);
					chunk->applyDebugMesh();
					if (m_mode != GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
						chunk->applyAABB();
				}
			}
		private:
			enum class GDN_TheWorld_Globals::ChunkDebugMode m_mode;
		};

		class EnterWorldChunkAction : public ChunkAction
		{
		public:
			EnterWorldChunkAction() {}
			virtual ~EnterWorldChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				chunk->enterWorld();
			}
		private:
		};

		class ExitWorldChunkAction : public ChunkAction
		{
		public:
			ExitWorldChunkAction() {}
			virtual ~ExitWorldChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				chunk->exitWorld();
			}
		private:
		};

		class VisibilityChangedChunkAction : public ChunkAction
		{
		public:
			VisibilityChangedChunkAction(bool isVisibleInTree) { m_isVisibleInTree = isVisibleInTree; }
			virtual ~VisibilityChangedChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				chunk->setVisible(m_isVisibleInTree);
			}
		private:
			bool m_isVisibleInTree;
		};

		class DebugVisibilityChangedChunkAction : public ChunkAction
		{
		public:
			DebugVisibilityChangedChunkAction(bool isVisible) { m_isVisible = isVisible; }
			virtual ~DebugVisibilityChangedChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				chunk->setDebugVisibility(m_isVisible);
			}
		private:
			bool m_isVisible;
		};

		class TransformChangedChunkAction : public ChunkAction
		{
		public:
			TransformChangedChunkAction(Transform globalT) { m_globalT = globalT; }
			virtual ~TransformChangedChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				chunk->parentTransformChanged(m_globalT);
			}
		private:
			Transform m_globalT;
		};

		class DumpChunkAction : public ChunkAction
		{
		public:
			DumpChunkAction(void) {}
			virtual ~DumpChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				chunk->dump();
			}
		private:
		};

		class ChunkPos
		{
		public:
			ChunkPos(int slotPosX, int slotPosZ, int lod)
			{
				m_slotPosX = slotPosX;
				m_slotPosZ = slotPosZ;
				m_lod = lod;
			}
			~ChunkPos() {}
			// needed to use an istance of GISPoint as a key in a map (to keep the map sorted by y and by x for equal y)
			// first row, second row, ... etc
			bool operator<(const ChunkPos& pos) const
			{
				if (m_lod == pos.m_lod)
				{
					if (m_slotPosZ < pos.m_slotPosZ)
						return true;
					if (m_slotPosZ > pos.m_slotPosZ)
						return false;
					else
						return m_slotPosX < pos.m_slotPosX;
				}
				else
					if (m_lod < pos.m_lod)
						return true;
					else
						return false;
			}
			bool operator==(const ChunkPos& pos) const
			{
				if (m_slotPosX == pos.m_slotPosX && m_slotPosZ == pos.m_slotPosZ && m_lod == pos.m_lod)
					return true;
				else
					return false;
			}
			ChunkPos operator=(const ChunkPos& pos)
			{
				m_slotPosX = pos.m_slotPosX;
				m_slotPosZ = pos.m_slotPosZ;
				m_lod = pos.m_lod;
				return *this;
			}
			ChunkPos operator+(const ChunkPos& pos)
			{
				m_slotPosX += pos.m_slotPosX;
				m_slotPosZ += pos.m_slotPosZ;
				//m_lod = pos.m_lod;
				return *this;
			}
			int getLod(void) { return m_lod; }
			int getSlotPosX(void) { return m_slotPosX; }
			int getSlotPosZ(void) { return m_slotPosZ; }
			std::string getId(void)
			{
				return "LOD:" + std::to_string(m_lod) + "-X:" + std::to_string(m_slotPosX) + "-Z:" + std::to_string(m_slotPosZ);
			}
		
		private:
			int m_slotPosX;
			int m_slotPosZ;
			int m_lod;
		};

		typedef std::map<Chunk::ChunkPos, Chunk*> MapChunkPerLod;
		typedef std::map<int, MapChunkPerLod> MapChunk;

		Chunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Ref<Material>& mat);
		virtual ~Chunk();
		
		void initVisual(void);
		void deinit(void);

		// Actions
		virtual void enterWorld(void);
		virtual void exitWorld(void);
		virtual void parentTransformChanged(Transform t);
		virtual void setVisible(bool b);
		virtual void setDebugVisibility(bool b);
		virtual void applyAABB(void);
		virtual void dump(void);
		virtual void setCameraPos(Vector3 localToGriddCoordCameraLastPos, Vector3 globalCoordCameraLastPos);
		virtual void setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode);
		virtual void applyDebugMesh(void);

		virtual void update(bool isVsisible);
		bool isActive(void) { return m_active; }
		void setActive(bool b);
		bool isVisible(void) { return m_visible; }
		bool isPendingUpdate(void) { return m_pendingUpdate; }
		void setPendingUpdate(bool b) { m_pendingUpdate = b; }
		bool gotJustJoined(void) { return m_justJoined; }
		void setJustJoined(bool b) { m_justJoined = b; }
		int getLod(void) { return m_lod; }
		ChunkPos getPos(void) { return ChunkPos(m_slotPosX, m_slotPosZ, m_lod); }
		float getChunkSizeInWUs(void) { return m_chunkSizeInWUs; }
		AABB getAABB(void) { return m_aabb; };
		void getCameraPos(Vector3& localToGriddCoordCameraLastPos, Vector3& globalCoordCameraLastPos);
		bool isCameraVerticalOnChunk(void) { return m_isCameraVerticalOnChunk; }
		Transform getGlobalTransformOfAABB(void);
	
	private:
		void setMesh(Ref<Mesh> mesh);

	protected:
		GDN_TheWorld_Viewer* m_viewer;
		Transform m_parentTransform;
		AABB m_aabb;						// AABB of the chunk in WUs relative to the chunk so X and Z are 0
		AABB m_gridRelativeAABB;			// AABB of the chunk in WUs relative to the grid (the viewer)
		int m_lod;
		Vector3 m_localToGriddCoordCameraLastPos;
		Vector3 m_globalCoordCameraLastPos;
		bool m_isCameraVerticalOnChunk;
		enum class GDN_TheWorld_Globals::ChunkDebugMode m_debugMode;
		bool m_debugVisibility;

		int m_numVerticesPerChuckSide;		// Number of vertices of the side of a chunk (-1) which is fixed (not a function of the lod) and is a multiple of 2
		int m_numChunksPerWorldGridSide;	// The number of chunks required to cover every side of the grid at the current lod value
		int m_gridStepInGridInWGVs;			// The number of World Grid vertices (-1) or grid vertices (-1) which separate the vertices of the mesh at the specified lod value
		float m_gridStepInWUs;				// The number of World Units which separate the vertices of the mesh at the specified lod value
		int m_chunkSizeInWGVs;				// The number of World Grid vertices (-1) of the chunk at the current lod
		float m_chunkSizeInWUs;				// The size in Word Units of the chunk at the current lod
		int m_originXInGridInWGVs;			// X coord. of the origin of the chunk (lower left corner) inside the Grid Map: it is expressed in number of grid vertices
		int m_originZInGridInWGVs;			// Z coord. of the origin of the chunk (lower left corner) inside the Grid Map: it is expressed in number of grid vertices
		float m_originXInWUsLocalToGrid;	// X coord. of the origin of the chunk (lower left corner) inside the Grid Map: it is local to the Grid Map and is expressed in Viewer Node local coordinate System
		float m_originZInWUsLocalToGrid;	// Z coord. of the origin of the chunk (lower left corner) inside the Grid Map: it is local to the Grid Map and is expressed in Viewer Node local coordinate System
		int m_firstWorldVertCol;			// 0-based starting column in the World Grid vertices array (Map Manager grid map) of the chunk/mesh (viewer m_worldVertices)
		int m_lastWorldVertCol;				// 0-based ending column in the World Grid vertices array (Map Manager grid map) of the chunk/mesh (viewer m_worldVertices)
		int m_firstWorldVertRow;			// 0-based starting row in the World Grid vertices array (Map Manager grid map) of the chunk/mesh (viewer m_worldVertices)
		int m_lastWorldVertRow;				// 0-based endif row in the World Grid vertices array (Map Manager grid map) of the chunk/mesh (viewer m_worldVertices)

	private:
		int m_slotPosX;	// express the orizzontal (X) and vertical (Z) position of the chunk in the grid of chunks
		int m_slotPosZ;	// at the specific lod : 0 the first chunk, 1 the following to the max number of chunks on a size for the specific lod
		Ref<Material> m_matOverride;

		bool m_active;
		bool m_visible;
		bool m_pendingUpdate;
		bool m_justJoined;
		RID m_meshInstance;
		RID m_meshRID;
		Ref<Mesh> m_mesh;
	};

	class ChunkDebug : public Chunk
	{
	public:
		ChunkDebug(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, Ref<Material>& mat);
		virtual ~ChunkDebug();

		virtual void enterWorld(void);
		virtual void exitWorld(void);
		virtual void parentTransformChanged(Transform parentT);
		virtual void setVisible(bool b);
		virtual void setDebugVisibility(bool b);
		virtual void applyAABB(void);
		virtual void dump(void);
		virtual void setCameraPos(Vector3 localToGriddCoordCameraLastPos, Vector3 globalCoordCameraLastPos);
		virtual void setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode);
		virtual void applyDebugMesh(void);

	private:
		void setDebugMesh(Ref<Mesh> mesh);
		Ref<ArrayMesh> createWireCubeMesh(Color c = Color(1, 1, 1));
		Ref<ArrayMesh> createWireSquareMesh(Color c = Color(1, 1, 1));

	private:
		RID m_debugMeshInstance;
		RID m_debugMeshRID;
		Ref<Mesh> m_debugMesh;
		AABB m_debugMeshAABB;
	};
}

