//#include "pch.h"
#include "QuadTree.h"
#include <File.hpp>

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

#include "assert.h"

using namespace godot;

// QuadTree coordinates are local to Viewer Node which contains the grid of quads: the first chunk (posX = posZ = 0) has its lower vertex in the origin of the Viwer Node

Quad::Quad(int slotPosX, int slotPosZ, int lod, enum PosInQuad posInQuad, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
{
	m_slotPosX = slotPosX;
	m_slotPosZ = slotPosZ;
	m_lod = lod;
	m_posInQuad = posInQuad;
	m_viewer = viewer;
	m_chunk = nullptr;
	m_chunkSizeInWUs = 0;

	m_quadTree = quadTree;
	
	assignChunk();
}

Quad::~Quad()
{
	clearChildren();
}

Quad* Quad::getChild(int idx)
{
	if (isLeaf())
		return nullptr;

	return m_children[idx].get();
}

bool Quad::isLeaf(void)
{
	return (m_children[0].get() == nullptr ? true : false);
}

void Quad::split(void)
{
	assert(m_lod > 0);
	assert(m_children[0] == nullptr);
	
	for (int i = 0; i < 4; i++)
	{
		//int x = (m_slotPosX * 2) + (i & 1);
		//int z = (m_slotPosZ * 2) + ((i & 2) >> 1);
		m_children[i] = make_unique<Quad>((m_slotPosX * 2) + (i & 1), (m_slotPosZ * 2) + ((i & 2) >> 1), m_lod - 1, enum PosInQuad(i + 1), m_viewer, m_quadTree);
	}
	
	recycleChunk();
}

void Quad::recycleChunk(void)
{
	if (m_chunk)
	{
		m_chunk->setPosInQuad(PosInQuad::NotSet);
		m_chunk->setVisible(false);
		m_chunk->setActive(false);
		m_chunk = nullptr;
	}
}

void Quad::clearChildren(void)
{
	assert(m_children.size() == 0 || m_children.size() == 4);
	if (m_children.size() != 0)
		for (int i = 0; i < 4; i++)
		{
			if (m_children[i] != nullptr)
			{
				m_children[i]->recycleChunk();
				m_children[i].reset();
			}
		}
}

void Quad::assignChunk(void)
{
	assert(m_chunk == nullptr);
	
	m_chunk = m_quadTree->getChunkAt(Chunk::ChunkPos(m_slotPosX, m_slotPosZ, m_lod));

	if (m_chunk == nullptr)
	{
		// TODORIC Material stuff

		// create chunk for current quad because this is the first time that is needed for current lod and pos
		Ref<Material> mat = m_quadTree->getShaderTerrainData().getMaterial();
		if (_DEBUG_AAB)
			m_chunk = new ChunkDebug(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);
		else
			m_chunk = new Chunk(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);

		// Set Viwer Pos in Global Coord
		m_chunk->setParentGlobalTransform(m_viewer->internalTransformGlobalCoord());
		m_quadTree->addChunk(m_chunk);
	}
	m_chunk->setPosInQuad(m_posInQuad);
	m_quadTree->addChunkUpdate(m_chunk);
	m_chunk->setActive(true);
	m_chunkAABB = m_chunk->getAABB();
	m_chunkSizeInWUs = m_chunk->getChunkSizeInWUs();
}

void Quad::setCameraPos(Vector3 localToGriddCoordCameraLastPos, Vector3 globalCoordCameraLastPos)
{
	if (isLeaf())
	{
		Chunk* chunk = getChunk();
		if (chunk != nullptr)
			chunk->setCameraPos(localToGriddCoordCameraLastPos, globalCoordCameraLastPos);
	}
}
	
QuadTree::QuadTree(GDN_TheWorld_Viewer* viewer)
{
	m_isValid = false;
	m_viewer = viewer;
	m_numSplits = 0;
	m_numJoins = 0;
	m_numLeaf = 0;
	m_worldQuadrant = nullptr;
}

void QuadTree::init(TheWorld_MapManager::MapManager::Quadrant* worldQuadrant)
{
	m_isValid = false;
	m_worldQuadrant = worldQuadrant;
	m_root = make_unique<Quad>(0, 0, m_viewer->Globals()->lodMaxDepth(), PosInQuad::NotSet, m_viewer, this);
	m_shaderTerrainData.init(m_viewer, this);
	resetMaterialParams();
	m_isValid = true;
}

QuadTree::~QuadTree()
{
	m_root.reset();
	
	clearChunkUpdate();
	
	// Delete all Chunks
	for (Chunk::MapChunk::iterator it = m_mapChunk.begin(); it != m_mapChunk.end(); it++)
	{
		for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
		{
			delete it1->second;
		}
		it->second.clear();
	}
	m_mapChunk.clear();

	if (m_worldQuadrant)
		delete m_worldQuadrant;
}

void QuadTree::ForAllChunk(Chunk::ChunkAction& chunkAction)
{
	for (Chunk::MapChunk::iterator it = m_mapChunk.begin(); it != m_mapChunk.end(); it++)
	{
		for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
		{
			chunkAction.exec(it1->second);
		}
	}
}

void QuadTree::update(Vector3 cameraPosViewerNodeLocalCoord, Vector3 cameraPosGlobalCoord)
{
	m_numLeaf = 0;
	internalUpdate(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord, m_root.get());
	assert(!m_root->isLeaf() || m_root->getChunk() != nullptr);
}

void QuadTree::internalUpdate(Vector3 cameraPosViewerNodeLocalCoord, Vector3 cameraPosGlobalCoord, Quad* quad)
{
	// cameraPosViewerNodeLocalCoord are in grid local coordinates (WUs)

	GDN_TheWorld_Globals* globals = m_viewer->Globals();
	//float chunkSizeInWUs = globals->gridStepInBitmapWUs(quad->Lod()) * globals->numVerticesPerChuckSide();									// chunk size in World Units
	float chunkSizeInWUs = quad->getChunkSizeInWUs();
	Vector3 chunkCenter = real_t(chunkSizeInWUs) * (Vector3(real_t(quad->slotPosX()), 0, real_t(quad->slotPosZ())) + Vector3(0.5, 0, 0.5));	// chunk center in local coord. respect Viewer Node (or the grid of chunks)
	AABB chunkAABB = quad->getChunkAABB();
	chunkCenter.y = (chunkAABB.position + chunkAABB.size / 2).y;		// medium altitude of the chunk
	real_t splitDistance = chunkSizeInWUs * globals->splitScale();
	if (quad->isLeaf())
	{
		
		m_numLeaf++;
		if (quad->Lod() > 0)
		{
			real_t distance = chunkCenter.distance_to(cameraPosViewerNodeLocalCoord);
			if (distance < splitDistance)
			{
				// Split
				quad->split();
				for (int i = 0; i < 4; i++)
					quad->getChild(i)->setCameraPos(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord);;
				m_numSplits++;
			}
		}
	}
	else
	{
		bool allChildrenAreLeaf = true;
		for (int i = 0; i < 4; i++)
		{
			Quad* child = quad->getChild(i);
			internalUpdate(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord, child);
			if (!child->isLeaf())
				allChildrenAreLeaf = false;
		}
		
		if (allChildrenAreLeaf && chunkCenter.distance_to(cameraPosViewerNodeLocalCoord) > splitDistance)
		{
			// Join
			quad->clearChildren();
			quad->assignChunk();
			quad->getChunk()->setJustJoined(true);
			m_numJoins++;
		}
	}
	
	// after all if quad is leaf: just joined or was leaf and it has not been splitted
	quad->setCameraPos(cameraPosViewerNodeLocalCoord, cameraPosGlobalCoord);
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir)
{
	Chunk* chunk = nullptr;

	switch (dir)
	{
	case Chunk::DirectionSlot::Center:
		chunk = getChunkAt(pos);
		break;
	case Chunk::DirectionSlot::XMinusChunk:
		//
		// - o
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
		break;
	case Chunk::DirectionSlot::XPlusChunk:
		//
		//   o -
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 0, -1));
		break;
	case Chunk::DirectionSlot::ZMinusChunk:
		//   -
		//   o
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
		break;
	case Chunk::DirectionSlot::ZPlusChunk:
		//
		//   o
		//   -
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 1, -1));
		break;
	case Chunk::DirectionSlot::XMinusQuadSecondChunk:
		//  
		//	- o =
		//	  = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
		break;
	case Chunk::DirectionSlot::XMinusQuadForthChunk:
		//  
		//	  o =
		//	- = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 1, -1));
		break;
	case Chunk::DirectionSlot::XPlusQuadFirstChunk:
		//  
		//	  o = -
		//	  = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 0, -1));
		break;
	case Chunk::DirectionSlot::XPlusQuadThirdChunk:
		//  
		//	  o = 
		//	  = = -
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 1, -1));
		break;
	case Chunk::DirectionSlot::ZMinusQuadThirdChunk:
		//    -
		//	  o =
		//	  = =
		//    
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
		break;
	case Chunk::DirectionSlot::ZMinusQuadForthChunk:
		//      -
		//	  o =
		//	  = =
		//    
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, -1, -1));
		break;
	case Chunk::DirectionSlot::ZPlusQuadFirstChunk:
		//    
		//	  o =
		//	  = =
		//    -
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 2, -1));
		break;
	case Chunk::DirectionSlot::ZPlusQuadSecondChunk:
		//      
		//	  o =
		//	  = =
		//      -
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 2, -1));
		break;
	//default:
	}

	return chunk;
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos)
{
	Chunk::MapChunk::iterator it = m_mapChunk.find(pos.getLod());
	if (it == m_mapChunk.end())
		return nullptr;
	else
	{
		Chunk::MapChunkPerLod::iterator it1 = m_mapChunk[pos.getLod()].find(pos);
		if (it1 == m_mapChunk[pos.getLod()].end())
			return nullptr;
		else
			return m_mapChunk[pos.getLod()][pos];
	}
}

