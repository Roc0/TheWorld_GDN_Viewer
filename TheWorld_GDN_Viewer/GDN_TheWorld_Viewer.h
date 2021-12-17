#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>

namespace godot {

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
		void _init(); // our initializer called by Godot
		void _ready();
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);

		//
		// Test
		//
		void debugPrint(String message) { if (m_isDebugEnabled) Godot::print(message); }
		String hello(String target1, String target2, int target3) { return String("Test, {0} {1} {2}!").format(Array::make(target1, target2, target3)); };

		bool isDebugEnabled(void) { return m_isDebugEnabled; }
		void setDebugEnabled(bool b = true) { m_isDebugEnabled = b; }
	
	private:
		bool m_isDebugEnabled;
	};

}

