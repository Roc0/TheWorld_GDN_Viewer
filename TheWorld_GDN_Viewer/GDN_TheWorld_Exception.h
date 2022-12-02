#pragma once

#include <string>
#include <exception>

using namespace std;

namespace godot
{
	class GDN_TheWorld_Exception : public exception
	{
	public:
		_declspec(dllexport) GDN_TheWorld_Exception(const char* function, const char* message = NULL, const char* message2 = NULL, int rc = 0)
		{
			m_exceptionName = "MapManagerException";
			if (message == NULL || strlen(message) == 0)
				m_message = "MapManager Generic Exception - C++ Exception";
			else
			{
				if (message2 == NULL || strlen(message2) == 0)
				{
					m_message = message;
				}
				else
				{
					m_message = message;
					m_message += " - ";
					m_message += message2;
					m_message += " - rc=";
					m_message += to_string(rc);
				}
			}
		};
		_declspec(dllexport) ~GDN_TheWorld_Exception() {};

		const char* what() const throw ()
		{
			return m_message.c_str();
		}

		const char* exceptionName()
		{
			return m_exceptionName.c_str();
		}
	private:
		string m_message;

	protected:
		string m_exceptionName;
	};

	//class MapManagerExceptionWrongInput : public MapManagerException
	//{
	//public:
	//	_declspec(dllexport) MapManagerExceptionWrongInput(const char* function, const char* message = NULL) : MapManagerException(function, message) { m_exceptionName = "MapManagerExceptionWrongInput"; };
	//};

	//class MapManagerExceptionUnexpectedError : public MapManagerException
	//{
	//public:
	//	_declspec(dllexport) MapManagerExceptionUnexpectedError(const char* function, const char* message = NULL) : MapManagerException(function, message) { m_exceptionName = "MapManagerExceptionUnexpectedError"; };
	//};

	//class MapManagerExceptionDuplicate : public MapManagerException
	//{
	//public:
	//	_declspec(dllexport) MapManagerExceptionDuplicate(const char* function, const char* message = NULL) : MapManagerException(function, message) { m_exceptionName = "MapManagerExceptionDuplicate"; };
	//};

	//class MapManagerExceptionDBException : public MapManagerException
	//{
	//public:
	//	_declspec(dllexport) MapManagerExceptionDBException(const char* function, const char* message = NULL, const char* message2 = NULL, int rc = 0) : MapManagerException(function, message, message2, rc) { m_exceptionName = "MapManagerExceptionDBException"; };
	//};
}