void QuadTree::addChunk(Chunk* chunk)
{
	if (getChunkAt(chunk->getPos()))
		return;
	
	Chunk::MapChunk::iterator it = m_mapChunk.find(chunk->getLod());
	if (it == m_mapChunk.end())
	{
		Chunk::MapChunkPerLod m;
		m_mapChunk[chunk->getLod()] = m;
	}

	m_mapChunk[chunk->getLod()][chunk->getPos()] = chunk;
}

void QuadTree::addChunkUpdate(Chunk* chunk)
{
	if (chunk->isPendingUpdate())
		return;

	m_vectChunkUpdate.push_back(chunk);
	chunk->setPendingUpdate(true);
}

void QuadTree::clearChunkUpdate(void)
{
	m_vectChunkUpdate.clear();
}

void QuadTree::updateMaterialParams(void)
{
	if (m_shaderTerrainData.materialParamsNeedUpdate())
	{
		m_shaderTerrainData.updateMaterialParams();
		m_shaderTerrainData.materialParamsNeedUpdate(false);
	}
}

void QuadTree::resetMaterialParams(void)
{
	m_shaderTerrainData.resetMaterialParams();
}

bool QuadTree::materialParamsNeedUpdate(void)
{
	return m_shaderTerrainData.materialParamsNeedUpdate();
}

