//#include "pch.h"
#include "Viewer_Utils.h"

#include "ClientServer.h"
#include "Profiler.h"

#include <Mutex>
#include <Functional>

#include <Rpc.h>


namespace TheWorld_ClientServer
{
	size_t ClientServerExecution::m_numCurrentServerExecutions = 0;
	bool ServerInterface::s_staticServerInitializationDone = false;
	std::recursive_mutex ServerInterface::s_staticServerInitializationMtx;

	void ClientServerExecution::onExecMethodAsync(void)
	{
		//TheWorld_Viewer_Utils::TimerMs clock("ClientServerExecution::onExecMethodAsync", "SERVER SIDE " + m_method + " " + m_ref, false, true);
		//clock.tick();

		serverExecutionStatus(ExecutionStatus::InProgress);

		m_numCurrentServerExecutions++;
		try
		{
			m_server->onExecMethodAsync(m_method, this);
		}
		catch (...)
		{
			//clock.headerMsg("SERVER SIDE " + m_method + " " + m_ref + " (" + std::to_string(m_numCurrentServerExecutions) + ")");
			m_numCurrentServerExecutions--;
			serverExecutionStatus(ExecutionStatus::Completed);
			return;
		}
		//clock.headerMsg("SERVER SIDE " + m_method + " " + m_ref + " (" + std::to_string(m_numCurrentServerExecutions) + ")");
		m_numCurrentServerExecutions--;
		serverExecutionStatus(ExecutionStatus::Completed);
	}
	
	bool ClientServerExecution::expiredTimeToLive(long long* _elapsedFromStartOfClientMethod)
	{
		if (m_expiredTimeToLive)
			return true;

		TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
		long long elapsedFromStartOfClientMethod = (now - m_startExecution).count();
		if (_elapsedFromStartOfClientMethod != nullptr)
			*_elapsedFromStartOfClientMethod = elapsedFromStartOfClientMethod;
		if (elapsedFromStartOfClientMethod > (long long)m_timeToLive)
		{
			m_expiredTimeToLive = true;
			return true;
		}
		return false;
	}

	void ClientServerExecution::callCallback(void)
	{
		//TheWorld_Viewer_Utils::TimerMs clock("ClientServerExecution::callCallback", "CLIENT SIDE " + m_method + " " + m_ref, false, true);
		//clock.tick();

		clientExecutionStatus(ExecutionStatus::InProgress);

		// Set the server execution elapsed
		size_t d = duration();

		if (clientCallbackSpecified())
			m_clientCallback->replyFromServer(*this);
		
		clientExecutionStatus(ExecutionStatus::Completed);
	}
	
	ClientInterface::ClientInterface(plog::Severity sev)
	{
		m_server = nullptr;
		m_sev = sev;
		m_msTimeout = THEWORLD_CLIENTSERVER_DEFAULT_TIMEOUT;

		m_receiverThreadRequiredExit = false;
		m_receiverThreadRunning = false;
	}

	ClientInterface::~ClientInterface(void)
	{
		disconnect();
	}

	bool ClientInterface::connected(void)
	{
		if (m_server != nullptr)
			return true;
		else
			return false;
	}

	int ClientInterface::connect(void)
	{
		if (m_server != nullptr)
			return THEWORLD_CLIENTSERVER_RC_ALREADY_CONNECTED;

		m_server = new ServerInterface(m_sev);
		m_server->onConnect(this);

		PLOG_INFO << "ClientInterface::connect - Server connected";

		m_receiverThreadRequiredExit = false;
		m_receiverThreadRunning = true;
		m_receiverThread = std::thread(&ClientInterface::receiver, this);

		PLOG_INFO << "ClientInterface::connect - Receiver thread started";

		return THEWORLD_CLIENTSERVER_RC_OK;
	}

