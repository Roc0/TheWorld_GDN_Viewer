//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

#include <SceneTree.hpp>

//#include <MapManager.h>
#include <plog/Initializers/RollingFileInitializer.h>

std::string getModuleLoadPath(void);

// Gestire le eccezioni della comunicazione verso il server

const godot::Color godot::GDN_TheWorld_Globals::g_color_white = godot::Color(255.0F / 255, 255.0F / 255, 255.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_red = godot::Color(255.0F / 255, 0.0F / 255, 0.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_dark_red = godot::Color(139.0F / 255, 0.0F / 255, 0.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_red_salmon = godot::Color(250.0F / 255, 128.0F / 255, 114.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink = godot::Color(255.0F / 255, 192.0F / 255, 203.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_pink = godot::Color(255.0F / 255, 182.0F / 255, 193.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_amaranth = godot::Color(159.0F / 255, 43.0F / 255, 104.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_bisque = godot::Color(242.0F / 255, 210.0F / 255, 189.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_cerise = godot::Color(222.0F / 255, 49.0F / 255, 99.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_green = godot::Color(0.0F / 255, 255.0F / 255, 0.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_green1 = godot::Color(0.0F / 255, 128.0F / 255, 0.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pale_green = godot::Color(152.0F / 255, 251.0F / 255, 152.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_green = godot::Color(144.0F / 255, 238.0F / 255, 144.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_darksea_green = godot::Color(143.0F / 255, 188.0F / 255, 143.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_aquamarine_green = godot::Color(127.0F / 255, 255.0F / 255, 212.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_blue = godot::Color(0.0F / 255, 0.0F / 255, 255.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_cyan = godot::Color(0.0F / 255, 255.0F / 255, 255.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_cyan = godot::Color(150.0F / 255, 255.0F / 255, 255.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow = godot::Color(255.0F / 255, 255.0F / 255, 0.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_almond = godot::Color(234.0F / 255, 221.0F / 255, 202.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_amber = godot::Color(255.0F / 255, 191.0F / 255, 0.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_apricot = godot::Color(251.0F / 255, 206.0F / 255, 177.0F / 255);
const godot::Color godot::GDN_TheWorld_Globals::g_color_black = godot::Color(0.0F / 255, 0.0F / 255, 0.0F / 255);

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

	//float GDN_TheWorld_Globals_Client::MapManagerGridStepInWU()
	//{
	//	std::vector<ClientServerVariant> replyParams;
	//	std::vector<ClientServerVariant> inputParams;
	//	int rc = execMethodSync("MapManager::gridStepInWU", inputParams, replyParams);
	//	if (rc != THEWORLD_CLIENTSERVER_RC_OK)
	//	{
	//		std::string m = std::string("execMethodSync ==> MapManager::gridStepInWU error ") + std::to_string(rc);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
	//	}
	//	if (replyParams.size() == 0)
	//	{
	//		std::string m = std::string("execMethodSync ==> MapManager::gridStepInWU error ") + std::to_string(rc);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "No param replied", rc));
	//	}

	//	const auto ptr(std::get_if<float>(&replyParams[0]));
	//	if (ptr == NULL)
	//	{
	//		std::string m = std::string("execMethodSync ==> MapManager::gridStepInWU did not return a float");
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
	//	}
	//	
	//	return *ptr;
	//}

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

	//void GDN_TheWorld_Globals_Client::MapManagerCalcNextCoordOnTheGridInWUs(std::vector<float>& inCoords, std::vector<float>& outCoords)
	//{
	//	std::vector<ClientServerVariant> replyParams;
	//	std::vector<ClientServerVariant> inputParams;
	//	for (size_t idx = 0; idx < inCoords.size(); idx++)
	//		inputParams.push_back(inCoords[idx]);
	//	int rc = execMethodSync(THEWORLD_CLIENTSERVER_METHOD_MAPM_CALCNEXTCOORDGETVERTICES, inputParams, replyParams);
	//	if (rc != THEWORLD_CLIENTSERVER_RC_OK)
	//	{
	//		std::string m = std::string("ClientInterface::execMethodSync ==> MapManager::calcNextCoordOnTheGridInWUs error ") + std::to_string(rc);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
	//	}
	//	if (replyParams.size() != inCoords.size())
	//	{
	//		std::string m = std::string("execMethodSync ==> MapManager::calcNextCoordOnTheGridInWUs error (not enough params replied)") + std::to_string(rc);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
	//	}

	//	for (size_t idx = 0; idx < inCoords.size(); idx++)
	//	{
	//		const auto ptr(std::get_if<float>(&replyParams[idx]));
	//		if (ptr == NULL)
	//		{
	//			std::string m = std::string("execMethodSync ==> MapManager::calcNextCoordOnTheGridInWUs did not return a float");
	//			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
	//		}
	//		outCoords.push_back(*ptr);
	//	}
	//}

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

	/*GDN_TheWorld_Globals* GDN_TheWorld_Globals_Client::Globals(bool useCache)
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
	}*/

	GDN_TheWorld_Globals::GDN_TheWorld_Globals()
	{
		m_initialized = false;
		setStatus(TheWorldStatus::uninitialized);
		m_isDebugEnabled = DEFAULT_DEBUG_ENABLED;
		m_bAppInError = false;
		m_lastErrorCode = 0;

		assert(resize(THEWORLD_VIEWER_CHUNK_SIZE_SHIFT, THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT, true));

		//m_mapManager = NULL;
		m_client = nullptr;
		m_gridStepInWU = 0;

		m_viewer = NULL;
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
		plog::init(PLOG_DEFAULT_LEVEL, logPath.c_str(), 1000000, 3);
		PLOG_INFO << "***************";
		PLOG_INFO << "Log initilized!";
		PLOG_INFO << "***************";

		PLOGI << "TheWorld Globals Initializing...";

		//setDebugEnabled(m_isDebugEnabled);

		m_initialized = true;
		setStatus(TheWorldStatus::initialized);
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

			PLOGI << "TheWorld Globals Deinitialized!";

			PLOG_INFO << "*****************";
			PLOG_INFO << "Log Terminated!";
			PLOG_INFO << "*****************";

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

		m_client = new GDN_TheWorld_Globals_Client(this, PLOG_DEFAULT_LEVEL);
		int rc = m_client->connect();
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
			throw(GDN_TheWorld_Exception(__FUNCTION__, "Connect to server failed with code " + rc));

		setStatus(TheWorldStatus::connectedToServer);

		infoPrint("GDN_TheWorld_Globals::connectToServer DONE!");

		plog::Severity sev = PLOG_DEFAULT_LEVEL;
		if (m_isDebugEnabled)
			sev = PLOG_DEBUG_LEVEL;
		m_client->ServerInitializeSession(sev);
	}

	void GDN_TheWorld_Globals::disconnectFromServer(void)
	{
		if (status() != TheWorldStatus::connectedToServer && status() != TheWorldStatus::sessionInitialized)
			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Disconnect from server failed: not valid status, status=") + std::to_string((size_t)status())).c_str()));

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
		
		if (b)
		{
			plog::get()->setMaxSeverity(PLOG_DEBUG_LEVEL);
			m_client->MapManagerSetLogMaxSeverity(PLOG_DEBUG_LEVEL);
		}
		else
		{
			plog::get()->setMaxSeverity(PLOG_DEFAULT_LEVEL);
			m_client->MapManagerSetLogMaxSeverity(PLOG_DEFAULT_LEVEL);
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
