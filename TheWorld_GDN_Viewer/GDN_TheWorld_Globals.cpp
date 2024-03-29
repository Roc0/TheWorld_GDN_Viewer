//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"

#pragma warning(push, 0)
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/os.hpp>
#pragma warning(pop)

static bool g_plogInitilized = false;

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
	const std::string GDN_TheWorld_Globals::c_groundTexturesDir = "res://assets/textures/ground/";
	const std::string GDN_TheWorld_Globals::c_albedo_bump_file_ext = "_albedo_bump.ground";
	const std::string GDN_TheWorld_Globals::c_normal_roughness_file_ext = "_normal_roughness.ground";

	std::string GDN_TheWorld_Globals::getClientDataDir(void)
	{
		String userPath = OS::get_singleton()->get_user_data_dir();
		godot::PackedByteArray array = userPath.to_ascii_buffer();
		std::string dir((char*)array.ptr(), array.size());
		size_t l = dir.length();
		//const char* s = userPath.utf8().get_data();
		//std::string dir = std::string(s) + "\\TheWorld";

		if (dir.length() < 10)
		{
			std::string msg = std::string("Beccato " + dir);
			PLOG_ERROR << msg;
			throw(GDN_TheWorld_Exception(__FUNCTION__, msg.c_str()));
		}

		return dir;
	}

	godot::Error GDN_TheWorld_Globals::connectSignal(godot::Node* node, godot::String nodeType, godot::String signal, godot::Object* callableObject, godot::String callableMethod, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3)
	{
		godot::Array args;
		args.append(node);
		args.append(signal);
		args.append(nodeType);
		args.append(node->get_instance_id());
		args.append(node->get_name());
		args.append(custom1);
		args.append(custom2);
		args.append(custom3);

		int64_t size = args.size();
		godot::Error e = node->connect(signal, Callable(callableObject, callableMethod).bindv(args));
		return e;
	}

	GDN_TheWorld_Globals_Client::GDN_TheWorld_Globals_Client(GDN_TheWorld_Globals* globals, plog::Severity sev) : TheWorld_ClientServer::ClientInterface(sev)
	{
		m_globals = globals;
	}

	bool GDN_TheWorld_Globals_Client::quitting(void)
	{
		return m_globals->quitting();
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
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_SETLOGMAXSEVERITY " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(),"", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::ServerInitializeSession(bool isInEditor, plog::Severity sev)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(isInEditor);
		inputParams.push_back((int)sev);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, (TheWorld_ClientServer::ClientCallback*)m_globals);
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodAsync ==> " THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerGetQuadrantVertices(bool isInEditor, float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, float lowerXGridVertex, float lowerZGridVertex, int anchorType, int numVerticesPerSize, float gridStepinWU, int level, bool setCamera, float cameraDistanceFromTerrainForced, std::string meshId)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(cameraX);
		inputParams.push_back(cameraZ);
		inputParams.push_back(lowerXGridVertex);
		inputParams.push_back(lowerZGridVertex);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(level);
		inputParams.push_back(meshId);
		inputParams.push_back(setCamera);
		inputParams.push_back(cameraDistanceFromTerrainForced);
		inputParams.push_back(cameraY);
		inputParams.push_back(cameraYaw);
		inputParams.push_back(cameraPitch);
		inputParams.push_back(cameraRoll);
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES, ref, inputParams, THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::Sync(bool isInEditor, size_t numVerticesPerSize, float gridStepinWU)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(isInEditor);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC, ref, inputParams, THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerUploadBuffer(bool isInEditor, float lowerXGridVertex, float lowerZGridVertex, int numVerticesPerSize, float gridStepinWU, int level, std::string buffer)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(lowerXGridVertex);
		inputParams.push_back(lowerZGridVertex);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(level);
		inputParams.push_back(buffer);
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER, ref, inputParams, THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerDeployLevel(bool isInEditor, int level, size_t numVerticesPerSize, float gridStepinWU, bool setCamera, float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(level);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(setCamera);
		inputParams.push_back(cameraX);
		inputParams.push_back(cameraY);
		inputParams.push_back(cameraZ);
		inputParams.push_back(cameraDistanceFromTerrainForced);
		inputParams.push_back(cameraYaw);
		inputParams.push_back(cameraPitch);
		inputParams.push_back(cameraRoll);
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerDeployWorld(bool isInEditor, int level, size_t numVerticesPerSize, float gridStepinWU, bool setCamera, float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(level);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(setCamera);
		inputParams.push_back(cameraX);
		inputParams.push_back(cameraY);
		inputParams.push_back(cameraZ);
		inputParams.push_back(cameraDistanceFromTerrainForced);
		inputParams.push_back(cameraYaw);
		inputParams.push_back(cameraPitch);
		inputParams.push_back(cameraRoll);
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerUndeployWorld(bool isInEditor)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	GDN_TheWorld_Globals::GDN_TheWorld_Globals()
	{
		m_quitting = false;
		m_initialized = false;
		m_ready = false;
		setStatus(TheWorldStatus::uninitialized);
		m_isDebugEnabled = DEFAULT_DEBUG_ENABLED;
		m_bAppInError = false;
		m_lastErrorCode = 0;

		bool b = resize(THEWORLD_VIEWER_CHUNK_SIZE_SHIFT, THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT, true);
		assert(b);

		//m_mapManager = NULL;
		m_client = nullptr;
		m_gridStepInWU = 0;

		m_viewer = nullptr;
		m_isInEditor = false;

		m_totElementToInitialize = m_numElementInitialized = 0;
		
		_init();
	}

	GDN_TheWorld_Globals::~GDN_TheWorld_Globals()
	{
		deinit();
	}

	void GDN_TheWorld_Globals::_bind_methods()
	{
		ClassDB::bind_method(D_METHOD("get_status"), &GDN_TheWorld_Globals::get_status);
		ClassDB::bind_method(D_METHOD("set_status"), &GDN_TheWorld_Globals::set_status);

		ClassDB::bind_method(D_METHOD("connect_to_server"), &GDN_TheWorld_Globals::connectToServer);
		ClassDB::bind_method(D_METHOD("prepare_disconnect_from_server"), &GDN_TheWorld_Globals::prepareDisconnectFromServer);
		ClassDB::bind_method(D_METHOD("disconnect_from_server"), &GDN_TheWorld_Globals::disconnectFromServer);

		ClassDB::bind_method(D_METHOD("set_debug_enabled"), &GDN_TheWorld_Globals::setDebugEnabled);
		ClassDB::bind_method(D_METHOD("is_debug_enabled"), &GDN_TheWorld_Globals::isDebugEnabled);
		ClassDB::bind_method(D_METHOD("debug_print"), &GDN_TheWorld_Globals::debugPrint);
		ClassDB::bind_method(D_METHOD("error_print"), &GDN_TheWorld_Globals::errorPrint);
		ClassDB::bind_method(D_METHOD("warning_print"), &GDN_TheWorld_Globals::warningPrint);
		ClassDB::bind_method(D_METHOD("info_print"), &GDN_TheWorld_Globals::infoPrint);
		ClassDB::bind_method(D_METHOD("print"), &GDN_TheWorld_Globals::print);

		ClassDB::bind_method(D_METHOD("set_app_in_error"), &GDN_TheWorld_Globals::setAppInError);
		ClassDB::bind_method(D_METHOD("get_app_in_error"), &GDN_TheWorld_Globals::getAppInError);
		ClassDB::bind_method(D_METHOD("get_app_in_error_code"), &GDN_TheWorld_Globals::getAppInErrorCode);
		ClassDB::bind_method(D_METHOD("get_app_in_error_message"), &GDN_TheWorld_Globals::getAppInErrorMsg);

		ClassDB::bind_method(D_METHOD("get_num_vertices_per_chunk_side"), &GDN_TheWorld_Globals::numVerticesPerChuckSide);
		ClassDB::bind_method(D_METHOD("get_bitmap_resolution"), &GDN_TheWorld_Globals::heightmapResolution);
		ClassDB::bind_method(D_METHOD("get_lod_max_depth"), &GDN_TheWorld_Globals::lodMaxDepth);
		ClassDB::bind_method(D_METHOD("get_num_lods"), &GDN_TheWorld_Globals::numLods);
		ClassDB::bind_method(D_METHOD("get_chunks_per_bitmap_side"), &GDN_TheWorld_Globals::numChunksPerHeightmapSide);
		ClassDB::bind_method(D_METHOD("get_grid_step_in_wu"), &GDN_TheWorld_Globals::gridStepInHeightmapWUs);

		ClassDB::bind_method(D_METHOD("get_tot_element_to_initialize"), &GDN_TheWorld_Globals::getTotElementToInitialize);
		ClassDB::bind_method(D_METHOD("add_element_to_initialize"), &GDN_TheWorld_Globals::addElementToInitialize);
		ClassDB::bind_method(D_METHOD("get_num_element_initialized"), &GDN_TheWorld_Globals::getNumElementInitialized);
		ClassDB::bind_method(D_METHOD("add_num_element_initialized"), &GDN_TheWorld_Globals::addNumElementInitialized);

		ClassDB::bind_method(D_METHOD("viewer"), &GDN_TheWorld_Globals::Viewer);

		ADD_SIGNAL(MethodInfo("tw_status_changed", PropertyInfo(Variant::INT, "old_value"), PropertyInfo(Variant::INT, "new_value")));
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
				if (reply.getNumReplyParams() != 2)
				{
					std::string m = std::string("Server::initializeSession: error reply params ");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", 0));
				}

				ClientServerVariant v = reply.getReplyParam(0);
				const auto ptr(std::get_if<float>(&v));
				if (ptr == NULL)
				{
					std::string m = std::string("Server::initializeSession did not return a float as first param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				m_gridStepInWU = *ptr;

				v = reply.getReplyParam(1);
				const auto ptr1(std::get_if<std::string>(&v));
				if (ptr == NULL)
				{
					std::string m = std::string("Server::initializeSession did not return a string as second param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				m_mapName = *ptr1;

				Viewer()->initRequired(true);

				//Viewer()->init();
				
				//setStatus(TheWorldStatus::sessionInitialized);
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

	void GDN_TheWorld_Globals::init(bool isInEditor)
	{
		m_isInEditor = isInEditor;

		string logPath;
		if (IS_EDITOR_HINT())
			logPath = getModuleLoadPath() + "\\TheWorld_Viewer_EDITOR_log.txt";
		else
			logPath = getModuleLoadPath() + "\\TheWorld_Viewer_log.txt";
		plog::Severity sev = PLOG_DEFAULT_LEVEL;
		if (m_isDebugEnabled)
			sev = PLOG_DEBUG_LEVEL;
		if (!g_plogInitilized)
		{
			plog::init(sev, logPath.c_str(), 1000000, 3);
			TheWorld_Utils::Utils::plogInit(sev, plog::get());
			g_plogInitilized = true;
		}
		PLOG(plog::get()->getMaxSeverity()) << "***************";
		PLOG(plog::get()->getMaxSeverity()) << "Log initilized!";
		PLOG(plog::get()->getMaxSeverity()) << "***************";

		PLOG_INFO << "TheWorld Globals Initializing...";

		//setDebugEnabled(m_isDebugEnabled);

		m_initialized = true;
		setStatus(TheWorldStatus::initialized);
		PLOG_INFO << "TheWorld Globals Initialized!";
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
			PLOG_INFO << "TheWorld Globals Deinitializing...";

			PLOG_INFO << "TheWorld Globals Deinitialized!";

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

	std::string GDN_TheWorld_Globals::getMapName(void)
	{
		return m_mapName;
	}

	void GDN_TheWorld_Globals::_init(void)
	{
		//Cannot find Globals pointer as current node is not yet in the scene
		//godot::UtilityFunctions::print("GDN_TheWorld_Globals::Init");

		set_name(THEWORLD_GLOBALS_NODE_NAME);
	}

	bool GDN_TheWorld_Globals::connectedToServer(void)
	{
		if (m_client != nullptr && m_client->connected())
			return true;
		else
			return false;
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

		m_client->ServerInitializeSession(IS_EDITOR_HINT(), sev);
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
		bool can;
		
		if (m_client == nullptr)
			can = true;
		else
			can = m_client->canDisconnect();

		if (can)
			return true;
		else
			return false;
	}

	void GDN_TheWorld_Globals::disconnectFromServer(void)
	{
		if ((int)status() < (int)TheWorldStatus::connectedToServer)
			return;
			//throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Disconnect from server failed: not valid status, status=") + std::to_string((size_t)status())).c_str()));

		if (m_client == nullptr)
			return;
			//throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Disconnect from server failed: pointer to client is null").c_str())));

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
		m_ready = true;
	}

	void GDN_TheWorld_Globals::_input(const Ref<InputEvent>& event)
	{
	}

	void GDN_TheWorld_Globals::_notification(int p_what)
	{
		switch (p_what)
		{
		case NOTIFICATION_WM_CLOSE_REQUEST:
		{
			m_quitting = true;
		}
		break;
		case NOTIFICATION_PREDELETE:
		{
			if (m_ready)
			{
				if (m_quitting)
					debugPrint("GDN_TheWorld_Globals::_notification (NOTIFICATION_PREDELETE) - Destroy Globals", false);
				else
					debugPrint("GDN_TheWorld_Globals::_notification (NOTIFICATION_PREDELETE) - Destroy Globals");
			}
		}
		break;
		}
	}

	void GDN_TheWorld_Globals::_process(double _delta)
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
			m_viewer = Object::cast_to<GDN_TheWorld_Viewer>(root->find_child(THEWORLD_VIEWER_NODE_NAME, true, false));
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