void QuadTree::materialParamsNeedUpdate(bool b)
{
	m_shaderTerrainData.materialParamsNeedUpdate(b);
}

void QuadTree::dump(void)
{
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	globals->debugPrint(String("Num vertices per chunk side: ") + String(to_string(globals->numVerticesPerChuckSide()).c_str())
				+ String(" - ") + String("Num vertices pert grid size: ") + String(to_string(globals->heightmapResolution()).c_str())
				+ String(" - ") + String("Grid size [WUs]: ") + String(to_string(globals->heightmapSizeInWUs()).c_str())
				+ String(" - ") + String("Lod max depth: ") + String(to_string(globals->lodMaxDepth()).c_str()));

	globals->debugPrint(String("FROM LAST DUMP - Num quad split: ") + String(to_string(m_numSplits).c_str())
				+ String(" - ") + String("Num quad join: ") + String(to_string(m_numJoins).c_str())
				+ String(" - ") + String("Num leaves: ") + String(to_string(m_numLeaf).c_str()));

	globals->debugPrint(String("GRID POS (global)")
		+ String(" - ") + String("X = ") + String(to_string(m_viewer->get_global_transform().origin.x).c_str())
		+ String(" - ") + String("Z = ") + String(to_string(m_viewer->get_global_transform().origin.z).c_str()));

	int numChunks = 0;
	int numActiveChunks = 0;
	Chunk* cameraChunk = nullptr;
	for (int i = globals->lodMaxDepth(); i >= 0; i--)
	{
		int numChunksAtLod = 0;
		Chunk::MapChunk::iterator it = m_mapChunk.find(i);
		if (it != m_mapChunk.end())
			numChunksAtLod = int(m_mapChunk[i].size());
		int numChunksActiveAtLod = 0;
		for (Chunk::MapChunkPerLod::iterator it1 = m_mapChunk[i].begin(); it1 != m_mapChunk[i].end(); it1++)
		{
			if (it1->second->isActive())
				numChunksActiveAtLod++;
			if (it1->second->isCameraVerticalOnChunk())
				cameraChunk = it1->second;
		}
		globals->debugPrint("Num chunks at lod " + String(to_string(i).c_str()) + ": " + String(to_string(numChunksAtLod).c_str())
			+ String("\t") + " Active " + String(to_string(numChunksActiveAtLod).c_str()));
		numChunks += numChunksAtLod;
		numActiveChunks += numChunksActiveAtLod;
	}

	globals->debugPrint("Num chunks: " + String(to_string(numChunks).c_str()));
	globals->debugPrint("Num active chunks: " + String(to_string(numActiveChunks).c_str()));

	// DEBUGRIC
	//if (cameraChunk != nullptr)
	//{
	//	Ref<Mesh> cameraMesh = cameraChunk->getMesh();
	//	Array cameraArrays = cameraMesh->surface_get_arrays(0);
	//	PoolVector3Array cameraVerts = cameraArrays[Mesh::ARRAY_VERTEX];
	//	globals->debugPrint("camerMesh Verts: " + String(to_string(cameraVerts.size()).c_str()));
	//	Transform cameraT = cameraChunk->getMeshGlobalTransformApplied();
	//	globals->debugPrint("camerMesh t= " + String(cameraT));
	//	int numPointsPerSize = (int)sqrt(cameraVerts.size());
	//	String s;	int i = 0;
	//	for (int z = 0; z < numPointsPerSize; z++)
	//	{
	//		for (int x = 0; x < numPointsPerSize; x++)
	//		{
	//			s += cameraVerts[i];
	//			s += " ";
	//			i++;
	//		}
	//		globals->debugPrint("camerMesh r=" + String(to_string(z + 1).c_str()) + " " + s);
	//		s = "";
	//	}

	//	Chunk* cameraChunkXMinus = getChunkAt(cameraChunk->getPos(), Chunk::DirectionSlot::XMinusChunk);
	//	if (cameraChunkXMinus != nullptr)
	//	{
	//		Ref<Mesh> camerMeshXMinus = cameraChunkXMinus->getMesh();
	//		Array cameraArraysXMinus = camerMeshXMinus->surface_get_arrays(0);
	//		PoolVector3Array cameraVertsXMinus = cameraArraysXMinus[Mesh::ARRAY_VERTEX];
	//		globals->debugPrint("camerMeshXMinus Verts: " + String(to_string(cameraVertsXMinus.size()).c_str()));
	//		Transform cameraT = cameraChunkXMinus->getMeshGlobalTransformApplied();
	//		globals->debugPrint("camerMeshXMinus t= " + String(cameraT));
	//		int numPointsPerSize = (int)sqrt(cameraVertsXMinus.size());
	//		String s;	int i = 0;
	//		for (int z = 0; z < numPointsPerSize; z++)
	//		{
	//			for (int x = 0; x < numPointsPerSize; x++)
	//			{
	//				s += cameraVertsXMinus[i];
	//				s += " ";
	//				i++;
	//			}
	//			globals->debugPrint("camerMeshXMinus r=" + String(to_string(z + 1).c_str()) + " " + s);
	//			s = "";
	//		}
	//	}
	//	Chunk* cameraChunkXPlus = getChunkAt(cameraChunk->getPos(), Chunk::DirectionSlot::XPlusChunk);
	//	if (cameraChunkXPlus != nullptr)
	//	{
	//		Ref<Mesh> camerMeshXPlus = cameraChunkXPlus->getMesh();
	//		Array cameraArraysXPlus = camerMeshXPlus->surface_get_arrays(0);
	//		PoolVector3Array cameraVertsXPlus = cameraArraysXPlus[Mesh::ARRAY_VERTEX];
	//		globals->debugPrint("camerMeshXPlus Verts: " + String(to_string(cameraVertsXPlus.size()).c_str()));
	//		Transform cameraT = cameraChunkXPlus->getMeshGlobalTransformApplied();
	//		globals->debugPrint("camerMeshXPlus t= " + String(cameraT));
	//		int numPointsPerSize = (int)sqrt(cameraVertsXPlus.size());
	//		String s;	int i = 0;
	//		for (int z = 0; z < numPointsPerSize; z++)
	//		{
	//			for (int x = 0; x < numPointsPerSize; x++)
	//			{
	//				s += cameraVertsXPlus[i];
	//				s += " ";
	//				i++;
	//			}
	//			globals->debugPrint("camerMeshXPlus r=" + String(to_string(z + 1).c_str()) + " " + s);
	//			s = "";
	//		}
	//	}
	//}
	// DEBUGRIC

	Chunk::DumpChunkAction action;
	ForAllChunk(action);

	m_numSplits = 0;
	m_numJoins = 0;
}

