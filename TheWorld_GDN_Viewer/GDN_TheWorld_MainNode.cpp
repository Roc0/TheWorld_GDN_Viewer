//#include "pch.h"
#include "GDN_TheWorld_MainNode.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_Template.h"

using namespace godot;

void GDN_TheWorld_MainNode::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_MainNode::_ready);
	register_method("_process", &GDN_TheWorld_MainNode::_process);
	register_method("_input", &GDN_TheWorld_MainNode::_input);
	register_method("_notification", &GDN_TheWorld_MainNode::_notification);

	register_method("init", &GDN_TheWorld_MainNode::init);
	register_method("pre_deinit", &GDN_TheWorld_MainNode::preDeinit);
	register_method("can_deinit", &GDN_TheWorld_MainNode::canDeinit);
	register_method("deinit", &GDN_TheWorld_MainNode::deinit);
	register_method("globals", &GDN_TheWorld_MainNode::Globals);
}

GDN_TheWorld_MainNode::GDN_TheWorld_MainNode()
{
	m_initialized = false;
	m_initInProgress = false;
	m_globals = NULL;
}

GDN_TheWorld_MainNode::~GDN_TheWorld_MainNode()
{
	deinit();
}

void GDN_TheWorld_MainNode::_init(void)
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//Godot::print("GDN_TheWorld_MainNode::Init");
	set_name(THEWORLD_MAIN_NODE_NAME);
}

void GDN_TheWorld_MainNode::_ready(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("GDN_TheWorld_MainNode::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_TheWorld_MainNode::_notification(int p_what)
{
	switch (p_what)
	{
	case NOTIFICATION_PREDELETE:
	{
		GDN_TheWorld_Globals* globals = Globals();
		if (globals != nullptr)
			globals->debugPrint("GDN_TheWorld_MainNode::_notification - Destroy Main Node");
	}
	break;
	}
}

void GDN_TheWorld_MainNode::_input(const Ref<InputEvent> event)
{
	//Globals()->debugPrint("GDN_TheWorld_MainNode::_ready");
}

void GDN_TheWorld_MainNode::_process(float _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Globals()->debugPrint("GDN_TheWorld_MainNode::_process");

	//if (!m_initialized && m_initInProgress)
	//{
	//	godot::Spatial* pWorldMainNode = (Spatial*)get_parent();
	//	
	//	GDN_TheWorld_Globals* globals = GDN_TheWorld_Globals::_new();
	//	if (globals)
	//	{
	//		//Node* parent = globals->get_parent();
	//		pWorldMainNode->add_child(globals);
	//		globals->set_name(THEWORLD_GLOBALS_NODE_NAME);
	//		globals->init();
	//		//m_globals = globals;
	//	}

	//	PLOGI << "TheWorld Main Node Initializing...";

	//	GDN_TheWorld_Viewer* viewer = GDN_TheWorld_Viewer::_new();
	//	if (viewer)
	//	{
	//		pWorldMainNode->add_child(viewer);
	//		viewer->set_name(THEWORLD_VIEWER_NODE_NAME);
	//		Transform gt = pWorldMainNode->get_global_transform();
	//		viewer->set_global_transform(gt);
	//		//viewer->init();
	//	}

	//	m_initialized = true;
	//	m_initInProgress = false;

	//	PLOGI << "TheWorld Main Node Initialized!";
	//}
}

bool GDN_TheWorld_MainNode::init(Spatial* pWorldMainNode)
{
	assert(!m_initialized);
	
	// Must exist a Spatial Node acting as the world; globals and the viewer will be a child of it
	if (!pWorldMainNode)
		return false;

	int64_t worldMainNodeId = pWorldMainNode->get_instance_id();

	godot::Node* parent = get_parent();
	if (parent != nullptr)
	{
		int64_t parentId = parent->get_instance_id();
		if (parentId != worldMainNodeId)
		{
			parent->remove_child(this);
			pWorldMainNode->add_child(this);
		}
	}
	else
		pWorldMainNode->add_child(this);
	
	set_name(THEWORLD_MAIN_NODE_NAME);

	SceneTree* scene = get_tree();
	Node* sceneRoot = nullptr;
	if (godot::Engine::get_singleton()->is_editor_hint())
		sceneRoot = scene->get_edited_scene_root();
	else
		sceneRoot = scene->get_root();

	//if (sceneRoot != nullptr)	// outside the editor get_edited_scene_root returns null
	//	set_owner(sceneRoot);

	set_owner(sceneRoot);

	m_initInProgress = true;

	GDN_TheWorld_Globals* globals = Globals(false);
	if (globals == nullptr)
		globals = GDN_TheWorld_Globals::_new();
	if (globals)
	{
		Node* parent = globals->get_parent();
		if (parent != nullptr)
		{
			int64_t parentId = parent->get_instance_id();
			if (parentId != worldMainNodeId)
			{
				parent->remove_child(globals);
				pWorldMainNode->add_child(globals);
			}
		}
		else
			pWorldMainNode->add_child(globals);
		globals->set_name(THEWORLD_GLOBALS_NODE_NAME);
		globals->init();
		globals->set_owner(sceneRoot);
	}
	else
		return false;

	PLOGI << "TheWorld Main Node Initializing...";

	GDN_TheWorld_Viewer* viewer = globals->Viewer(false);
	if (viewer == nullptr)
		viewer = GDN_TheWorld_Viewer::_new();
	if (viewer)
	{
		Node* parent = viewer->get_parent();
		if (parent != nullptr)
		{
			int64_t parentId = parent->get_instance_id();
			if (parentId != worldMainNodeId)
			{
				parent->remove_child(viewer);
				pWorldMainNode->add_child(viewer);
			}
		}
		else
			pWorldMainNode->add_child(viewer);
		viewer->set_name(THEWORLD_VIEWER_NODE_NAME);
		Transform gt = pWorldMainNode->get_global_transform();
		viewer->set_global_transform(gt);
		viewer->set_owner(sceneRoot);
	}
	else
		return false;

	m_initInProgress = false;
	m_initialized = true;
	PLOGI << "TheWorld Main Node Initialized!";

	return true;
}

void GDN_TheWorld_MainNode::preDeinit(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals)
	{
		GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
		if (viewer)
			viewer->preDeinit();
		
		globals->prepareDisconnectFromServer();

		globals->preDeinit();
	}
}

bool GDN_TheWorld_MainNode::canDeinit(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals)
	{
		GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
		if (viewer && !viewer->canDeinit())
			return false;

		if (!globals->canDisconnectFromServer())
			return false;

		return globals->canDeinit();
	}
	else
		return true;
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
				viewer->deinit();
				Node* parent = viewer->get_parent();
				if (parent)
					parent->remove_child(viewer);
				viewer->set_owner(nullptr);
				//viewer->queue_free();
			}
			PLOGI << "TheWorld Main Node Deinitialized!";

			globals->deinit();
			Node* parent = globals->get_parent();
			if (parent)
				parent->remove_child(globals);
			globals->set_owner(nullptr);
			//globals->queue_free();
		}

		m_initialized = false;

		Globals()->debugPrint("GDN_TheWorld_MainNode::deinit DONE!");

		Node* parent = get_parent();
		if (parent)
			parent->remove_child(this);
		set_owner(nullptr);
	}
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
	}

	return m_globals;
}
