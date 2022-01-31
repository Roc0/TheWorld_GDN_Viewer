//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

#include <MapManager.h>
#include <plog/Initializers/RollingFileInitializer.h>

std::string getModuleLoadPath(void);

namespace godot
{
	GDN_TheWorld_Globals::GDN_TheWorld_Globals()
	{
		m_initialized = false;
		m_isDebugEnabled = false;
		m_bAppInError = false;
		m_lastErrorCode = 0;

		string logPath = getModuleLoadPath() + "\\TheWorld_Viewer_log.txt";
		plog::init(PLOG_LEVEL, logPath.c_str(), 1000000, 3);
		PLOG_INFO << "***************";
		PLOG_INFO << "Log initilized!";
		PLOG_INFO << "***************";

		PLOGI << "TheWorld Globals Initializing...";

		m_mapManager = new TheWorld_MapManager::MapManager(NULL, PLOG_LEVEL, plog::get());
		m_mapManager->instrument(true);
		m_mapManager->consoleDebugMode(false);

		m_numVerticesPerChuckSide = 1 << THEWORLD_VIEWER_CHUNK_SIZE_SHIFT;
		m_bitmapResolution = 1 << THEWORLD_VIEWER_BITMAP_RESOLUTION_SHIFT;
		m_lodMaxDepth = THEWORLD_VIEWER_BITMAP_RESOLUTION_SHIFT - THEWORLD_VIEWER_CHUNK_SIZE_SHIFT;
		m_numLods = m_lodMaxDepth + 1;

		m_viewer = NULL;

		m_initialized = true;
		PLOGI << "TheWorld Globals Initialized!";
	}

	GDN_TheWorld_Globals::~GDN_TheWorld_Globals()
	{
		deinit();
	}

	void GDN_TheWorld_Globals::deinit(void)
	{
		if (m_initialized)
		{
			PLOGI << "TheWorld Globals Deinitializing...";

			delete m_mapManager;

			PLOGI << "TheWorld Globals Deinitialized!";

			PLOG_INFO << "*****************";
			PLOG_INFO << "Log Terminated!";
			PLOG_INFO << "*****************";

			//call_deferred("free");
			m_initialized = false;
		}
	}

	void GDN_TheWorld_Globals::_init(void)
	{
		//Godot::print("GD_ClientApp::Init");
	}

	void GDN_TheWorld_Globals::_ready(void)
	{
		//Godot::print("GD_ClientApp::_ready");
		//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
	}

	void GDN_TheWorld_Globals::_input(const Ref<InputEvent> event)
	{
	}

	void GDN_TheWorld_Globals::_process(float _delta)
	{
		// To activate _process method add this Node to a Godot Scene
		//Godot::print("GD_ClientApp::_process");
	}

	GDN_TheWorld_Viewer* GDN_TheWorld_Globals::Viewer(bool useCache)
	{
		if (m_viewer == NULL || !useCache)
		{
			SceneTree* scene = get_tree();
			if (!scene)
				return NULL;
			Viewport* root = scene->get_root();
			if (!root)
				return NULL;
			m_viewer = Object::cast_to<GDN_TheWorld_Viewer>(root->find_node(THEWORLD_VIEWER_NODE_NAME, true, false));
		}

		return m_viewer;
	}
}

std::string getModuleLoadPath(void)
{
	char path[MAX_PATH];
	HMODULE hm = NULL;

	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)getModuleLoadPath, &hm) == 0)
	{
		return "";
	}
	if (GetModuleFileNameA(hm, path, sizeof(path)) == 0)
	{
		return "";
	}

	int l = (int)strlen(path);
	for (int i = l; i >= 0; i--)
		if (path[i] == '\\')
		{
			path[i] = '\0';
			break;
		}

	std::string s = path;

	return s;
}
