//#include "pch.h"
#include <Godot.hpp>
#include <Array.hpp>
#include <ResourceLoader.hpp>
#include <InputEvent.hpp>

#include "GDN_TheWorld_Quadrant.h"
#include "QuadTree.h"

namespace godot
{
	GDN_TheWorld_Quadrant::GDN_TheWorld_Quadrant()
	{
		m_quadTree = nullptr;
		m_initialized = false;
	}
	
	GDN_TheWorld_Quadrant::~GDN_TheWorld_Quadrant()
	{
		deinit();
	}

	void GDN_TheWorld_Quadrant::init(QuadTree* quadTree)
	{
		assert(m_initialized == false);

		m_quadTree = quadTree;
		set_meta("QuadrantPos", m_quadTree->getQuadrant()->getPos().getIdStr().c_str());
		m_initialized = true;
	}

	void GDN_TheWorld_Quadrant::deinit(void)
	{
		if (m_initialized)
		{
			m_quadTree = nullptr;
			m_initialized = false;
		}
	}
		
	void GDN_TheWorld_Quadrant::_register_methods()
	{
		register_method("_ready", &GDN_TheWorld_Quadrant::_ready);
		register_method("_process", &GDN_TheWorld_Quadrant::_process);
		register_method("_physics_process", &GDN_TheWorld_Quadrant::_physics_process);
		register_method("_input", &GDN_TheWorld_Quadrant::_input);
		register_method("_notification", &GDN_TheWorld_Quadrant::_notification);
	}

	void GDN_TheWorld_Quadrant::_init(void)
	{
	}

	void GDN_TheWorld_Quadrant::_ready(void)
	{
	}

	void GDN_TheWorld_Quadrant::_input(const Ref<InputEvent> event)
	{
	}

	void GDN_TheWorld_Quadrant::_notification(int p_what)
	{
		switch (p_what)
		{
		case NOTIFICATION_PREDELETE:
		{
		}
		break;
		case NOTIFICATION_ENTER_WORLD:
		{
		}
		break;
		case NOTIFICATION_EXIT_WORLD:
		{
		}
		break;
		case NOTIFICATION_VISIBILITY_CHANGED:
		{
		}
		break;
		case NOTIFICATION_TRANSFORM_CHANGED:
		{
		}
		break;
		}
	}

	void GDN_TheWorld_Quadrant::_physics_process(float _delta)
	{
	}

	void GDN_TheWorld_Quadrant::_process(float _delta)
	{
	}
}

