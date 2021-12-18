#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>

#include "GDN_TheWorld_Globals.h"

namespace godot
{

	class GDN_TheWorld_Viewer : public Node
	{
		GODOT_CLASS(GDN_TheWorld_Viewer, Node)

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
	
		Node* Globals(void) { return m_globals; }
	
	private:
		bool m_isDebugEnabled;
		GDN_TheWorld_Globals* m_globals;
	};

}