ShaderTerrainData::ShaderTerrainData()
{
	m_viewer = nullptr;
	m_quadTree = nullptr;
	m_materialParamsNeedUpdate = false;
	m_heightMapTexModified = false;
	m_normalMapTexModified = false;
	//m_splat1MapTexModified = false;
	//m_colorMapTexModified = false;
}

ShaderTerrainData::~ShaderTerrainData()
{
}

void ShaderTerrainData::init(GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
{
	m_viewer = viewer;
	m_quadTree = quadTree;
	Ref<ShaderMaterial> mat = ShaderMaterial::_new();
	ResourceLoader* resLoader = ResourceLoader::get_singleton();
	String shaderPath = "res://shaders/lookdev.shader";
	//String shaderPath = "res://shaders/test.shader";		// SUPER DEBUGRIC
	Ref<Shader> shader = resLoader->load(shaderPath);
	mat->set_shader(shader);

	m_material = mat;
}

// it is expected that globals and World Datas are loaded
// TODORIC: maybe usefull for performance reasons specify which texture need update and which rect of the texture 
void ShaderTerrainData::resetMaterialParams(void)
{
	int _resolution = m_viewer->Globals()->heightmapResolution() + 1;

	// Creating Heightmap Map Texture
	{
		Ref<Image> image = Image::_new();
		image->create(_resolution, _resolution, false, Image::FORMAT_RH);
		m_heightMapImage = image;
	}

	// Creating Normal Map Texture
	{
		Ref<Image> image = Image::_new();
		image->create(_resolution, _resolution, false, Image::FORMAT_RGB8);
		m_normalMapImage = image;
	}

	{
		// Filling Heightmap Map Texture , Normal Map Texture
		assert(_resolution == m_heightMapImage->get_height());
		assert(_resolution == m_heightMapImage->get_width());
		assert(_resolution == m_quadTree->getQuadrant()->getId().getNumVerticesPerSize());
		float gridStepInWUs = m_viewer->Globals()->gridStepInWU();
		m_heightMapImage->lock();
		m_normalMapImage->lock();
		for (int z = 0; z < _resolution; z++)			// m_heightMapImage->get_height()
			for (int x = 0; x < _resolution; x++)		// m_heightMapImage->get_width()
			{
				float h = m_quadTree->getQuadrant()->getGridVertices()[z * _resolution + x].altitude();
				m_heightMapImage->set_pixel(x, z, Color(h, 0, 0));
				//if (h != 0)	// DEBUGRIC
				//	m_viewer->Globals()->debugPrint("Altitude not null.X=" + String(std::to_string(x).c_str()) + " Z=" + std::to_string(z).c_str() + " H=" + std::to_string(h).c_str());

				// h = height of the point for which we are computing the normal
				// hr = height of the point on the rigth
				// hl = height of the point on the left
				// hf = height of the forward point (z growing)
				// hb = height of the backward point (z lessening)
				// step = step in WUs between points
				// we compute normal normalizing the vector (h - hr, step, h - hf) or (hl - h, step, hb - h)
				// according to https://hterrain-plugin.readthedocs.io/en/latest/ section "Procedural generation" it should be (h - hr, step, hf - h)
				Vector3 normal;
				Vector3 P((float)x, h, (float)z);
				if (x < _resolution - 1 && z < _resolution - 1)
				{
					float hr = m_quadTree->getQuadrant()->getGridVertices()[z * _resolution + x + 1].altitude();
					float hf = m_quadTree->getQuadrant()->getGridVertices()[(z + 1) * _resolution + x].altitude();
					normal = Vector3(h - hr, gridStepInWUs, h - hf).normalized();
					//{		// Verify
					//	Vector3 PR((float)(x + gridStepInWUs), hr, (float)z);
					//	Vector3 PF((float)x, hf, (float)(z + gridStepInWUs));
					//	Vector3 normal1 = (PF - P).cross(PR - P).normalized();
					//	if (!equal(normal1, normal))	// DEBUGRIC
					//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
					//}
				}
				else
				{
					if (x == _resolution - 1 && z == 0)
					{
						float hf = m_quadTree->getQuadrant()->getGridVertices()[(z + 1) * _resolution + x].altitude();
						float hl = m_quadTree->getQuadrant()->getGridVertices()[z * _resolution + x - 1].altitude();
						normal = Vector3(hl - h, gridStepInWUs, h - hf).normalized();
						//{		// Verify
						//	Vector3 PL((float)(x - gridStepInWUs), hl, (float)z);
						//	Vector3 PF((float)x, hf, (float)(z + gridStepInWUs));
						//	Vector3 normal1 = (PL - P).cross(PF - P).normalized();
						//	if (!equal(normal1, normal))	// DEBUGRIC
						//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
						//}
					}
					else if (x == 0 && z == _resolution - 1)
					{
						float hr = m_quadTree->getQuadrant()->getGridVertices()[z * _resolution + x + 1].altitude();
						float hb = m_quadTree->getQuadrant()->getGridVertices()[(z - 1) * _resolution + x].altitude();
						normal = Vector3(h - hr, gridStepInWUs, hb - h).normalized();
						//{		// Verify
						//	Vector3 PR((float)(x + gridStepInWUs), hr, (float)z);
						//	Vector3 PB((float)(x), hb, (float)(z - gridStepInWUs));
						//	Vector3 normal1 = (PR - P).cross(PB - P).normalized();
						//	if (!equal(normal1, normal))	// DEBUGRIC
						//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
						//}
					}
					else
					{
						float hl = m_quadTree->getQuadrant()->getGridVertices()[z * _resolution + x - 1].altitude();
						float hb = m_quadTree->getQuadrant()->getGridVertices()[(z - 1) * _resolution + x].altitude();
						normal = Vector3(hl - h, gridStepInWUs, hb - h).normalized();
						//{		// Verify
						//	Vector3 PB((float)x, hb, (float)(z - gridStepInWUs));
						//	Vector3 PL((float)(x - gridStepInWUs), hl, (float)z);
						//	Vector3 normal1 = (PB - P).cross(PL - P).normalized();
						//	if (!equal(normal1, normal))	// DEBUGRIC
						//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
						//}
					}
				}
				m_normalMapImage->set_pixel(x, z, encodeNormal(normal));
			}
		m_normalMapImage->unlock();
		m_heightMapImage->unlock();

		{
			Ref<ImageTexture> tex = ImageTexture::_new();
			tex->create_from_image(m_heightMapImage, Texture::FLAG_FILTER);
			m_heightMapTexture = tex;
			m_heightMapTexModified = true;
			//debugPrintTexture(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightMapTexture);	// DEBUGRIC
		}

		{
			Ref<ImageTexture> tex = ImageTexture::_new();
			tex->create_from_image(m_normalMapImage, Texture::FLAG_FILTER);
			m_normalMapTexture = tex;
			m_normalMapTexModified = true;
			//debugPrintTexture(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalMapTexture);	// DEBUGRIC
		}

	}

	// Creating Splat Map Texture
	//{
	//	Ref<Image> image = Image::_new();
	//	image->create(_resolution, _resolution, false, Image::FORMAT_RGBA8);
	//	image->fill(Color(1, 0, 0, 0));
	//	m_splat1MapImage = image;
	//}

	// Filling Splat Map Texture
	//{
	//	Ref<ImageTexture> tex = ImageTexture::_new();
	//	tex->create_from_image(m_splat1MapImage, Texture::FLAG_FILTER);
	//	m_splat1MapTexture = tex;
	//	m_splat1MapImageModified = true;
	//}

	// Creating Color Map Texture
	//{
	//	Ref<Image> image = Image::_new();
	//	image->create(_resolution, _resolution, false, Image::FORMAT_RGBA8);
	//	image->fill(Color(1, 1, 1, 1));
	//	m_colorMapImage = image;
	//}

	// Filling Color Map Texture
	//{
	//	Ref<ImageTexture> tex = ImageTexture::_new();
	//	tex->create_from_image(m_colorMapImage, Texture::FLAG_FILTER);
	//	m_colorMapTexture = tex;
	//	m_colorMapImageModified = true;
	//}

	// _update_all_vertical_bounds ???	// TODORIC
	//	# RGF image where R is min heightand G is max height
	//	var _chunked_vertical_bounds : = Image.new()	// _chunked_vertical_bounds.create(csize_x, csize_y, false, Image.FORMAT_RGF)
	//	_update_vertical_bounds(0, 0, _resolution - 1, _resolution - 1)

	materialParamsNeedUpdate(true);
}

Color ShaderTerrainData::encodeNormal(Vector3 normal)
{
	normal = 0.5 * (normal + Vector3::ONE);
	return Color(normal.x, normal.z, normal.y);
}

void ShaderTerrainData::updateMaterialParams(void)
{
	// Completare da _update_material_params
	m_viewer->Globals()->debugPrint("Updating terrain material params");

	if (m_viewer->is_inside_tree())
	{
		Transform globalTransform = m_viewer->internalTransformGlobalCoord();
		//globalTransform.origin = Vector3(0, 0, 0);		// DEBUGRIC
		m_viewer->Globals()->debugPrint("internalTransformGlobalCoord" + String(" t=") + String(globalTransform));	// DEBUGRIC

		Transform t = globalTransform.affine_inverse();
		m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_INVERSE_TRANSFORM) + String(" t=") + String(t));	// DEBUGRIC
		m_material->set_shader_param(SHADER_PARAM_INVERSE_TRANSFORM, t);

		Basis b = globalTransform.basis.inverse().transposed();
		m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_NORMAL_BASIS) + String(" b=") + String(b));	// DEBUGRIC
		m_material->set_shader_param(SHADER_PARAM_NORMAL_BASIS, b);

		float f = m_viewer->Globals()->gridStepInWU();
		m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GRID_STEP) + String(" grid_step=") + String(std::to_string(f).c_str()));	// DEBUGRIC
		m_material->set_shader_param(SHADER_PARAM_GRID_STEP, f);

		if (m_heightMapTexModified)
		{
			m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_HEIGHTMAP));	// DEBUGRIC
			m_material->set_shader_param(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightMapTexture);
			m_heightMapTexModified = false;
		}

		if (m_normalMapTexModified)
		{
			m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_NORMALMAP));	// DEBUGRIC
			m_material->set_shader_param(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalMapTexture);
			m_normalMapTexModified = false;
		}

		//Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node (grid) local coordinates of the camera pos
	}
}

