//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

#include <SceneTree.hpp>

//#include <MapManager.h>
#include <plog/Initializers/RollingFileInitializer.h>

std::string getModuleLoadPath(void);

// Gestire le eccezioni della comunicazione verso il server

const godot::Color godot::GDN_TheWorld_Globals::g_color_white = godot::Color(255.0F / 255.0F, 255.0F / 255.0F, 255.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_red = godot::Color(255.0F / 255.0F, 0.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_dark_red = godot::Color(139.0F / 255.0F, 0.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_red_salmon = godot::Color(250.0F / 255.0F, 128.0F / 255.0F, 114.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink = godot::Color(255.0F / 255.0F, 192.0F / 255.0F, 203.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_pink = godot::Color(255.0F / 255.0F, 182.0F / 255.0F, 193.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_amaranth = godot::Color(159.0F / 255.0F, 43.0F / 255.0F, 104.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_bisque = godot::Color(242.0F / 255.0F, 210.0F / 255.0F, 189.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_cerise = godot::Color(222.0F / 255.0F, 49.0F / 255.0F, 99.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_green = godot::Color(0.0F / 255.0F, 255.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_green1 = godot::Color(0.0F / 255.0F, 128.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pale_green = godot::Color(152.0F / 255.0F, 251.0F / 255.0F, 152.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_green = godot::Color(144.0F / 255.0F, 238.0F / 255.0F, 144.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_darksea_green = godot::Color(143.0F / 255.0F, 188.0F / 255.0F, 143.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_aquamarine_green = godot::Color(127.0F / 255.0F, 255.0F / 255.0F, 212.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_blue = godot::Color(0.0F / 255.0F, 0.0F / 255.0F, 255.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_cyan = godot::Color(0.0F / 255.0F, 255.0F / 255.0F, 255.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_cyan = godot::Color(150.0F / 255.0F, 255.0F / 255.0F, 255.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow = godot::Color(255.0F / 255.0F, 255.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_almond = godot::Color(234.0F / 255.0F, 221.0F / 255.0F, 202.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_amber = godot::Color(255.0F / 255.0F, 191.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_apricot = godot::Color(251.0F / 255.0F, 206.0F / 255.0F, 177.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_black = godot::Color(0.0F / 255.0F, 0.0F / 255.0F, 0.0F / 255.0F);

// DEBUG
//size_t godot::GDN_TheWorld_Globals::s_num = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed1 = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed2 = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed3 = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed4 = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed5 = 0;

namespace godot
{
	std::string GDN_TheWorld_Globals::getClientDataDir(void)
	{
		String userPath = OS::get_singleton()->get_user_data_dir();
		char* s = userPath.alloc_c_string();
		std::string dir = std::string(s) + "\\TheWorld";
		godot::api->godot_free(s);

		return dir;
	}

	GDN_TheWorld_Globals_Client::GDN_TheWorld_Globals_Client(GDN_TheWorld_Globals* globals, plog::Severity sev) : TheWorld_ClientServer::ClientInterface(sev)
	{
		m_globals = globals;
	}

	void GDN_TheWorld_Globals_Client::MapManagerSetLogMaxSeverity(plog::Severity sev)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back((int)sev);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_SETLOGMAXSEVERITY, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, (TheWorld_ClientServer::ClientCallback*)m_globals);
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> MapManager::MapManagerSetLogMaxSeverity error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(),"", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::ServerInitializeSession(plog::Severity sev)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back((int)sev);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, (TheWorld_ClientServer::ClientCallback*)m_globals);
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodAsync ==> Server::initializeSession error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerGetVertices(float viewerPosX, float viewerPosZ, float lowerXGridVertex, float lowerZGridVertex, int anchorType, int numVerticesPerSize, float gridStepinWU, int level, bool setCamera, float cameraDistanceFromTerrain, std::string meshId)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(viewerPosX);
		inputParams.push_back(viewerPosZ);
		inputParams.push_back(lowerXGridVertex);
		inputParams.push_back(lowerZGridVertex);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(level);
		inputParams.push_back(meshId);
		inputParams.push_back(setCamera);
		inputParams.push_back(cameraDistanceFromTerrain);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES, ref, inputParams, THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> MapManager::getVertices error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerUploadBuffer(float lowerXGridVertex, float lowerZGridVertex, int numVerticesPerSize, float gridStepinWU, int level, std::string buffer)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(lowerXGridVertex);
		inputParams.push_back(lowerZGridVertex);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(level);
		inputParams.push_back(buffer);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER, ref, inputParams, THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> MapManager::getVertices error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	GDN_TheWorld_Globals::GDN_TheWorld_Globals()
	{
		m_initialized = false;
		setStatus(TheWorldStatus::uninitialized);
		m_isDebugEnabled = DEFAULT_DEBUG_ENABLED;
		m_bAppInError = false;
		m_lastErrorCode = 0;

		bool b = resize(THEWORLD_VIEWER_CHUNK_SIZE_SHIFT, THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT, true);
		assert(b);

		//m_mapManager = NULL;
		m_client = nullptr;
		m_gridStepInWU = 0;

		m_viewer = NULL;
	}

	GDN_TheWorld_Globals::~GDN_TheWorld_Globals()
	{
		deinit();
	}

	void GDN_TheWorld_Globals::replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply)
	{
		std::string method = reply.getMethod();

		if (reply.error())
		{
			setStatus(TheWorldStatus::error);
			errorPrint((std::string("GDN_TheWorld_Globals::replyFromServer: error method ") + method + std::string(" rc=") + std::to_string(reply.getErrorCode()) + std::string(" ") + reply.getErrorMessage()).c_str());
		}

		try
		{
			if (method == THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION)
			{
				if (reply.getNumReplyParams() != 1)
				{
					std::string m = std::string("Server::initializeSession: error reply params ");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", 0));
				}

				ClientServerVariant v = reply.getReplyParam(0);
				const auto ptr(std::get_if<float>(&v));
				if (ptr == NULL)
				{
					std::string m = std::string("Server::initializeSession did not return a float");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}

				m_gridStepInWU = *ptr;

				Viewer()->init();

				setStatus(TheWorldStatus::sessionInitialized);
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_SETLOGMAXSEVERITY)
			{
				method = "to remove";
			}
			else
			{
				setStatus(TheWorldStatus::error);
				errorPrint((std::string("GDN_TheWorld_Globals::replyFromServer: unknow method ") + method).c_str());
			}
		}
		catch (GDN_TheWorld_Exception ex)
		{
			setStatus(TheWorldStatus::error);
			errorPrint(String(ex.exceptionName()) + String(" caught - ") + ex.what());
		}
		catch (...)
		{
			setStatus(TheWorldStatus::error);
			errorPrint("GDN_TheWorld_Globals::replyFromServer: exception caught");
		}
	}

	bool GDN_TheWorld_Globals::resize(int chunkSizeShift, int heightmapResolutionShift, bool force)
	{
		if (chunkSizeShift == -1 || heightmapResolutionShift == -1)
			return false;

		if (!force && m_chunkSizeShift == chunkSizeShift && m_heightmapResolutionShift == heightmapResolutionShift)
			return false;

		m_chunkSizeShift = chunkSizeShift;
		m_heightmapResolutionShift = heightmapResolutionShift;
		m_numVerticesPerChuckSide = 1 << m_chunkSizeShift;
		m_heightmapResolution = 1 << m_heightmapResolutionShift;
		m_lodMaxDepth = m_heightmapResolutionShift - m_chunkSizeShift;
		m_numLods = m_lodMaxDepth + 1;
		return true;
	}

	void GDN_TheWorld_Globals::init(void)
	{
		string logPath = getModuleLoadPath() + "\\TheWorld_Viewer_log.txt";
		plog::Severity sev = PLOG_DEFAULT_LEVEL;
		if (m_isDebugEnabled)
			sev = PLOG_DEBUG_LEVEL;
		plog::init(sev, logPath.c_str(), 1000000, 3);
		PLOG(plog::get()->getMaxSeverity()) << "***************";
		PLOG(plog::get()->getMaxSeverity()) << "Log initilized!";
		PLOG(plog::get()->getMaxSeverity()) << "***************";

		TheWorld_Utils::Utils::plogInit(sev, plog::get());

		PLOGI << "TheWorld Globals Initializing...";

		//setDebugEnabled(m_isDebugEnabled);

		m_initialized = true;
		setStatus(TheWorldStatus::initialized);
		PLOGI << "TheWorld Globals Initialized!";
	}

	void GDN_TheWorld_Globals::preDeinit(void)
	{
	}

	bool GDN_TheWorld_Globals::canDeinit(void)
	{
		return true;
	}

	void GDN_TheWorld_Globals::deinit(void)
	{
		if (m_initialized)
		{
			PLOGI << "TheWorld Globals Deinitializing...";

			PLOGI << "TheWorld Globals Deinitialized!";

			PLOG(plog::get()->getMaxSeverity()) << "*****************";
			PLOG(plog::get()->getMaxSeverity()) << "Log Terminated!";
			PLOG(plog::get()->getMaxSeverity()) << "*****************";

			TheWorld_Utils::Utils::plogDenit();

			m_initialized = false;
			setStatus(TheWorldStatus::uninitialized);

			debugPrint("GDN_TheWorld_Globals::deinit DONE!");
		}
	}

	float GDN_TheWorld_Globals::gridStepInWU(void)
	{
		return m_gridStepInWU;
	}

	void GDN_TheWorld_Globals::_init(void)
	{
		//debugPrint("GDN_TheWorld_Globals::_init");
	}

	void GDN_TheWorld_Globals::connectToServer(void)
	{
		if (status() != TheWorldStatus::initialized)
			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Connect to server failed: not valid status, status=") + std::to_string((size_t)status())).c_str()));

		if (m_client != nullptr)
			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Connect to server failed: pointer to client is not null").c_str())));

		plog::Severity sev = PLOG_DEFAULT_LEVEL;
		if (m_isDebugEnabled)
			sev = PLOG_DEBUG_LEVEL;

		m_client = new GDN_TheWorld_Globals_Client(this, sev);
		int rc = m_client->connect();
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
			throw(GDN_TheWorld_Exception(__FUNCTION__, "Connect to server failed with code " + rc));

		setStatus(TheWorldStatus::connectedToServer);

		infoPrint("GDN_TheWorld_Globals::connectToServer DONE!");

		m_client->ServerInitializeSession(sev);
	}

	void GDN_TheWorld_Globals::prepareDisconnectFromServer(void)
	{
		if ((int)status() >= (int)TheWorldStatus::connectedToServer)
		{
			if (m_client != nullptr)
				m_client->prepareDisconnect();
		}
	}

	bool GDN_TheWorld_Globals::canDisconnectFromServer(void)
	{
		return m_client->canDisconnect();
	}

	void GDN_TheWorld_Globals::disconnectFromServer(void)
	{
		if ((int)status() < (int)TheWorldStatus::connectedToServer)
			return;
			//throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Disconnect from server failed: not valid status, status=") + std::to_string((size_t)status())).c_str()));

		if (m_client == nullptr)
			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Disconnect from server failed: pointer to client is null").c_str())));

		m_client->disconnect();
		delete m_client;
		m_client = nullptr;

		infoPrint("GDN_TheWorld_Globals::disconnectFromServer DONE!");

		setStatus(TheWorldStatus::initialized);
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
		if (m_client == nullptr)
			return;
		
		m_isDebugEnabled = b;

		plog::Severity sev = PLOG_DEFAULT_LEVEL;
		if (b)
			sev = PLOG_DEBUG_LEVEL;

		plog::get()->setMaxSeverity(sev);
		PLOG(plog::get()->getMaxSeverity()) << "Log severity changed to: " << std::to_string(sev);
		TheWorld_Utils::Utils::plogSetMaxSeverity(sev);
		m_client->MapManagerSetLogMaxSeverity(sev);
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
