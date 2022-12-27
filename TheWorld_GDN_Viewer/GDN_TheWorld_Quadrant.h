#pragma once
#include <Godot.hpp>
#include <Spatial.hpp>

namespace godot
{
	class QuadTree;

	class GDN_TheWorld_Quadrant : public Spatial
	{
		GODOT_CLASS(GDN_TheWorld_Quadrant, Spatial)
	public:
		GDN_TheWorld_Quadrant(void);
		~GDN_TheWorld_Quadrant(void);
		void init(QuadTree* quadTree);
		void deinit(void);

		//
		// Godot Standard Functions
		//
		static void _register_methods();
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _physics_process(float _delta);
		void _input(const Ref<InputEvent> event);
		void _notification(int p_what);

	private:
		QuadTree* m_quadTree;
		bool m_initialized;
	};
}

