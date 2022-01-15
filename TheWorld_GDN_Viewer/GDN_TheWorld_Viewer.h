#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Spatial.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>

namespace godot
{
	class MeshCache;
	class GDN_TheWorld_Globals;

	class GDN_TheWorld_Viewer : public Spatial
	{
		GODOT_CLASS(GDN_TheWorld_Viewer, Spatial)

	public:
		GDN_TheWorld_Viewer();
		~GDN_TheWorld_Viewer();

		static void _register_methods();

		//
		// Godot Standard Functions
		//
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);

		void debugPrint(String message) { if (m_isDebugEnabled) Godot::print(message); }
		bool isDebugEnabled(void) { return m_isDebugEnabled; }
		void setDebugEnabled(bool b = true) { m_isDebugEnabled = b; }
		void destroy(void);
		bool init(Node* pWorldNode);

		GDN_TheWorld_Globals* Globals(void) { return m_globals; }
	
	private:
		bool m_isDebugEnabled;
		GDN_TheWorld_Globals* m_globals;
		MeshCache* m_meshCache;
	};

}