	void ClientInterface::prepareDisconnect(void)
	{
		m_receiverThreadRequiredExit = true;
		//if (m_receiverThread.joinable())
		//{
		//	m_receiverThread.join();
		//	PLOG_INFO << "ClientInterface::prepareDisconnect - Receiver thread joined (stopped execution)";
		//}
	}

	bool ClientInterface::canDisconnect(void)
	{
		return !m_receiverThreadRunning;
		//if (m_receiverThread.joinable())
		//{
		//	m_receiverThread.join();
		//	PLOG_INFO << "ClientInterface::prepareDisconnect - Receiver thread joined (stopped execution)";
		//}
	}

	void ClientInterface::disconnect(void)
	{
		m_receiverThreadRequiredExit = true;
		if (m_receiverThread.joinable())
		{
			m_receiverThread.join();
			PLOG_INFO << "ClientInterface::connect - Receiver thread joined (stopped execution)";
		}

		if (m_server)
		{
			m_server->onDisconnect();
			delete m_server;
			m_server = nullptr;

			PLOG_INFO << "ClientInterface::disconnect - Server disconnected";
		}
	}

	int ClientInterface::execMethodAsync(std::string method, std::string& ref, std::vector<ClientServerVariant>& inputParams, size_t timeToLive, ClientCallback* clientCallbak)
	{
		GUID newId;
		RPC_STATUS ret_val = ::UuidCreate(&newId);
		if (ret_val != RPC_S_OK)
		{
			PLOG_ERROR << "UuidCreate in error with rc " + std::to_string(ret_val);
			return THEWORLD_CLIENTSERVER_RC_KO;
		}

		{
			std::lock_guard<std::recursive_mutex> lock(m_mtxReplyMap);
			ref = TheWorld_Viewer_Utils::ToString(&newId);
			m_ReplyMap[ref] = make_unique<ClientServerExecution>(method, inputParams, ref, this, m_server, timeToLive, clientCallbak);
			
			//PLOG_DEBUG << "ClientInterface::execMethodAsync - " + method + " " + ref;

			ClientServerExecution* clientServerExecution = m_ReplyMap[ref].get();
			std::function<void(void)> f = std::bind(&ClientServerExecution::onExecMethodAsync, clientServerExecution);
			m_server->queueJob(f, clientServerExecution);
		}
		
		return THEWORLD_CLIENTSERVER_RC_OK;
	}
	
