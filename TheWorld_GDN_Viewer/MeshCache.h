#pragma 
#include <vector>

#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>
#include <ArrayMesh.hpp>

namespace godot
{
	class GDN_TheWorld_Viewer;

	class MeshCache
	{
	public:
#define SEAM_LEFT			1
#define SEAM_RIGHT			2
#define SEAM_BOTTOM			4
#define SEAM_TOP			8
#define SEAM_CONFIG_COUNT	16

		// _lodMesh[lod] ==> Mesh*
		typedef std::vector<Ref<ArrayMesh>> _lodMesh;
		// _meshCache[seams] ==> _lodMesh		_meshCache[seams][lod] ==> Mesh*
		typedef std::vector<_lodMesh> _meshCache;

		MeshCache(GDN_TheWorld_Viewer* viewer);
		~MeshCache();
		
		void initCache(int numVerticesPerChuckSide, int numLods);
		Mesh* getMesh(int lod, int seams);

	private:
		ArrayMesh* makeFlatChunk(int numVerticesPerChuckSide, int lod, int seams);
		void resetCache(void);

	private:
		_meshCache m_meshCache;
		GDN_TheWorld_Viewer* m_viewer;
	};

}