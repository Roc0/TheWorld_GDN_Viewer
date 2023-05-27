#pragma once

#pragma warning(push, 0)
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#pragma warning(pop)

namespace godot
{

	class GDN_Template : public Node
	{
		GDCLASS(GDN_Template, Node)

	protected:
		static void _bind_methods();

	public:
		GDN_Template();
		~GDN_Template();
		void init(void);
		void deinit(void);

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;

		//
		// Test
		//
		String hello(String target1, String target2, int target3)
		{
			return String("Test, {0} {1} {2}!").format(Array::make(target1, target2, target3)); 
		};

	private:
		bool m_initialized;
		bool m_ready;
	};

}

