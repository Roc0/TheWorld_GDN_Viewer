//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

#include <MapManager.h>
#include <plog/Initializers/RollingFileInitializer.h>

std::string getModuleLoadPath(void);

const godot::Color godot::GDN_TheWorld_Globals::g_color_white = godot::Color(255, 255, 255); 
const godot::Color godot::GDN_TheWorld_Globals::g_color_red = godot::Color(255, 0, 0);
const godot::Color godot::GDN_TheWorld_Globals::g_color_dark_red = godot::Color(139, 0, 0);
const godot::Color godot::GDN_TheWorld_Globals::g_color_red_salmon = godot::Color(250, 128, 114);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink = godot::Color(255, 192, 203);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_pink = godot::Color(255, 182, 193);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_amaranth = godot::Color(159, 43, 104);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_bisque = godot::Color(242, 210, 189);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_cerise = godot::Color(222, 49, 99);
const godot::Color godot::GDN_TheWorld_Globals::g_color_green = godot::Color(0, 255, 0);
const godot::Color godot::GDN_TheWorld_Globals::g_color_green1 = godot::Color(0, 128, 0);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pale_green = godot::Color(152, 251, 152);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_green = godot::Color(144, 238, 144);
const godot::Color godot::GDN_TheWorld_Globals::g_color_darksea_green = godot::Color(143, 188, 143);
const godot::Color godot::GDN_TheWorld_Globals::g_color_aquamarine_green = godot::Color(127, 255, 212);
const godot::Color godot::GDN_TheWorld_Globals::g_color_blue = godot::Color(0, 0, 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_cyan = godot::Color(0, 255, 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_cyan = godot::Color(150, 255, 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow = godot::Color(255, 255, 0);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_almond = godot::Color(234, 221, 202);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_amber = godot::Color(255, 191, 0);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_apricot = godot::Color(251, 206, 177);
const godot::Color godot::GDN_TheWorld_Globals::g_color_black = godot::Color(0, 0, 0);

namespace godot
{
	GDN_TheWorld_Globals::GDN_TheWorld_Globals()
	{
		m_initialized = false;
		m_isDebugEnabled = false;
		m_bAppInError = false;
		m_lastErrorCode = 0;

		m_mapManager = NULL;

		m_numVerticesPerChuckSide = 1 << THEWORLD_VIEWER_CHUNK_SIZE_SHIFT;
		m_bitmapResolution = 1 << THEWORLD_VIEWER_BITMAP_RESOLUTION_SHIFT;
		m_lodMaxDepth = THEWORLD_VIEWER_BITMAP_RESOLUTION_SHIFT - THEWORLD_VIEWER_CHUNK_SIZE_SHIFT;
		m_numLods = m_lodMaxDepth + 1;

		m_viewer = NULL;
	}

	void GDN_TheWorld_Globals::init(void)
	{
		string logPath = getModuleLoadPath() + "\\TheWorld_Viewer_log.txt";
		plog::init(PLOG_DEFAULT_LEVEL, logPath.c_str(), 1000000, 3);
		PLOG_INFO << "***************";
		PLOG_INFO << "Log initilized!";
		PLOG_INFO << "***************";

		PLOGI << "TheWorld Globals Initializing...";

		m_mapManager = new TheWorld_MapManager::MapManager(NULL, PLOG_DEFAULT_LEVEL, plog::get());
		m_mapManager->instrument(true);
		m_mapManager->consoleDebugMode(false);

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

			m_initialized = false;
			
			debugPrint("GDN_TheWorld_Globals::deinit DONE!");
		}
	}

	void GDN_TheWorld_Globals::_init(void)
	{
		//debugPrint("GDN_TheWorld_Globals::_init");
	}

	void GDN_TheWorld_Globals::_ready(void)
	{
		debugPrint("GDN_TheWorld_Globals::_ready");
		//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
	}

	void GDN_TheWorld_Globals::_input(const Ref<InputEvent> event)
	{
	}

	void GDN_TheWorld_Globals::_notification(int p_what)
	{
		switch (p_what)
		{
		case NOTIFICATION_PREDELETE:
		{
			debugPrint("GDN_TheWorld_Globals::_notification - Destroy Globals");
		}
		break;
		}
	}

	void GDN_TheWorld_Globals::_process(float _delta)
	{
		// To activate _process method add this Node to a Godot Scene
		//debugPrint("GDN_TheWorld_Globals::_process");
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

	void GDN_TheWorld_Globals::setDebugEnabled(bool b)
	{
		m_isDebugEnabled = b;
		
		if (b)
		{
			plog::get()->setMaxSeverity(PLOG_DEBUG_LEVEL);
			m_mapManager->setLogMaxSeverity(PLOG_DEBUG_LEVEL);
		}
		else
		{
			plog::get()->setMaxSeverity(PLOG_DEFAULT_LEVEL);
			m_mapManager->setLogMaxSeverity(PLOG_DEFAULT_LEVEL);
		}
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
