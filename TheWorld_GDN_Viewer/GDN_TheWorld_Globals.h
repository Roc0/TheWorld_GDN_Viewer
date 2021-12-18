#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>

#define THEWORLD_CHUNK_SIZE	32

namespace godot
{

	class GDN_TheWorld_Globals : public Node
	{
		GODOT_CLASS(GDN_TheWorld_Globals, Node)

	public:
		GDN_TheWorld_Globals();
		~GDN_TheWorld_Globals();

		static void _register_methods();

		//
		// Godot Standard Functions
		//
		void _init(); // our initializer called by Godot
		void _ready();
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);

		int chuckSize(void) { return m_chunkSize; }

	private:
		int m_chunkSize;	// Chunk num vertices
	};

}


