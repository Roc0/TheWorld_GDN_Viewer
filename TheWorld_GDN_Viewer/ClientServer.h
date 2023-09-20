#pragma once

#include <MapManager.h>
#include "Viewer_Utils.h"
#include "Utils.h"
#include "Profiler.h"

#include <string>
#include <vector>
#include <variant>
#include <map>
#include <algorithm>
#include <iterator>
#include <thread>

typedef std::variant<size_t, int, long, float, std::string, bool> ClientServerVariant;

namespace TheWorld_ClientServer
{
#define THEWORLD_CLIENTSERVER_RC_OK						0
#define THEWORLD_CLIENTSERVER_RC_ALREADY_CONNECTED		1
#define THEWORLD_CLIENTSERVER_RC_METHOD_ONGOING			2
#define THEWORLD_CLIENTSERVER_RC_TIMEOUT_EXPIRED		3
#define THEWORLD_CLIENTSERVER_RC_INDEX_OUT_OF_BORDER	4
#define THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR		5
#define THEWORLD_CLIENTSERVER_RC_TIMETOLIVE_EXPIRED		6
#define THEWORLD_CLIENTSERVER_RC_KO						-1

	//typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> MsTimePoint;

	class ClientInterface;
	class ServerInterface;
	class ClientCallback;

#define THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE		600000	//10000	// ms
#define THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE	600000	//10000	// ms
#define THEWORLD_CLIENTSERVER_DEFAULT_TIMEOUT			600000	//10000	// ms
#define THEWORLD_CLIENTSERVER_RECEIVER_SLEEP_TIME		1

#define THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION				"Server::initializeSession"
#define THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES			"MapManager::getQuadrantVertices"
#define THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER				"MapManager::uploadCacheBuffer"
#define THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD					"MapManager::deployWorld"
#define THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL					"MapManager::deployLevel"
#define THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD					"MapManager::undeployWorld"
#define THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC							"MapManager::sync"

#define THEWORLD_CLIENTSERVER_METHOD_MAPM_SETLOGMAXSEVERITY				"MapManager::setLogMaxSeverity"
//#define THEWORLD_CLIENTSERVER_METHOD_MAPM_CALCNEXTCOORDGETVERTICES		"MapManager::calcNextCoordOnTheGridInWUs"

	class ServerThreadContext
	{
	public:
		ServerThreadContext(plog::Severity sev)
		{
			m_sev = sev;
			//TheWorld_MapManager::MapManager* mapManager = getMapManager();
		}

		~ServerThreadContext()
		{
			deinit();
		}

		void deinit(void)
		{
			m_mapManager.reset();
		}
		
