//#include "pch.h"
#include "ClientServer.h"
#include "Utils.h"

#include <Mutex>

#include <Rpc.h>

namespace TheWorld_ClientServer
{
	void ReplyMethod::onExecMethodAsync(void)
	{
		m_server->onExecMethodAsync(m_method, this);
	}
	
	ClientInterface::ClientInterface(plog::Severity sev)
	{
		m_server = nullptr;
		m_sev = sev;
		m_msTimeout = 10000;
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

		return THEWORLD_CLIENTSERVER_RC_OK;
	}

	void ClientInterface::disconnect(void)
	{
		if (m_server)
		{
			m_server->onDisconnect();
			delete m_server;
			m_server = nullptr;

			PLOG_INFO << "ClientInterface::disconnect - Server disconnected";
		}
	}

	int ClientInterface::execMethodAsync(std::string method, std::string& ref, std::vector<ClientServerVariant>& inputParams)
	{
		GUID newId;
		RPC_STATUS ret_val = ::UuidCreate(&newId);
		if (ret_val != RPC_S_OK)
		{
			PLOG_ERROR << "UuidCreate in error with rc " + std::to_string(ret_val);
			return THEWORLD_CLIENTSERVER_RC_KO;
		}

		//WCHAR* wszUuid = NULL;
		//::UuidToStringW(&newId, (RPC_WSTR*)&wszUuid);
		//if (wszUuid == NULL)
		//{
		//	m_errMessage = "UuidToStringW in error";
		//	return THEWORLD_CLIENTSERVER_RC_KO;
		//}
		
		{
			std::lock_guard<std::recursive_mutex> lock(m_mutexReplyMap);
			ref = ToString(&newId);
			m_ReplyMap[ref] = make_unique<ReplyMethod>(method, inputParams, ref, this, m_server);
			std::thread* th = new std::thread(&ReplyMethod::onExecMethodAsync, m_ReplyMap[ref].get());
		}
		
		// free up the allocated string
		//::RpcStringFreeW((RPC_WSTR*)&wszUuid);
		//wszUuid = NULL;

		return THEWORLD_CLIENTSERVER_RC_OK;
	}
	
