//#include "pch.h"
// SUPER DEBUGRIC
#include <Godot.hpp>
#include <ResourceLoader.hpp>
#include <ImageTexture.hpp>
// SUPER DEBUGRIC

#include "ClientServer.h"
#include "TheWorld_Utils.h"

#include <Mutex>
#include <Functional>

#include <Rpc.h>


namespace TheWorld_ClientServer
{
	size_t ClientServerExecution::m_numCurrentServerExecutions = 0;
	
	void ClientServerExecution::onExecMethodAsync(void)
	{
		TheWorld_Utils::TimerMs clock("ClientServerExecution::onExecMethodAsync", "SERVER SIDE " + m_method + " " + m_ref, false, true);
		clock.tick();

		serverExecutionStatus(ExecutionStatus::InProgress);

		m_numCurrentServerExecutions++;
		try
		{
			m_server->onExecMethodAsync(m_method, this);
		}
		catch (...)
		{
			clock.headerMsg("SERVER SIDE " + m_method + " " + m_ref + " (" + std::to_string(m_numCurrentServerExecutions) + ")");
			m_numCurrentServerExecutions--;
			serverExecutionStatus(ExecutionStatus::Completed);
			return;
		}
		clock.headerMsg("SERVER SIDE " + m_method + " " + m_ref + " (" + std::to_string(m_numCurrentServerExecutions) + ")");
		m_numCurrentServerExecutions--;
		serverExecutionStatus(ExecutionStatus::Completed);
	}
	
	bool ClientServerExecution::expiredTimeToLive(long long* _elapsedFromStartOfClientMethod)
	{
		if (m_expiredTimeToLive)
			return true;

		TheWorld_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
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
		TheWorld_Utils::TimerMs clock("ClientServerExecution::callCallback", "CLIENT SIDE " + m_method + " " + m_ref, false, true);
		clock.tick();

		clientExecutionStatus(ExecutionStatus::InProgress);

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
	}

	ClientInterface::~ClientInterface(void)
	{
		disconnect();
	}

	int ClientInterface::connect(void)
	{
		if (m_server != nullptr)
			return THEWORLD_CLIENTSERVER_RC_ALREADY_CONNECTED;

		m_server = new ServerInterface(m_sev);
		m_server->onConnect(this);

		PLOG_INFO << "ClientInterface::connect - Server connected";

		m_receiverThreadRequiredExit = false;
		m_receiverThread = std::thread(&ClientInterface::receiver, this);

		PLOG_INFO << "ClientInterface::connect - Receiver thread started";

		return THEWORLD_CLIENTSERVER_RC_OK;
	}

