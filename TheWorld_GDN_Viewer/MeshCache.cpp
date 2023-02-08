//#include "pch.h"
#include "MeshCache.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"

#include <string>

#include <PoolArrays.hpp>
#include <SpatialMaterial.hpp>
#include <MeshDataTool.hpp>
#include <CubeMesh.hpp>

//using namespace godot;
namespace godot
{
	MeshCache::MeshCache(GDN_TheWorld_Viewer* viewer)
	{
		m_viewer = viewer;
	}

	MeshCache::~MeshCache()
	{
		resetCache();
	}

	void MeshCache::initCache(int numVerticesPerChuckSide, int numLods)
	{
		resetCache();

		m_meshCache.resize(SEAM_CONFIG_COUNT);
		
		for (int seams = 0; seams < SEAM_CONFIG_COUNT; seams++)
		{
			m_meshCache[seams].resize(m_viewer->Globals()->numLods());

			for (int idxLod = 0; idxLod < m_viewer->Globals()->numLods(); idxLod++)
			{
				m_meshCache[seams][idxLod] = makeFlatChunk(numVerticesPerChuckSide, idxLod, seams);
			}
		}
	}

	void MeshCache::resetCache(void)
	{
		for (int seams = 0; seams < m_meshCache.size(); seams++)
		{
			for (int idxLod = 0; idxLod < m_meshCache[seams].size(); idxLod++)
			{
				m_meshCache[seams][idxLod].unref();
			}
			m_meshCache[seams].clear();
		}
		m_meshCache.clear();
	}

	Ref<Mesh> MeshCache::makeFlatChunk(int numVerticesPerChuckSide, int idxLod, int seams)
	{
		//	https://gist.github.com/bnolan/01a7d240774f1e02056d6607e5b621da

		float strideInWUs = m_viewer->Globals()->strideInWorldGridWUs(idxLod);

		// Determines the vertices in a flat mesh at the required lod using World Grid vertices: coordinates are expressed in WUs and are realtive to the lower vertex of the chunk
		PoolVector3Array positions;
		PoolColorArray colors;
		Color initialVertexColor = GDN_TheWorld_Globals::g_color_white;
		//Color initialVertexColor = GDN_TheWorld_Globals::g_color_pink_amaranth;
		//strideInWUs = (numVerticesPerChuckSide * strideInWUs) / 4;	// DEBUGRIC
		//numVerticesPerChuckSide = 4;								// DEBUGRIC
		positions.resize((int)pow(numVerticesPerChuckSide + 1, 2));
		colors.resize((int)pow(numVerticesPerChuckSide + 1, 2));
		int pos = 0;
		for (real_t z = 0; z < numVerticesPerChuckSide + 1; z++)
			for (real_t x = 0; x < numVerticesPerChuckSide + 1; x++)
			{
				Vector3 v(x * strideInWUs, 0, z * strideInWUs);
				positions.set(pos, v);
				colors.set(pos, initialVertexColor);
				pos++;
			}
		
		// Preparing array of indices releate to vertex array to form the triangles of the mesh: a tringle every three indices
		PoolIntArray indices;
		void makeFlatChunk_makeIndices(PoolIntArray& indices, int numVerticesPerChuckSide, int seams);
		makeFlatChunk_makeIndices(indices, numVerticesPerChuckSide, seams);

		// DEBUGRIC
		/*
		{
			for (int i = 0; i < positions.size(); i++)
			{
				Vector3 val = positions[i];
				m_viewer->Globals()->debugPrint("Vertex(" + String(to_string(i).c_str()) + ")=" + String(val));
			}
			//for (int i = 0; i < colors.size(); i++)
			//{
			//	Color val = colors[i];
			//	m_viewer->Globals()->debugPrint("Color(" + String(to_string(i).c_str()) + ")=" + String(val));
			//}
			for (int i = 0; i < indices.size(); i++)
			{
				int val = indices[i];
				m_viewer->Globals()->debugPrint("Index(" + String(to_string(i).c_str()) + ")=" + String(to_string(val).c_str()));
			}
			//m_viewer->getMainProcessingMutex().lock();
			//MeshDataTool* mdt = MeshDataTool::_new();
			//m_viewer->getMainProcessingMutex().unlock();
			//Error err = mdt->create_from_surface(mesh, 0);
			//assert(err == Error::OK);
			//int64_t numEdges = mdt->get_edge_count();
			//int64_t numFaces = mdt->get_face_count();
			//for (int i = 0; i < mdt->get_vertex_count(); i++)
			//{
			//	Vector3 vertex = mdt->get_vertex(i);
			//	Color c = mdt->get_vertex_color(i);
			//}
		}
		*/
		// DEBUGRIC

		godot::Array arrays;
		arrays.resize(ArrayMesh::ARRAY_MAX);
		arrays[ArrayMesh::ARRAY_VERTEX] = positions;
		arrays[ArrayMesh::ARRAY_COLOR] = colors;
		arrays[ArrayMesh::ARRAY_INDEX] = indices;

		m_viewer->getMainProcessingMutex().lock();
		Ref<ArrayMesh> mesh = ArrayMesh::_new();
		m_viewer->getMainProcessingMutex().unlock();
		int64_t surf_idx = mesh->get_surface_count();	// next surface added will have this surf_idx
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

		// Initial Material
		m_viewer->getMainProcessingMutex().lock();
		Ref<SpatialMaterial> initialMaterial = SpatialMaterial::_new();
		m_viewer->getMainProcessingMutex().unlock();
		initialMaterial->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		initialMaterial->set_albedo(initialVertexColor);
		mesh->surface_set_material(surf_idx, initialMaterial);

		return mesh;
	}

