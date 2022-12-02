#pragma once

#include <MapManager.h>

#include <string>
#include <vector>
#include <variant>
#include <map>
#include <algorithm>
#include <iterator>

typedef std::variant<size_t, int, long, float, std::string> ClientServerVariant;

namespace TheWorld_ClientServer
{
#define THEWORLD_CLIENTSERVER_RC_OK						0
#define THEWORLD_CLIENTSERVER_RC_ALREADY_CONNECTED		1
#define THEWORLD_CLIENTSERVER_RC_METHOD_ONGOING			2
#define THEWORLD_CLIENTSERVER_RC_TIMEOUT_EXPIRED		3
#define THEWORLD_CLIENTSERVER_RC_INDEX_OUT_OF_BORDER	4
#define THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR		5
#define THEWORLD_CLIENTSERVER_RC_KO						-1

	class ClientInterface;
	class ServerInterface;

	class ReplyMethod
	{
		friend ClientInterface;
		friend ServerInterface;
	public:
		ReplyMethod(std::string method, std::vector<ClientServerVariant> inputParams, std::string ref, ClientInterface* client, ServerInterface* server)
		{
			m_replied = false;
			m_error = false;
			m_errorCode = THEWORLD_CLIENTSERVER_RC_OK;
			m_method = method;
			m_inputParams = inputParams;
			//std::copy(inputParams.begin(), inputParams.end(), back_inserter(m_replyParam));
			m_ref = ref;
			m_client = client;
			m_server = server;
		}

		void replyParam(ClientServerVariant replyParam)
		{
			m_replyParams.push_back(replyParam);
		}
		
		void replyComplete(void)
		{
			m_replied = true; 
			m_error = false;
		}

		bool replied(void) { return m_replied; }
		ClientServerVariant getReplyParam(size_t index) { return m_replyParams[index]; }
		size_t getNumReplyParams(void) { return m_replyParams.size(); }
		std::string getMethod(void) { return m_method; }

		void replyError(int errorCode, std::string errorMessage)
		{
			m_error = true;
			m_errorCode = errorCode;
			m_errorMessage = errorMessage;
		}
		
		bool error(void) { return m_error; }
		int getErrorCode(void) { return m_errorCode; };
		std::string getErrorMessage(void) { return m_errorMessage; };

		void onExecMethodAsync(void);

	private:
		bool m_replied;
		bool m_error;
		int m_errorCode;
		std::string m_errorMessage;
		std::string m_method;
		std::vector<ClientServerVariant> m_inputParams;
		std::vector<ClientServerVariant> m_replyParams;
		std::string m_ref;
		ClientInterface* m_client;
		ServerInterface* m_server;
	};

	typedef std::map<std::string, std::unique_ptr<ReplyMethod>> MapReplyMethod;

	class ClientInterface
	{
		friend ServerInterface;
	public:
		ClientInterface(plog::Severity sev);
		~ClientInterface(void);
		virtual int connect(void);
		virtual void disconnect(void);
		virtual int execMethodAsync(std::string method, std::string& ref, std::vector<ClientServerVariant>& inputParams);
		virtual int execMethodSync(std::string method, std::vector<ClientServerVariant>& inputParams, std::vector <ClientServerVariant>& replyParams);
		virtual int getReplyParam(std::string& ref, size_t index, ClientServerVariant& ReplyParam);
		virtual int getReplyParams(std::string& ref, std::vector <ClientServerVariant>& replyParams);
		virtual int replied(std::string& method, std::string& ref, bool& replied, size_t& numReplyParams, bool& error, int& errorCode, std::string& errorMessage);

	private:
		ServerInterface* m_server;
		plog::Severity m_sev;
		std::recursive_mutex m_mutexReplyMap;
		MapReplyMethod m_ReplyMap;
		long m_msTimeout;
	};

	class ServerInterface
	{
	public:
		ServerInterface(plog::Severity sev);
		~ServerInterface(void);
		virtual int onConnect(ClientInterface* client);
		virtual void onDisconnect(void);
		virtual void onExecMethodAsync(std::string method, ReplyMethod* reply);

	private:
		ClientInterface* m_client;
		TheWorld_MapManager::MapManager* m_mapManager;
		plog::Severity m_sev;
	};
}