	void ClientInterface::disconnect(void)
	{
		if (m_receiverThread.joinable())
		{
			m_receiverThreadRequiredExit = true;
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
			ref = TheWorld_Utils::ToString(&newId);
			m_ReplyMap[ref] = make_unique<ClientServerExecution>(method, inputParams, ref, this, m_server, timeToLive, clientCallbak);
			//std::thread* th = new std::thread(&ClientServerExecution::onExecMethodAsync, m_ReplyMap[ref].get());
			
			PLOG_DEBUG << "ClientInterface::execMethodAsync - " + method + " " + ref;

			ClientServerExecution* clientServerExecution = m_ReplyMap[ref].get();
			std::function<void(void)> f = std::bind(&ClientServerExecution::onExecMethodAsync, clientServerExecution);
			//std::function<void(void)> f1 = [=] {m_ReplyMap[ref]->onExecMethodAsync(); };
			//m_server->queueJob( [=] { m_ReplyMap[ref]->onExecMethodAsync(); } ); // lambda expressione
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

//	int ClientInterface::execMethodSync(std::string method, std::vector<ClientServerVariant>& inputParams, std::vector <ClientServerVariant>& replyParams, size_t timeout)
//	{
//		PLOG_DEBUG << "execMethodSync: inizio";
//
//		int rc = THEWORLD_CLIENTSERVER_RC_OK;
//
//		replyParams.clear();
//
//		std::string ref;
//
//		ClientCallback* noCallback = nullptr;
//
//#define SHORT_ROAD 0
//		if (SHORT_ROAD)
//		{
//			GUID newId;
//			RPC_STATUS ret_val = ::UuidCreate(&newId);
//			if (ret_val != RPC_S_OK)
//			{
//				PLOG_ERROR << "UuidCreate in error with rc " + std::to_string(ret_val);
//				return THEWORLD_CLIENTSERVER_RC_KO;
//			}
//			
//			ClientServerExecution* reply = nullptr;
//			{
//				std::lock_guard<std::recursive_mutex> lock(m_mtxReplyMap);
//				ref = TheWorld_Utils::ToString(&newId);
//				m_ReplyMap[ref] = make_unique<ClientServerExecution>(method, inputParams, ref, this, m_server, timeout, noCallback);
//				reply = m_ReplyMap[ref].get();
//				//std::thread* th = new std::thread(&ClientServerExecution::onExecMethodAsync, m_ReplyMap[ref].get());
//			}
//			reply->onExecMethodAsync();
//		}
//		else		
//			rc = execMethodAsync(method, ref, inputParams, timeout, noCallback);
//		
//		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
//		{
//			if (ref.length() > 0 && m_ReplyMap.contains(ref))
//			{
//				m_ReplyMap[ref]->eraseMe(true);
//			}
//			return rc;
//		}
//
//		PLOG_DEBUG << "execMethodSync: attesa risposta";
//		TheWorld_Utils::TimerMs clock;
//		clock.tick();
//		while (true)
//		{
//			bool replied = false, error = false;
//			size_t numReplyParams = 0;
//			int errorCode;
//			std::string errorMessage;
//			std::string method;
//			rc = ClientInterface::replied(method, ref, replied, numReplyParams, error, errorCode, errorMessage);
//			if (rc != THEWORLD_CLIENTSERVER_RC_OK)
//			{
//				m_ReplyMap[ref]->eraseMe(true);
//				return rc;
//			}
//			
//			if (error)
//			{
//				PLOG_ERROR << "Method execution error - Method: " + method + " - Error: " + std::to_string(errorCode) + " - " + errorMessage;
//				m_ReplyMap[ref]->eraseMe(true);
//				return THEWORLD_CLIENTSERVER_RC_KO;
//			}
//
//			size_t _timeout = timeout;
//			if (_timeout == -1)
//				_timeout = m_msTimeout;
//			if (!replied)
//			{
//				clock.tock();
//				long long elapsed = clock.duration().count();
//				if (elapsed > (long long)_timeout)
//				{
//					rc = THEWORLD_CLIENTSERVER_RC_TIMEOUT_EXPIRED;
//					m_ReplyMap[ref]->eraseMe(true);
//					break;
//				}
//				else
//					Sleep(5);
//			}
//			else
//			{
//				PLOG_DEBUG << "execMethodSync: callect params";
//				rc = getReplyParams(ref, replyParams);
//				//for (size_t idx = 0; idx < numReplyParams; idx++)
//				//{
//				//	ClientServerVariant replyParam;
//				//	rc = getReplyParam(ref, idx, replyParam);
//				//	if (rc != THEWORLD_CLIENTSERVER_RC_OK)
//				//		return rc;
//				//	replyParams.push_back(replyParam);
//				//}
//				std::lock_guard<std::recursive_mutex> lock(m_mtxReplyMap);
//				m_ReplyMap[ref]->eraseMe(true);
//				break;
//			}
//		}
//
//		PLOG_DEBUG << "execMethodSync: fine";
//
//		return rc;
//	}

	void ClientInterface::receiver(void)
	{
		MapClientServerExecution toCallCallback;

		m_tp.Start(4);

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
	}
	
	ServerInterface::ServerInterface(plog::Severity sev)
	{
		m_client = nullptr;
		m_mapManager = nullptr;
		m_sev = sev;
	}

	ServerInterface::~ServerInterface(void)
	{
		onDisconnect();
	}

	int ServerInterface::onConnect(ClientInterface* client)
	{
		m_client = client;
		m_mapManager = new TheWorld_MapManager::MapManager(NULL, m_sev, plog::get());
		m_mapManager->instrument(false);
		m_mapManager->consoleDebugMode(false);
		m_tpSlowExecutions.Start(2);
		m_tp.Start(2);
		PLOG_INFO << "ServerInterface::onConnect - Server connected";
		return THEWORLD_CLIENTSERVER_RC_OK;
	}

	void ServerInterface::onDisconnect(void)
	{
		if (m_client != nullptr)
		{
			m_tp.Stop();
			m_tpSlowExecutions.Stop();
			m_client = nullptr;
			delete m_mapManager;
			PLOG_INFO << "ServerInterface::onDisconnect - Server Disconnected";
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
				m_mapManager->setLogMaxSeverity(sev);
				reply->replyComplete();
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION)
			{
				int* i = std::get_if<int>(&reply->m_inputParams[0]);
				if (i == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be an INT");
					return;
				}
				plog::Severity sev = plog::Severity(*i);
				m_mapManager->setLogMaxSeverity(sev);
				float f = m_mapManager->gridStepInWU();
				reply->replyParam(f);
				reply->replyComplete();
			}
			//else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_CALCNEXTCOORDGETVERTICES)
			//{
			//	for (size_t i = 0; i < reply->m_inputParams.size(); i++)
			//	{
			//		float* coord = std::get_if<float>(&reply->m_inputParams[i]);
			//		if (coord == nullptr)
			//		{
			//			reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be a FLOAT");
			//			return;
			//		}
			//		float f = m_mapManager->calcNextCoordOnTheGridInWUs(*coord);
			//		reply->replyParam(f);
			//	}
			//	reply->replyComplete();
			//}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES)
			{
				float* viewerPosX = std::get_if<float>(&reply->m_inputParams[0]);
				if (viewerPosX == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be a FLOAT");
					return;
				}
				
				float* viewerPosZ = std::get_if<float>(&reply->m_inputParams[1]);
				if (viewerPosZ == nullptr)
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

				//TheWorld_Utils::TimerMs clock;
				//clock.tick();
				float _gridStepInWU;
				std::string meshBuffer;
				m_mapManager->getQuadrantVertices(*lowerXGridVertex, *lowerZGridVertex, *numVerticesPerSize, _gridStepInWU, *level, *meshId, meshBuffer);
				//clock.tock();
				//PLOG_DEBUG << std::string("ELAPSED MapManager::getVertices=") + std::to_string(clock.duration().count()).c_str() + " ms";

				if (expiredTimeToLive(reply))
					return;

				if (_gridStepInWU != *gridStepinWU)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "gridStepinWU not compatible");
					return;
				}

				float f = m_mapManager->calcNextCoordOnTheGridInWUs(*viewerPosX);
				reply->replyParam(f);
				f = m_mapManager->calcNextCoordOnTheGridInWUs(*viewerPosZ);
				reply->replyParam(f);
				
				reply->replyParam(meshBuffer);

				reply->replyComplete();
			}
		}
		catch (TheWorld_Utils::GDN_TheWorld_Exception& e)
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
}