	Mesh* MeshCache::getMesh(int seams, int lod)
	{
		return m_meshCache[seams][lod].ptr();
	}
}

void makeFlatChunk_makeIndices(godot::PoolIntArray& indices, int numVerticesPerChuckSide, int seams)
{
	// only even sizes admitted
	assert(numVerticesPerChuckSide % 2 == 0);
	
	int firstColumnOfRegularTriangles = 0;
	int firstRowOfRegularTriangles = 0;
	int numberOfColumnsOfRegularTriangles = numVerticesPerChuckSide;	// num vertices (-1) on each row
	int numberOfRowsOfRegularTriangles = numVerticesPerChuckSide;		// num rows (-1)
	int deviationToFirstVertxeOfNextRow = 1;							// To point the first vertex on next row when current vertex is the last of the row

	if (seams & SEAM_LEFT)
	{
		// Seam on the left: need to adjust to point all the vertices not on the border subject to a seam
		firstColumnOfRegularTriangles += 1;		// bypassing first vertex on every row
		numberOfColumnsOfRegularTriangles -= 1;	// as we bypass first vertex the number of the columns (the size of the row) is decremented by 1
		deviationToFirstVertxeOfNextRow += 1;	// and we increment by 1 the deviation to bypass the first vertex on rows next to the first
	}

	if (seams & SEAM_BOTTOM)
	{
		// Seam on the bottom: need to adjust to point all the vertices not on the border subject to a seam
		firstRowOfRegularTriangles += 1;		// bypassing first row
		numberOfRowsOfRegularTriangles -= 1;	// and of course decrementing the number of the rows
	}

	if (seams & SEAM_RIGHT)
	{
		// Seam on the rigth: need to adjust to point all the vertices not on the border subject to a seam
		numberOfColumnsOfRegularTriangles -= 1;	// bypassing last vertex on every row decrementeing by 1 the number of the columns (the size of the row)
		deviationToFirstVertxeOfNextRow += 1;	// and we increment by 1 the deviation to bypass the last vertex of every row
	}

	if (seams & SEAM_TOP)
	{
		// Seam on the top: need to adjust to point all the vertices not on the border subject to a seam
		numberOfRowsOfRegularTriangles -= 1;	// decrementing number of rows by 1 to bypass last row
	}

	// inserting regular triangles = triangles not on the border subject to a seam (left, right, top, bottom)
	int vertexIndex = firstColumnOfRegularTriangles + firstRowOfRegularTriangles * (numVerticesPerChuckSide + 1);
	for (int z = 0; z < numberOfRowsOfRegularTriangles; z++)
	{
		// sliding all rows with the exclusion of the ones on the border if it nead a seam (on the top or on the bottom) against a less defined mesh
		for (int x = 0; x < numberOfColumnsOfRegularTriangles; x++)
		{
			// sliding all vertex of a row with the exclusion of the ones on the border subject to a seam (on the left or on the right) against a less defined mesh
			
			//    x ==>
			// z  00---10
			// |   |\  |
			// -   | \ |
			//     |  \|
			//    01---11

			int ixz00 = vertexIndex;											// current vertex
			int ixz10 = vertexIndex + 1;										// next vertex
			//int ixz01 = vertexIndex + numberOfColumnsOfRegularTriangles + 1;	// same vertex on next row
			int ixz01 = vertexIndex + numVerticesPerChuckSide + 1;	// same vertex on next row
			int ixz11 = ixz01 + 1;												// next vertex

			// first triangle
			indices.append(ixz00);
			indices.append(ixz10);
			indices.append(ixz11);

			// second triangle
			indices.append(ixz00);
			indices.append(ixz11);
			indices.append(ixz01);

			// ponting next vertex on the row
			vertexIndex += 1;
		}
		// ponting first vertex on next row: bypassing the border subject to a seam (on the left and on the right) against a less defined mesh
		vertexIndex += deviationToFirstVertxeOfNextRow;
	}
	
	// inserting triangles on the border subject to a seam (left, right, top, bottom)

	if (seams & SEAM_LEFT)
	{
		//		x ==>
		//	z	0 . 1
		//	|	|\  .
		//	-	| \ .
		//		|  \.
		//		(2)|   3
		//		|  /.
		//		| / .
		//		|/  .
		//		4 . 5

		vertexIndex = 0;	// positioning on first vertex of first seam on the lower, left corner of the mesh (0): first vertex of first row
		int numberOfSeamsOnTheLeft = numVerticesPerChuckSide / 2;

		for (int idxSeam = 0; idxSeam < numberOfSeamsOnTheLeft; idxSeam++)
		{
			int i0 = vertexIndex;											// current vertex
			int i1 = vertexIndex + 1;										// next vertex on same row
			int i3 = vertexIndex + numVerticesPerChuckSide + 2;				// next vertex on next row
			int i4 = vertexIndex + (numVerticesPerChuckSide + 1) * 2;		// same vertex 2 row below
			int i5 = i4 + 1;												// next vertex 2 row below

			// triangle against left chunk with higher lod index (one quad split lesser)
			indices.append(i0);
			indices.append(i3);
			indices.append(i4);

			// if not on first seam from bottom to top we need this triangle to fill the hole
			// instead if we are on the first seam on the left and we dont have a seam on the bottom we need this triangle too to fill the hole (otherwise the bottom seam will fill the hole)
			if (idxSeam != 0 || !(seams & SEAM_BOTTOM))
			{
				indices.append(i0);
				indices.append(i1);
				indices.append(i3);
			}

			// if not on last seam from bottom to top we need this triangle to fill the hole
			// instead if we are on the last seam on the left and we dont have a seam on the top we need this triangle too to fill the hole (otherwise the top seam will fill the hole)
			if (idxSeam != numberOfSeamsOnTheLeft - 1 || !(seams & SEAM_TOP))
			{
				indices.append(i3);
				indices.append(i5);
				indices.append(i4);
			}

			vertexIndex = i4;	// positioning on first vertex of next seam going uppward
		}
	}

	if (seams & SEAM_RIGHT)
	{
		//		x ==>
		//	z	0 . 1
		//	|	.  /|
		//	-	. / |
		//		./  |
		//		2 | (3)
		//		.\  |
		//		. \ |
		//		.  \|
		//		4 . 5

		vertexIndex = numVerticesPerChuckSide - 1;	// positioning on first vertex of first seam on the lower, right corner of the mesh (0): penultimate vertex of first row
		int numberOfSeamsOnTheRight = numVerticesPerChuckSide / 2;

		for (int idxSeam = 0; idxSeam < numberOfSeamsOnTheRight; idxSeam++)
		{
			int i0 = vertexIndex;											// current vertex
			int i1 = vertexIndex + 1;										// next vertex on same row
			int i2 = vertexIndex + numVerticesPerChuckSide + 1;				// same vertex on next row
			int i4 = vertexIndex + (numVerticesPerChuckSide + 1) * 2;		// same vertex 2 row below
			int i5 = i4 + 1;												// next vertex 2 row below

			// triangle against left chunk with higher lod index (one quad split lesser)
			indices.append(i1);
			indices.append(i5);
			indices.append(i2);

			// if not on first seam from bottom to top we need this triangle to fill the hole
			// instead if we are on the first seam on the right and we dont have a seam on the bottom we need this triangle too to fill the hole (otherwise the bottom seam will fill the hole)
			if (idxSeam != 0 || !(seams & SEAM_BOTTOM))
			{
				indices.append(i0);
				indices.append(i1);
				indices.append(i2);
			}

			// if not on last seam from bottom to top we need this triangle to fill the hole
			// instead if we are on the last seam on the right and we dont have a seam on the top we need this triangle too to fill the hole (otherwise the top seam will fill the hole)
			if (idxSeam != numberOfSeamsOnTheRight - 1 || !(seams & SEAM_TOP))
			{
				indices.append(i2);
				indices.append(i5);
				indices.append(i4);
			}

			vertexIndex = i4;	// positioning on first vertex of next seam going uppward
		}
	}

	if (seams & SEAM_BOTTOM)
	{
		//		x ==>
		//		   (1)
		//	z	0------ - 2
		//	|	.\     /.
		//	-	. \   / .
		//		.  \ /  .
		//		3 . 4 . 5

		vertexIndex = 0;	// positioning on first vertex of first seam on the lower, left corner of the mesh (0): first vertex of first row
		int numberOfSeamsOnTheBottom = numVerticesPerChuckSide / 2;

		for (int idxSeam = 0; idxSeam < numberOfSeamsOnTheBottom; idxSeam++)
		{
			int i0 = vertexIndex;											// current vertex
			int i2 = vertexIndex + 2;										// moving 2 vertex on the right on same row
			int i3 = vertexIndex + numVerticesPerChuckSide + 1;				// same vertex on next row
			int i4 = i3 + 1;												// next vertex on next row
			int i5 = i4 + 1;												// moving 2 vertex on the right on next row

			// triangle against left chunk with higher lod index (one quad split lesser)
			indices.append(i0);
			indices.append(i2);
			indices.append(i4);

			// if not on first seam from bottom to top we need this triangle to fill the hole
			// instead if we are on the first seam on the left and we dont have a seam on the bottom we need this triangle too to fill the hole (otherwise the bottom seam will fill the hole)
			if (idxSeam != 0 || !(seams & SEAM_LEFT))
			{
				indices.append(i0);
				indices.append(i4);
				indices.append(i3);
			}

			// if not on last seam from bottom to top we need this triangle to fill the hole
			// instead if we are on the last seam on the left and we dont have a seam on the top we need this triangle too to fill the hole (otherwise the top seam will fill the hole)
			if (idxSeam != numberOfSeamsOnTheBottom - 1 || !(seams & SEAM_RIGHT))
			{
				indices.append(i2);
				indices.append(i5);
				indices.append(i4);
			}

			vertexIndex = i2;	// positioning on first vertex of next seam going uppward
		}
	}

	if (seams & SEAM_TOP)
	{
		//		x ==>
		//	z	0 . 1 . 2
		//	|	.  / \  .
		//	-	. /   \ .
		//		./     \.
		//		3------ - 5
		//		   (4)

		vertexIndex = (numVerticesPerChuckSide - 1) * (numVerticesPerChuckSide + 1);	// positioning on first vertex of first seam on the upper, left corner of the mesh (0): first vertex of penultimate row
		int numberOfSeamsOnTheTop = numVerticesPerChuckSide / 2;

		for (int idxSeam = 0; idxSeam < numberOfSeamsOnTheTop; idxSeam++)
		{
			int i0 = vertexIndex;											// current vertex
			int i1 = vertexIndex + 1;										// next vertex on same row
			int i2 = vertexIndex + 2;										// moving 2 vertex on the right on same row
			int i3 = vertexIndex + numVerticesPerChuckSide + 1;				// same vertex on next row
			int i5 = i3 + 2;												// moving 2 vertex on the right on next row

			// triangle against left chunk with higher lod index (one quad split lesser)
			indices.append(i3);
			indices.append(i1);
			indices.append(i5);

			// if not on first seam from bottom to top we need this triangle to fill the hole
			// instead if we are on the first seam on the left and we dont have a seam on the bottom we need this triangle too to fill the hole (otherwise the bottom seam will fill the hole)
			if (idxSeam != 0 || !(seams & SEAM_LEFT))
			{
				indices.append(i0);
				indices.append(i1);
				indices.append(i3);
			}

			// if not on last seam from bottom to top we need this triangle to fill the hole
			// instead if we are on the last seam on the left and we dont have a seam on the top we need this triangle too to fill the hole (otherwise the top seam will fill the hole)
			if (idxSeam != numberOfSeamsOnTheTop - 1 || !(seams & SEAM_RIGHT))
			{
				indices.append(i1);
				indices.append(i2);
				indices.append(i5);
			}

			vertexIndex = i2;	// positioning on first vertex of next seam going uppward
		}
	}

	return;
}
