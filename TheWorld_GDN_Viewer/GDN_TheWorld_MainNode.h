#pragma once

#pragma warning (push, 0)
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/engine.hpp>
#pragma warning (pop)

namespace godot
{
	class GDN_TheWorld_Globals;
	class GDN_Template;

	class GDN_TheWorld_MainNode : public Node
	{
		GDCLASS(GDN_TheWorld_MainNode, Node)

	public:
		GDN_TheWorld_MainNode();
		~GDN_TheWorld_MainNode();
		bool init(Node3D* pWorldMainNode, bool isInEditor);
		void preDeinit(void);
		bool canDeinit(void);
		void deinit(void);
		bool initialized(void)
		{
			return m_initialized;
		}

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;

		GDN_TheWorld_Globals* Globals(bool useCache = true);

	protected:
		static void _bind_methods();
		virtual void _notification(int p_what);
		void _init(void);

	private:
		bool m_initialized;
		bool m_ready;
		bool m_initInProgress;

		// Node cache
		GDN_TheWorld_Globals* m_globals;
	};
}
