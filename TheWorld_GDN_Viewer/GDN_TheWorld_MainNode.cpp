//#include "pch.h"
#include "GDN_TheWorld_MainNode.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

using namespace godot;

void GDN_TheWorld_MainNode::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_MainNode::_ready);
	register_method("_process", &GDN_TheWorld_MainNode::_process);
	register_method("_input", &GDN_TheWorld_MainNode::_input);

	register_method("init", &GDN_TheWorld_MainNode::init);
	register_method("deinit", &GDN_TheWorld_MainNode::deinit);
	register_method("globals", &GDN_TheWorld_MainNode::Globals);
}

GDN_TheWorld_MainNode::GDN_TheWorld_MainNode()
{
	m_initialized = false;
	m_globals = NULL;
}

GDN_TheWorld_MainNode::~GDN_TheWorld_MainNode()
{
	deinit();
}

void GDN_TheWorld_MainNode::deinit(void)
{
	if (m_initialized)
	{
		PLOGI << "TheWorld Main Node Deinitializing...";

		GDN_TheWorld_Globals* globals = Globals();
		if (globals)
		{
			GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
			if (viewer)
			{
				Node* parent = viewer->get_parent();
				if (parent)
					parent->remove_child(viewer);
				viewer->deinit();
				viewer->queue_free();
				//viewer->call_deferred("free");
			}
			PLOGI << "TheWorld Main Node Deinitialized!";
			
			Node* parent = globals->get_parent();
			if (parent)
				parent->remove_child(globals);
			globals->deinit();
			globals->queue_free();
			//globals->call_deferred("free");
		}

		//queue_free();
		//call_deferred("free");
		m_initialized = false;
	}
}

void GDN_TheWorld_MainNode::_init(void)
{
	//Godot::print("GD_ClientApp::Init");
}

void GDN_TheWorld_MainNode::_ready(void)
{
	//Godot::print("GD_ClientApp::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_TheWorld_MainNode::_input(const Ref<InputEvent> event)
{
}

void GDN_TheWorld_MainNode::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GD_ClientApp::_process");
}

bool GDN_TheWorld_MainNode::init(Node* pWorldMainNode)
{
	if (!pWorldMainNode)
		return false;

	// Must exist a Node acting as the world; the viewer will be a child of this node
	//pWorldMainNode->add_child(this);		/* DEBUG */

	GDN_TheWorld_Globals* globals = GDN_TheWorld_Globals::_new();
	if (globals)
	{
		/* DEBUG */
		pWorldMainNode->add_child(globals);		// critical poin on exit
		//Node* parent = globals->get_parent();
		//if (parent)
		//	parent->remove_child(globals);
		/* DEBUG */
		globals->set_name(THEWORLD_GLOBALS_NODE_NAME);
		m_globals = globals;
	}
	else
		return false;

	PLOGI << "TheWorld Main Node Initializing...";

	/* DEBUG */
	/*
	GDN_TheWorld_Viewer* viewer = GDN_TheWorld_Viewer::_new();
	if (viewer)
	{
		pWorldMainNode->add_child(viewer);
		viewer->set_name(THEWORLD_VIEWER_NODE_NAME);
	}
	else
		return false;

	viewer->init();
	*/
	/* DEBUG */

	m_initialized = true;
	PLOGI << "TheWorld Main Node Initialized!";

	return true;
}

GDN_TheWorld_Globals* GDN_TheWorld_MainNode::Globals(bool useCache)
{
	if (m_globals == NULL)
		return NULL;
	
	if (m_globals == NULL || !useCache)
	{
		SceneTree* scene = get_tree();
		if (!scene)
			return NULL;
		Viewport* root = scene->get_root();
		if (!root)
			return NULL;
		m_globals = Object::cast_to<GDN_TheWorld_Globals>(root->find_node(THEWORLD_GLOBALS_NODE_NAME, true, false));
		//m_globals = Object::cast_to<GDN_TheWorld_Globals>(get_tree()->get_root()->find_node(THEWORLD_GLOBALS_NODE_NAME, true, false));
	}

	return m_globals;
}
