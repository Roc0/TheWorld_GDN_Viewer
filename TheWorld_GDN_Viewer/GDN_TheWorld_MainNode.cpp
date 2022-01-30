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
	register_method("globals", &GDN_TheWorld_MainNode::Globals);
}

GDN_TheWorld_MainNode::GDN_TheWorld_MainNode()
{
	m_globals = NULL;
}

GDN_TheWorld_MainNode::~GDN_TheWorld_MainNode()
{
	GDN_TheWorld_Globals* globals = Globals(false);
	if (globals)
	{
		GDN_TheWorld_Viewer* viewer = Globals()->Viewer(false);
		if (viewer)
		{
			//m_pSpaceWorld->queue_free();
			viewer->call_deferred("free");
			viewer = NULL;
		}

		//m_pSpaceWorld->queue_free();
		globals->call_deferred("free");
		globals = NULL;
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
	pWorldMainNode->add_child(this);

	GDN_TheWorld_Globals* globals = GDN_TheWorld_Globals::_new();
	if (globals)
	{
		add_child(globals);
		globals->set_name(THEWORLD_GLOBALS_NODE_NAME);
	}
	else
		return false;

	PLOGI << "TheWorld Main Node Initializing...";

	GDN_TheWorld_Viewer* viewer = GDN_TheWorld_Viewer::_new();
	if (viewer)
	{
		add_child(viewer);
		viewer->set_name(THEWORLD_VIEWER_NODE_NAME);
	}
	else
		return false;

	viewer->init();
	
	PLOGI << "TheWorld Main Node Initialized!";

	return true;
}

GDN_TheWorld_Globals* GDN_TheWorld_MainNode::Globals(bool useCache)
{
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