	int ClientInterface::replied(std::string& method, std::string& ref, bool& replied, size_t& numReplyParams, bool& error, int& errorCode, std::string& errorMessage)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutexReplyMap);

		if (m_ReplyMap.find(ref) == m_ReplyMap.end())
		{
			PLOG_ERROR << "Cannot find ref in ReplyMap";
			return THEWORLD_CLIENTSERVER_RC_KO;
		}

		ReplyMethod* reply = m_ReplyMap[ref].get();

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
		std::lock_guard<std::recursive_mutex> lock(m_mutexReplyMap);

		if (m_ReplyMap.find(ref) == m_ReplyMap.end())
		{
			PLOG_ERROR << "Cannot find ref in ReplyMap";
			return THEWORLD_CLIENTSERVER_RC_KO;
		}

		ReplyMethod* reply = m_ReplyMap[ref].get();

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
		std::lock_guard<std::recursive_mutex> lock(m_mutexReplyMap);

		if (m_ReplyMap.find(ref) == m_ReplyMap.end())
		{
			PLOG_ERROR << "Cannot find ref in ReplyMap";
			return THEWORLD_CLIENTSERVER_RC_KO;
		}

		ReplyMethod* reply = m_ReplyMap[ref].get();

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

	int ClientInterface::execMethodSync(std::string method, std::vector<ClientServerVariant>& inputParams, std::vector <ClientServerVariant>& replyParams)
	{
		PLOG_DEBUG << "execMethodSync: inizio";

		int rc = THEWORLD_CLIENTSERVER_RC_OK;

		replyParams.clear();

		std::string ref;
		
#define SHORT_ROAD 0
		if (SHORT_ROAD)
		{
			GUID newId;
			RPC_STATUS ret_val = ::UuidCreate(&newId);
			if (ret_val != RPC_S_OK)
			{
				PLOG_ERROR << "UuidCreate in error with rc " + std::to_string(ret_val);
				return THEWORLD_CLIENTSERVER_RC_KO;
			}
			
			ReplyMethod* reply = nullptr;
			{
				std::lock_guard<std::recursive_mutex> lock(m_mutexReplyMap);
				ref = ToString(&newId);
				m_ReplyMap[ref] = make_unique<ReplyMethod>(method, inputParams, ref, this, m_server);
				reply = m_ReplyMap[ref].get();
				//std::thread* th = new std::thread(&ReplyMethod::onExecMethodAsync, m_ReplyMap[ref].get());
			}
			reply->onExecMethodAsync();
		}
		else		
			rc = execMethodAsync(method, ref, inputParams);
		
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
			return rc;

		PLOG_DEBUG << "execMethodSync: attesa risposta";
		TimerMs clock;
		clock.tick();
		while (true)
		{
			bool replied = false, error = false;
			size_t numReplyParams = 0;
			int errorCode;
			std::string errorMessage;
			std::string method;
			rc = ClientInterface::replied(method, ref, replied, numReplyParams, error, errorCode, errorMessage);
			if (rc != THEWORLD_CLIENTSERVER_RC_OK)
				return rc;
			
			if (error)
			{
				PLOG_ERROR << "Method execution error - Method: " + method + " - Error: " + std::to_string(errorCode) + " - " + errorMessage;
				return THEWORLD_CLIENTSERVER_RC_KO;
			}

			if (!replied)
			{
				clock.tock();
				long long elapsed = clock.duration().count();
				if (elapsed > m_msTimeout)
				{
					rc = THEWORLD_CLIENTSERVER_RC_TIMEOUT_EXPIRED;
					break;
				}
				else
					Sleep(5);
			}
			else
			{
				PLOG_DEBUG << "execMethodSync: callect params";
				rc = getReplyParams(ref, replyParams);
				//for (size_t idx = 0; idx < numReplyParams; idx++)
				//{
				//	ClientServerVariant replyParam;
				//	rc = getReplyParam(ref, idx, replyParam);
				//	if (rc != THEWORLD_CLIENTSERVER_RC_OK)
				//		return rc;
				//	replyParams.push_back(replyParam);
				//}
				break;
			}
		}

		PLOG_DEBUG << "execMethodSync: fine";

		return rc;
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
		m_mapManager->instrument(true);
		m_mapManager->consoleDebugMode(false);
		PLOG_INFO << "ServerInterface::onConnect - Server connected";
		return THEWORLD_CLIENTSERVER_RC_OK;
	}

	void ServerInterface::onDisconnect(void)
	{
		if (m_client != nullptr)
		{
			m_client = nullptr;
			delete m_mapManager;
			PLOG_INFO << "ServerInterface::onDisconnect - Server Disconnected";
		}
	}

	void ServerInterface::onExecMethodAsync(std::string method, ReplyMethod* reply)
	{
		try
		{
			if (method == "MapManager::gridStepInWU")
			{
				float f = m_mapManager->gridStepInWU();
				reply->replyParam(f);
				reply->replyComplete();
			}
			else if (method == "MapManager::setLogMaxSeverity")
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
			else if (method == "MapManager::calcNextCoordOnTheGridInWUs")
			{
				for (size_t i = 0; i < reply->m_inputParams.size(); i++)
				{
					float* coord = std::get_if<float>(&reply->m_inputParams[i]);
					if (coord == nullptr)
					{
						reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be a FLOAT");
						return;
					}
					float f = m_mapManager->calcNextCoordOnTheGridInWUs(*coord);
					reply->replyParam(f);
				}
				reply->replyComplete();
			}
			else if (method == "MapManager::getVertices")
			{
				std::vector<TheWorld_MapManager::SQLInterface::GridVertex> worldVertices;
				
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
				
				int* i = std::get_if<int>(&reply->m_inputParams[4]);
				if (i == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Fifth param must be an INT");
					return;
				}
				TheWorld_MapManager::MapManager::anchorType anchorType = TheWorld_MapManager::MapManager::anchorType (*i);
				
				int* numVerticesPerSize = std::get_if<int>(&reply->m_inputParams[5]);
				if (numVerticesPerSize == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Sixth param must be an INT");
					return;
				}
				
				float* gridStepinWU = std::get_if<float>(&reply->m_inputParams[6]);
				if (gridStepinWU == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Seventh param must be a FLOAT");
					return;
				}
				
				int* level = std::get_if<int>(&reply->m_inputParams[7]);
				if (level == nullptr)
				{
					reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "Eight param must be an INT");
					return;
				}

				TimerMs clock; // Timer<milliseconds, steady_clock>
				clock.tick();
				m_mapManager->getVertices(*lowerXGridVertex, *lowerZGridVertex,
															   anchorType,
															   *numVerticesPerSize, *numVerticesPerSize,
															   worldVertices, *gridStepinWU, *level);
				clock.tock();
				PLOG_DEBUG << std::string("DURATION MapManager::getVertices=") + std::to_string(clock.duration().count()).c_str() + " ms";

				float f = m_mapManager->calcNextCoordOnTheGridInWUs(*viewerPosX);
				reply->replyParam(f);
				f = m_mapManager->calcNextCoordOnTheGridInWUs(*viewerPosZ);
				reply->replyParam(f);
				
				{
					BYTE shortBuffer[256 + 1];
					size_t size_tSize = 0;
					serializeToByteStream<size_t>(size_tSize, shortBuffer, size_tSize);

					// Serialize an empty GridVertex only to obtain the size of a serialized GridVertex
					size_t serializedVertexSize = 0;
					TheWorld_Utils::GridVertex v;
					v.serialize(shortBuffer, serializedVertexSize);

					size_t vertexArraySize = (*numVerticesPerSize) * (*numVerticesPerSize);
					size_t streamBufferSize = 1 /* "0" */ + size_tSize /* the size of a size_t */ + vertexArraySize * serializedVertexSize;
					BYTE* streamBuffer = (BYTE*)calloc(1, streamBufferSize);
					if (streamBuffer == nullptr)
						throw(std::exception((std::string(__FUNCTION__) + std::string("Allocation error")).c_str()));

					size_t streamBufferIterator = 0;
					memcpy(streamBuffer + streamBufferIterator, "0", 1);
					streamBufferIterator++;
					
					size_t size = 0;
					serializeToByteStream<size_t>(vertexArraySize, streamBuffer + streamBufferIterator, size);
					streamBufferIterator += size;

					for (int z = 0; z < *numVerticesPerSize; z++)			// m_heightMapImage->get_height()
						for (int x = 0; x < *numVerticesPerSize; x++)		// m_heightMapImage->get_width()
						{
							TheWorld_MapManager::SQLInterface::GridVertex& v = worldVertices[z * *numVerticesPerSize + x];
							TheWorld_Utils::GridVertex v1(v.posX(), v.altitude(), v.posZ(), *level);
							v1.serialize(streamBuffer + streamBufferIterator, size);
							streamBufferIterator += size;
						}

					std::string GridVertexStream((char*)streamBuffer, streamBufferSize);
					reply->replyParam(GridVertexStream);

					free(streamBuffer);
				}

				reply->replyComplete();
			}
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

}