	int ClientInterface::replied(std::string& method, std::string& ref, bool& replied, size_t& numReplyParams, bool& error, int& errorCode, std::string& errorMessage)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxReplyMap);

		if (m_ReplyMap.find(ref) == m_ReplyMap.end())
		{
			PLOG_ERROR << "Cannot find ref in ReplyMap";
			return THEWORLD_CLIENTSERVER_RC_KO;
		}

		ClientServerExecution* reply = m_ReplyMap[ref].get();

		method = reply->getMethod();

		error = reply->error();
		if (error)
		{
			errorCode = reply->getErrorCode();
			errorMessage = reply->getErrorMessage();
		}
		
		replied = reply->replied();

		if (replied)
			numReplyParams = reply->getNumReplyParams();

		return THEWORLD_CLIENTSERVER_RC_OK;
	}

	int ClientInterface::getReplyParam(std::string& ref, size_t index, ClientServerVariant& ReplyParam)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxReplyMap);

		if (m_ReplyMap.find(ref) == m_ReplyMap.end())
		{
			PLOG_ERROR << "Cannot find ref in ReplyMap";
			return THEWORLD_CLIENTSERVER_RC_KO;
		}

		ClientServerExecution* reply = m_ReplyMap[ref].get();

		if (reply->error())
		{
			PLOG_ERROR << "Method execution error - Method: " + reply->getMethod() + " - Error: " + std::to_string(reply->getErrorCode()) + " - " + reply->getErrorMessage();
			return THEWORLD_CLIENTSERVER_RC_KO;
		}
		
		if (reply->replied())
		{
			if (index < reply->getNumReplyParams())
			{
				ReplyParam = reply->getReplyParam(index);
				return THEWORLD_CLIENTSERVER_RC_OK;
			}
			else
				return THEWORLD_CLIENTSERVER_RC_INDEX_OUT_OF_BORDER;
		}
		else
			return THEWORLD_CLIENTSERVER_RC_METHOD_ONGOING;
	}

	int ClientInterface::getReplyParams(std::string& ref, std::vector <ClientServerVariant>& replyParams)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxReplyMap);

		if (m_ReplyMap.find(ref) == m_ReplyMap.end())
		{
			PLOG_ERROR << "Cannot find ref in ReplyMap";
			return THEWORLD_CLIENTSERVER_RC_KO;
		}

		ClientServerExecution* reply = m_ReplyMap[ref].get();

		if (reply->error())
		{
			PLOG_ERROR << "Method execution error - Method: " + reply->getMethod() + " - Error: " + std::to_string(reply->getErrorCode()) + " - " + reply->getErrorMessage();
			return THEWORLD_CLIENTSERVER_RC_KO;
		}

		if (reply->replied())
		{
			PLOG_DEBUG << "ClientInterface::getReplyParams - Prima di replyParams = reply->m_replyParams";
			replyParams = reply->m_replyParams;
			PLOG_DEBUG << "ClientInterface::getReplyParams - Dopo replyParams = reply->m_replyParams";
			return THEWORLD_CLIENTSERVER_RC_OK;
		}
		else
			return THEWORLD_CLIENTSERVER_RC_METHOD_ONGOING;
	}

	void ClientInterface::receiver(void)
	{
		m_receiverThreadRunning = true;

		MapClientServerExecution toCallCallback;

		m_tp.Start("ReceiverFromServer", 24);

		while (true)
		{
			{
				std::list<std::string> tempKeyToErase;
				for (auto iter = toCallCallback.begin(); iter != toCallCallback.end(); iter++)
				{
					if (iter->second->completedOnClient())
					{
						tempKeyToErase.push_back(iter->first);
					}
				}
				for (auto key : tempKeyToErase)
				{
					toCallCallback.erase(key);
				}
			}
			
			if (m_receiverThreadRequiredExit)
				break;
			else
			{
				bool locked = m_mtxReplyMap.try_lock();
				if (locked)
				{
					MapClientServerExecution tempToErase;

					for (MapClientServerExecution::iterator itReply = m_ReplyMap.begin(); itReply != m_ReplyMap.end(); itReply++)
					{
						if (itReply->second->replied() || itReply->second->error())
						{
							if (itReply->second->error())
							{
								PLOG_ERROR << "Server execution error - Method: " + itReply->second->getMethod() + " - Error: " + std::to_string(itReply->second->getErrorCode()) + " - " + itReply->second->getErrorMessage();
								tempToErase[itReply->first] = make_unique<ClientServerExecution>(*itReply->second);
							}
							else
							{
								if (itReply->second->completedOnServer())
								{
									if (itReply->second->clientCallbackSpecified())
									{
										toCallCallback[itReply->first] = make_unique<ClientServerExecution>(*itReply->second);
									}
									else if (itReply->second->eraseMe())
									{
										tempToErase[itReply->first] = make_unique<ClientServerExecution>(*itReply->second);
									}
								}
							}
						}
					}
					for (MapClientServerExecution::iterator itReply = toCallCallback.begin(); itReply != toCallCallback.end(); itReply++)
					{
						if (m_ReplyMap.contains(itReply->first))
						{
							m_ReplyMap.erase(itReply->first);
						}
					}
					
					for (MapClientServerExecution::iterator itReply = tempToErase.begin(); itReply != tempToErase.end(); itReply++)
					{
						if (m_ReplyMap.contains(itReply->first))
						{
							m_ReplyMap.erase(itReply->first);
						}
					}

					m_mtxReplyMap.unlock();

					tempToErase.clear();

					for (MapClientServerExecution::iterator itReply = toCallCallback.begin(); itReply != toCallCallback.end(); itReply++)
					{
						if (itReply->second->toStartOnClient())
						{
							std::function<void(void)> f = std::bind(&ClientServerExecution::callCallback, itReply->second.get());
							itReply->second->clientExecutionStatus(ExecutionStatus::InProgress);
							m_tp.QueueJob(f);
						}
					}
				}
			}
			
			Sleep(THEWORLD_CLIENTSERVER_RECEIVER_SLEEP_TIME);
		}
		
		m_tp.Stop();

		toCallCallback.clear();

		m_receiverThreadRunning = false;
	}
	
	ServerInterface::ServerInterface(plog::Severity sev)
	{
		m_client = nullptr;
		m_sev = sev;
	}

	ServerInterface::~ServerInterface(void)
	{
		onDisconnect();
	}

	int ServerInterface::onConnect(ClientInterface* client)
	{
		s_staticServerInitializationMtx.lock();
		if (!s_staticServerInitializationDone)
		{
			TheWorld_MapManager::MapManager::staticInit(nullptr, m_sev, plog::get(), true);
			s_staticServerInitializationDone = true;
		}
		s_staticServerInitializationMtx.unlock();
		
		m_client = client;

		m_threadContextPool = make_unique<ServerThreadContextPool>(m_sev);
		
		//std::function<void(void)> threadInitFunction = std::bind(&ServerInterface::serverThreadInit, this);
		//std::function<void(void)> threadDeinitFunction = std::bind(&ServerInterface::serverThreadDeinit, this);
		m_tpSlowExecutions.Start("SlowServerWorker", 24, /*&threadInitFunction, &threadDeinitFunction,*/ this);
		m_tp.Start("ServerWorker", 6, /*&threadInitFunction, &threadDeinitFunction,*/ this);
		
		PLOG_INFO << "ServerInterface::onConnect - Server connected";
		return THEWORLD_CLIENTSERVER_RC_OK;
	}

	void ServerInterface::onDisconnect(void)
	{
		if (m_client != nullptr)
		{
			m_tp.Stop();
			m_tpSlowExecutions.Stop();
			m_threadContextPool.reset();
			m_client = nullptr;
			PLOG_INFO << "ServerInterface::onDisconnect - Server Disconnected";
			TheWorld_MapManager::MapManager::staticDeinit();
		}
	}

	bool ServerInterface::expiredTimeToLive(ClientServerExecution* reply)
	{
		long long elapsedFromStartOfClientMethod;
		if (reply->expiredTimeToLive(&elapsedFromStartOfClientMethod))
		{
			reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, (std::string(__FUNCTION__) + std::string("Time To Live (") + std::to_string(reply->m_timeToLive) + std::string(") Expired: ") + std::to_string(elapsedFromStartOfClientMethod)).c_str());
			return true;
		}
		else
			return false;
	}
	
	void ServerInterface::onExecMethodAsync(std::string method, ClientServerExecution* reply)
	{
		// TODO gestione pool ServerThreadContext che contiene un map manager per ogni thread::id

		try
		{
			if (expiredTimeToLive(reply))
				return;

			if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_SETLOGMAXSEVERITY)
			{
				int* i = std::get_if<int>(&reply->m_inputParams[0]);
				if (i == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be an INT");
					return;
				}
				plog::Severity sev = plog::Severity(*i);
				TheWorld_MapManager::MapManager::setLogMaxSeverity(sev);
				reply->replyComplete();
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION)
			{
				int* i = std::get_if<int>(&reply->m_inputParams[1]);
				if (i == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be an INT");
					return;
				}
				plog::Severity sev = plog::Severity(*i);

				TheWorld_MapManager::MapManager::setLogMaxSeverity(sev);

				float gridStepInWU = m_threadContextPool->getCurrentContext()->getMapManager()->gridStepInWU();
				std::string mapName = m_threadContextPool->getCurrentContext()->getMapManager()->getMapName();
				reply->replyParam(gridStepInWU);
				reply->replyParam(mapName);
				reply->replyComplete();
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("UploadBuffer 1a ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER);

				float* lowerXGridVertex = std::get_if<float>(&reply->m_inputParams[0]);
				if (lowerXGridVertex == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be a FLOAT");
					return;
				}

				float* lowerZGridVertex = std::get_if<float>(&reply->m_inputParams[1]);
				if (lowerZGridVertex == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Second param must be a FLOAT");
					return;
				}

				int* numVerticesPerSize = std::get_if<int>(&reply->m_inputParams[2]);
				if (numVerticesPerSize == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Third param must be an INT");
					return;
				}

				float* gridStepinWU = std::get_if<float>(&reply->m_inputParams[3]);
				if (gridStepinWU == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Forth param must be a FLOAT");
					return;
				}

				int* level = std::get_if<int>(&reply->m_inputParams[4]);
				if (level == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Fifth param must be an INT");
					return;
				}
				
				std::string* buffer = std::get_if<std::string>(&reply->m_inputParams[5]);
				if (buffer == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Sixth param must be an std::string");
					return;
				}

				if (expiredTimeToLive(reply))
					return;

				m_threadContextPool->getCurrentContext()->getMapManager()->uploadCacheBuffer(*lowerXGridVertex, *lowerZGridVertex, *numVerticesPerSize, *gridStepinWU, *level, *buffer);

				reply->replyComplete();
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 1a ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES);

				float* cameraX = std::get_if<float>(&reply->m_inputParams[0]);
				if (cameraX == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be a FLOAT");
					return;
				}
				
				float* cameraZ = std::get_if<float>(&reply->m_inputParams[1]);
				if (cameraZ == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Second param must be a FLOAT");
					return;
				}
				
				float* lowerXGridVertex = std::get_if<float>(&reply->m_inputParams[2]);
				if (lowerXGridVertex == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Third param must be a FLOAT");
					return;
				}
				
				float* lowerZGridVertex = std::get_if<float>(&reply->m_inputParams[3]);
				if (lowerZGridVertex == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Fourth param must be a FLOAT");
					return;
				}
				
				int* numVerticesPerSize = std::get_if<int>(&reply->m_inputParams[4]);
				if (numVerticesPerSize == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Fifth param must be an INT");
					return;
				}
				
				float* gridStepinWU = std::get_if<float>(&reply->m_inputParams[5]);
				if (gridStepinWU == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Sixth param must be a FLOAT");
					return;
				}
				
				int* level = std::get_if<int>(&reply->m_inputParams[6]);
				if (level == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Seventh param must be an INT");
					return;
				}

				std::string* meshId = std::get_if<std::string>(&reply->m_inputParams[7]);
				if (meshId == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Eight param must be an std::string");
					return;
				}

				//TheWorld_Viewer_Utils::TimerMs clock;
				//clock.tick();
				float _gridStepInWU;
				std::string meshBuffer;
				TheWorld_MapManager::MapManager* mapManager = m_threadContextPool->getCurrentContext()->getMapManager();
				mapManager->getQuadrantVertices(*lowerXGridVertex, *lowerZGridVertex, *numVerticesPerSize, _gridStepInWU, *level, *meshId, meshBuffer);
				//clock.tock();
				//PLOG_DEBUG << std::string("ELAPSED MapManager::getVertices=") + std::to_string(clock.duration().count()).c_str() + " ms";

				if (expiredTimeToLive(reply))
					return;

				if (_gridStepInWU != *gridStepinWU)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "gridStepinWU not compatible");
					return;
				}

				float f = mapManager->calcNextCoordOnTheGridInWUs(*cameraX);
				reply->replyParam(f);
				f = mapManager->calcNextCoordOnTheGridInWUs(*cameraZ);
				reply->replyParam(f);
				
				reply->replyParam(meshBuffer);

				reply->replyComplete();
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("DeployLevel ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL);

				int* level = std::get_if<int>(&reply->m_inputParams[0]);
				if (level == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be an int");
					return;
				}

				size_t* numVerticesPerSize = std::get_if<size_t>(&reply->m_inputParams[1]);
				if (numVerticesPerSize == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Second param must be a size_t");
					return;
				}

				bool* isInEditor = std::get_if<bool>(&reply->m_inputParams[11]);
				if (isInEditor == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Third param must be a bool");
					return;
				}

				m_threadContextPool->getCurrentContext()->getMapManager()->alignDiskCacheAndDB(*isInEditor,*numVerticesPerSize, *level);

				reply->replyComplete();
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("DeployWorld 0a ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD);

				reply->replyComplete();
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("UndeployWorld ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD);

				bool* isInEditor = std::get_if<bool>(&reply->m_inputParams[0]);
				if (isInEditor == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be a bool");
					return;
				}

				m_threadContextPool->getCurrentContext()->getMapManager()->stopAlignDiskCacheAndDBTasks(*isInEditor);

				reply->replyComplete();
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("Sync ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC);

				bool* isInEditor = std::get_if<bool>(&reply->m_inputParams[0]);
				if (isInEditor == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be a bool");
					return;
				}

				size_t* numVerticesPerSize = std::get_if<size_t>(&reply->m_inputParams[1]);
				if (numVerticesPerSize == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Second param must be a size_t");
					return;
				}

				float* gridStepinWU = std::get_if<float>(&reply->m_inputParams[2]);
				if (gridStepinWU == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Third param must be a float");
					return;
				}

				int level = 0;
				float lowerXGridVertex = 0.0f;
				float lowerZGridVertex = 0.0f;
				bool foundUpdatedQuadrant = false;;
				m_threadContextPool->getCurrentContext()->getMapManager()->sync(*isInEditor, *numVerticesPerSize, *gridStepinWU, foundUpdatedQuadrant, level, lowerXGridVertex, lowerZGridVertex);

				reply->replyParam(foundUpdatedQuadrant);
				reply->replyParam(level);
				reply->replyParam(lowerXGridVertex);
				reply->replyParam(lowerZGridVertex);

				reply->replyComplete();
			}
		}
		catch (TheWorld_Viewer_Utils::GDN_TheWorld_Exception& e)
		{
			reply->replyError(THEWORLD_CLIENTSERVER_RC_KO, e.exceptionName() + string(" caught - ") + e.what());
		}
		catch (TheWorld_MapManager::MapManagerException& e)
		{
			reply->replyError(THEWORLD_CLIENTSERVER_RC_KO, e.exceptionName() + string(" caught - ") + e.what());
		}
		catch (std::exception& e)
		{
			reply->replyError(THEWORLD_CLIENTSERVER_RC_KO, std::string("std::exception caught - ") + e.what());
		}
		catch (...)
		{
			reply->replyError(THEWORLD_CLIENTSERVER_RC_KO, std::string("std::exception caught"));
		}
	}

	void ServerInterface::queueJob(const std::function<void()>& job, ClientServerExecution* clientServerExecution)
	{
		if (clientServerExecution->getMethod() == THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES)
			m_tpSlowExecutions.QueueJob(job);
		else
			m_tp.QueueJob(job);
	}

	void ServerInterface::serverThreadInit(void)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("ServerThreadInit ") + __FUNCTION__, "m_threadContextPool->getCurrentContext()");
		ServerThreadContext* threadCtx = m_threadContextPool->getCurrentContext();
	}

	void ServerInterface::serverThreadDeinit(void)
	{
		m_threadContextPool->resetForCurrentThread();
	}
}