void ShaderTerrainData::debugPrintTexture(std::string tex_name, Ref<Texture> tex)
{
	File* _file = File::_new();
	_file->open("res://" + String(tex_name.c_str()) + ".txt", File::WRITE);
	Ref<Image> _image = tex->get_data();
	int64_t width = _image->get_width();
	int64_t height = _image->get_height();
	Image::Format format = _image->get_format();
	std::string formatStr = std::to_string(format);
	if (format == Image::Format::FORMAT_RH)
		formatStr = "FORMAT_RH";
	else if (format == Image::Format::FORMAT_RGB8)
		formatStr = "FORMAT_RGB8";
	else if (format == Image::Format::FORMAT_RGBA8)
		formatStr = "FORMAT_RGBA8";
	std::string text = "Name=" + tex_name + " Format=" + formatStr + " W=" + std::to_string(width) + " H=" + std::to_string(height);
	_file->store_string(String(text.c_str()) + "\n");
	_image->lock();
	for (int h = 0; h < height; h++)
	{
		std::string row = "";
		for (int w = 0; w < width; w++)
		{
			row += std::to_string(w) + ":";
			if (format == Image::Format::FORMAT_RH)
				row += std::to_string(_image->get_pixel(w, h).r) + " ";
			else if (format == Image::Format::FORMAT_RGB8)
				row += std::to_string(_image->get_pixel(w, h).r) + "-" + std::to_string(_image->get_pixel(w, h).g) + "-" + std::to_string(_image->get_pixel(w, h).b) + " ";
			else if (format == Image::Format::FORMAT_RGBA8)
				row += std::to_string(_image->get_pixel(w, h).r) + "-" + std::to_string(_image->get_pixel(w, h).g) + "-" + std::to_string(_image->get_pixel(w, h).b) + "-" + std::to_string(_image->get_pixel(w, h).a) + " ";
			else
			{
				String s = String(_image->get_pixel(w, h));
				char* str = s.alloc_c_string();
				row += str;
				row += " ";
				godot::api->godot_free(str);
			}
		}
		text = "H=" + std::to_string(h) + " " + row;
		_file->store_string(String(text.c_str()) + "\n");
	}
	_image->unlock();
	_file->close();
}

