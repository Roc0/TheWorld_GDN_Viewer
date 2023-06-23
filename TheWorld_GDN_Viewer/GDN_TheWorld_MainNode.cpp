//#include "pch.h"
#include "GDN_TheWorld_MainNode.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_Template.h"

#pragma warning (push, 0)
//#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#pragma warning (pop)

using namespace godot;

void GDN_TheWorld_MainNode::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("init", "p_world_main_node"), &GDN_TheWorld_MainNode::init);
	ClassDB::bind_method(D_METHOD("pre_deinit"), &GDN_TheWorld_MainNode::preDeinit);
	ClassDB::bind_method(D_METHOD("can_deinit"), &GDN_TheWorld_MainNode::canDeinit);
	ClassDB::bind_method(D_METHOD("deinit"), &GDN_TheWorld_MainNode::deinit);
	ClassDB::bind_method(D_METHOD("globals"), &GDN_TheWorld_MainNode::Globals);
	ClassDB::bind_method(D_METHOD("initialized"), &GDN_TheWorld_MainNode::initialized);
}

GDN_TheWorld_MainNode::GDN_TheWorld_MainNode()
{
	m_initialized = false;
	m_ready = false;
	m_initInProgress = false;
	m_globals = NULL;

	_init();
}

GDN_TheWorld_MainNode::~GDN_TheWorld_MainNode()
{
	deinit();
}

void GDN_TheWorld_MainNode::_init(void)
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//godot::UtilityFunctions::print("GDN_TheWorld_MainNode::_init");

	set_name(THEWORLD_MAIN_NODE_NAME);
}

void GDN_TheWorld_MainNode::_ready(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("GDN_TheWorld_MainNode::_ready");

	m_ready = true;
}

void GDN_TheWorld_MainNode::_notification(int p_what)
{
	switch (p_what)
	{
	case NOTIFICATION_PREDELETE:
	{
		if (m_ready)
		{
			GDN_TheWorld_Globals* globals = Globals();
			if (globals != nullptr)
				globals->debugPrint("GDN_TheWorld_MainNode::_notification (NOTIFICATION_PREDELETE) - Destroy Main Node");
		}
	}
	break;
	}
}

void GDN_TheWorld_MainNode::_input(const Ref<InputEvent>& event)
{
	//Globals()->debugPrint("GDN_TheWorld_MainNode::_ready");
}

void GDN_TheWorld_MainNode::_process(double _delta)
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

	//	PLOG_INFO << "TheWorld Main Node Initializing...";

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

	//	PLOG_INFO << "TheWorld Main Node Initialized!";
	//}
}

bool GDN_TheWorld_MainNode::init(Node3D* pWorldMainNode, bool isInEditor)
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
	//if (isInEditor)		// it should be fixed in https://github.com/godotengine/godot/pull/77633 commit 31/05
	if (IS_EDITOR_HINT())
		sceneRoot = scene->get_edited_scene_root();
	else
		sceneRoot = scene->get_root();

	set_owner(sceneRoot);

	m_initInProgress = true;

	GDN_TheWorld_Globals* globals = Globals(false);
	if (globals == nullptr)
		globals = memnew(GDN_TheWorld_Globals);
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
		globals->init(isInEditor);
		globals->set_owner(sceneRoot);
	}
	else
		return false;

	PLOG_INFO << "TheWorld Main Node Initializing...";

	GDN_TheWorld_Viewer* viewer = globals->Viewer(false);
	if (viewer == nullptr)
		viewer = memnew(GDN_TheWorld_Viewer);
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
		Transform3D gt = pWorldMainNode->get_global_transform();
		viewer->set_global_transform(gt);
		viewer->set_owner(sceneRoot);
	}
	else
		return false;

	m_initInProgress = false;
	m_initialized = true;
	PLOG_INFO << "TheWorld Main Node Initialized!";

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
		PLOG_INFO << "TheWorld Main Node Deinitializing...";

		GDN_TheWorld_Globals* globals = Globals();
		if (globals)
		{
			GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
			if (viewer)
			{
				viewer->deinit();
				Node* parent = viewer->get_parent();
				if (parent)
				{
					//parent->call_deferred("remove_child", viewer);	// TESTING
					//parent->remove_child(viewer);
				}
				//viewer->set_owner(nullptr);							// TESTING
				//viewer->queue_free();									// TESTING
			}
			PLOG_INFO << "TheWorld Main Node Deinitialized!";

			globals->deinit();
			Node* parent = globals->get_parent();
			if (parent)
			{
				//parent->call_deferred("remove_child", globals);		// TESTING
				//parent->remove_child(globals);
			}
			//globals->set_owner(nullptr);								// TESTING
			//globals->queue_free();									// TESTING
		}

		m_initialized = false;

		Globals()->debugPrint("GDN_TheWorld_MainNode::deinit DONE!");

		Node* parent = get_parent();
		if (parent)
		{
			//parent->call_deferred("remove_child", this);				// TESTING
			//parent->remove_child(this);
		}
		//set_owner(nullptr);											// TESTING
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
		m_globals = Object::cast_to<GDN_TheWorld_Globals>(root->find_child(THEWORLD_GLOBALS_NODE_NAME, true, false));
	}

	return m_globals;
}
