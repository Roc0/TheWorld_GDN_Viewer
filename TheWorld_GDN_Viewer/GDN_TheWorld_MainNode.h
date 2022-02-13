#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>

namespace godot
{
	class GDN_TheWorld_Globals;
	class GDN_Template;

	class GDN_TheWorld_MainNode : public Node
	{
		GODOT_CLASS(GDN_TheWorld_MainNode, Node)

	public:
		GDN_TheWorld_MainNode();
		~GDN_TheWorld_MainNode();
		bool init(Node* pMainNode, Node* pWorldMainNode);
		void deinit(void);

		static void _register_methods();

		//
		// Godot Standard Functions
		//
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);

		GDN_TheWorld_Globals* Globals(bool useCache = true);

	private:
		bool m_initialized;

		// Node cache
		GDN_TheWorld_Globals* m_globals;
		//GDN_Template* m_temp;
	};

}