		TheWorld_MapManager::MapManager* getMapManager(void)
		{
			if (m_mapManager == nullptr)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("ServerThreadInit ") + __FUNCTION__, "Create m_mapManager");
				m_mapManager = make_unique<TheWorld_MapManager::MapManager>(/*nullptr, m_sev, plog::get(),*/ nullptr, true);
				m_mapManager->instrument(false);
				m_mapManager->consoleDebugMode(false);
			}
			return m_mapManager.get();
		}

	private:
		std::unique_ptr<TheWorld_MapManager::MapManager> m_mapManager;
		plog::Severity m_sev;
	};

	class ServerThreadContextPool
	{
	public:
		ServerThreadContextPool(plog::Severity sev)
		{
			m_sev = sev;
		}

		~ServerThreadContextPool()
		{
			m_mapCtxMtx.lock();
			for (auto& item : m_mapCtx)
			{
			}
			m_mapCtxMtx.unlock();
		}

		//void init()
		//{
		//	size_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
		//	m_mapCtxMtx.lock();
		//	if (!m_mapCtx.contains(tid))
		//		m_mapCtx[tid] = make_unique<ServerThreadContext>(m_sev);
		//	m_mapCtxMtx.unlock();
		//}

		ServerThreadContext* getCurrentContext(void)
		{
			size_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
			m_mapCtxMtx.lock();
			if (!m_mapCtx.contains(tid))
				m_mapCtx[tid] = make_unique<ServerThreadContext>(m_sev);
			ServerThreadContext* ctx = m_mapCtx[tid].get();
			m_mapCtxMtx.unlock();
			return ctx;
		}

		void resetForCurrentThread(void)
		{
			size_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
			m_mapCtxMtx.lock();
			if (m_mapCtx.contains(tid))
			{
				m_mapCtx[tid]->deinit();
				m_mapCtx.erase(tid);
			}
			m_mapCtxMtx.unlock();
		}

	private:
		map<size_t, std::unique_ptr<ServerThreadContext>> m_mapCtx;
		std::recursive_mutex m_mapCtxMtx;
		plog::Severity m_sev;
	};

	enum class ExecutionStatus
	{
		ToStart = 0,
		InProgress = 1,
		Completed = 2
	};
	
	class ClientServerExecution
	{
		friend ClientInterface;
		friend ServerInterface;

	public:
		ClientServerExecution(std::string method, std::vector<ClientServerVariant> inputParams, std::string ref, ClientInterface* client, ServerInterface* server, size_t timeToLive = THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, ClientCallback* callback = nullptr)
		{
			m_replied = false;
			m_error = false;
			m_serverExecutionStatus = ExecutionStatus::ToStart;
			m_clientExecutionStatus = ExecutionStatus::ToStart;
			m_errorCode = THEWORLD_CLIENTSERVER_RC_OK;
			m_method = method;
			m_inputParams = inputParams;
			//std::copy(inputParams.begin(), inputParams.end(), back_inserter(m_replyParam));
			m_ref = ref;
			m_client = client;
			m_server = server;
			m_clientCallback = callback;
			m_startExecution = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
			m_timeToLive = timeToLive;
			if (m_timeToLive == -1)
				m_timeToLive = THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE;
			m_eraseMe = false;
			m_expiredTimeToLive = false;
			m_duration = -1;
		}

		ClientServerExecution(const ClientServerExecution& reply)
		{
			*this = reply;
		}

		void operator=(const ClientServerExecution& reply)
		{
			m_replied = reply.m_replied;
			m_error = reply.m_error;
			m_serverExecutionStatus = reply.m_serverExecutionStatus;
			m_clientExecutionStatus = reply.m_clientExecutionStatus;
			m_errorCode = reply.m_errorCode;
			m_errorMessage = reply.m_errorMessage;
			m_method = reply.m_method;
			m_inputParams = reply.m_inputParams;
			m_replyParams = reply.m_replyParams;
			m_ref = reply.m_ref;
			m_client = reply.m_client;
			m_server = reply.m_server;
			m_clientCallback = reply.m_clientCallback;
			m_startExecution = reply.m_startExecution;
			m_timeToLive = reply.m_timeToLive;
			m_eraseMe = reply.m_eraseMe;
			m_expiredTimeToLive = reply.m_expiredTimeToLive;
			m_duration = reply.m_duration;
		}
			
		std::string getRef(void)
		{
			return m_ref;
		}

		void replyParam(ClientServerVariant replyParam)
		{
			m_replyParams.push_back(replyParam);
		}
		
		void callCallback(void);

		void replyComplete(void)
		{
			m_replied = true; 
			m_error = false;
		}

		bool expiredTimeToLive(long long* elapsedFromStartOfClientMethod = nullptr);

		bool replied(void) 
		{ 
			return m_replied; 
		}

		ClientServerVariant getReplyParam(size_t index) 
		{
			return m_replyParams[index]; 
		}
		size_t getNumReplyParams(void) 
		{
			return m_replyParams.size(); 
		}

		ClientServerVariant getInputParam(size_t index)
		{
			return m_inputParams[index]; 
		}
		size_t getNumInputParams(void)
		{
			return m_inputParams.size(); 
		}

		std::string getMethod(void)
		{
			return m_method; 
		}

		void replyError(int errorCode, std::string errorMessage)
		{
			m_error = true;
			m_errorCode = errorCode;
			m_errorMessage = errorMessage;
		}
		bool error(void) 
		{
			return m_error; 
		}
		int getErrorCode(void) 
		{
			return m_errorCode; 
		};
		std::string getErrorMessage(void)
		{
			return m_errorMessage; 
		};

		void onExecMethodAsync(void);

		bool clientCallbackSpecified(void)
		{
			return m_clientCallback != nullptr;
		}
		bool eraseMe(void) 
		{ return m_eraseMe; 
		}
		void eraseMe(bool eraseMe) 
		{
			m_eraseMe = eraseMe; 
		}

		bool completedOnServer(void)
		{
			return m_serverExecutionStatus == ExecutionStatus::Completed;
		}
		void serverExecutionStatus(enum class ExecutionStatus status)
		{
			m_serverExecutionStatus = status;
		}
		//bool busyOnClient(void)
		//{
		//	return m_busyOnClient; 
		//}
		bool toStartOnClient(void)
		{
			return m_clientExecutionStatus == ExecutionStatus::ToStart;
		}
		bool completedOnClient(void)
		{
			return m_clientExecutionStatus == ExecutionStatus::Completed;
		}
		void clientExecutionStatus(enum class ExecutionStatus status)
		{
			m_clientExecutionStatus = status;
		}
		size_t duration(void)
		{
			if (m_duration == -1)
			{
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_duration = (now - m_startExecution).count();
			}
			return m_duration;
		}

	private:
		bool m_replied;
		bool m_error;
		enum class ExecutionStatus m_serverExecutionStatus;
		enum class ExecutionStatus m_clientExecutionStatus;
		int m_errorCode;
		std::string m_errorMessage;
		std::string m_method;
		std::vector<ClientServerVariant> m_inputParams;
		std::vector<ClientServerVariant> m_replyParams;
		std::string m_ref;
		ClientInterface* m_client;
		ServerInterface* m_server;
		ClientCallback* m_clientCallback;
		TheWorld_Utils::MsTimePoint m_startExecution;
		size_t m_timeToLive;
		bool m_eraseMe;
		bool m_expiredTimeToLive;
		static size_t m_numCurrentServerExecutions;
		size_t m_duration;
	};

	typedef std::map<std::string, std::unique_ptr<ClientServerExecution>> MapClientServerExecution;

	class ClientCallback
	{
	public:
		virtual void replyFromServer(ClientServerExecution& reply) = 0;
	};

	class ClientInterface
	{
		friend ServerInterface;

	public:
		ClientInterface(plog::Severity sev);
		~ClientInterface(void);
		virtual int connect(void);
		virtual bool connected(void);
		virtual void prepareDisconnect(void);
		virtual bool canDisconnect(void);
		virtual void disconnect(void);
		virtual int execMethodAsync(std::string method, std::string& ref, std::vector<ClientServerVariant>& inputParams, size_t timeToLive = THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, ClientCallback* clientCallbak = nullptr);
		virtual int getReplyParam(std::string& ref, size_t index, ClientServerVariant& ReplyParam);
		virtual int getReplyParams(std::string& ref, std::vector <ClientServerVariant>& replyParams);
		virtual int replied(std::string& method, std::string& ref, bool& replied, size_t& numReplyParams, bool& error, int& errorCode, std::string& errorMessage);
		virtual bool quitting(void) = 0;

	private:
		void receiver(void);

	private:
		ServerInterface* m_server;
		plog::Severity m_sev;
		std::recursive_mutex m_mtxReplyMap;
		MapClientServerExecution m_ReplyMap;
		size_t m_msTimeout;

		// receiver thread
		std::thread m_receiverThread;
		bool m_receiverThreadRequiredExit;
		bool m_receiverThreadRunning;
		TheWorld_Utils::ThreadPool m_tp;
	};

	class ServerInterface : public TheWorld_Utils::ThreadInitDeinit
	{
	public:
		ServerInterface(plog::Severity sev);
		~ServerInterface(void);
		virtual int onConnect(ClientInterface* client);
		virtual void onDisconnect(void);
		virtual void onExecMethodAsync(std::string method, ClientServerExecution* reply);
		void queueJob(const std::function<void()>& job, ClientServerExecution* clientServerExecution);
		virtual void serverThreadInit(void);
		virtual void serverThreadDeinit(void);
		virtual void threadInit(void)
		{
			serverThreadInit();
		}
		virtual void threadDeinit(void)
		{
			serverThreadDeinit();
		}

	private:
		bool expiredTimeToLive(ClientServerExecution* reply);
	
	private:
		std::unique_ptr<ServerThreadContextPool> m_threadContextPool;
		std::recursive_mutex m_threadContextPoolMtx;
		static bool s_staticServerInitializationDone;
		static std::recursive_mutex s_staticServerInitializationMtx;
		ClientInterface* m_client;
		plog::Severity m_sev;
		TheWorld_Utils::ThreadPool m_tpSlowExecutions;
		TheWorld_Utils::ThreadPool m_tp;
	};
}

