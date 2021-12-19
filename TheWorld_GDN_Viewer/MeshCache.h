#pragma 
#include <map>

#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>
#include <Mesh.hpp>

namespace godot
{

	class MeshCache
	{
	public:
#define SEAM_LEFT			1
#define SEAM_RIGHT			2
#define SEAM_BOTTOM			4
#define SEAM_TOP			8
#define SEAM_CONFIG_COUNT	16

		// lod ==> Mesh
		typedef std::map<int, Mesh*> _lodMesh;
		// seam ==> _lodMesh (lod ==> Mesh)
		typedef std::map<int, _lodMesh*> _mesh;

		MeshCache(Node *viewer);
		~MeshCache();
		
		void initCache(int numVerticesPerChuckSide, int numLods);

	private:
		_mesh m_mesh;
		Node* m_viewer;
	};

}