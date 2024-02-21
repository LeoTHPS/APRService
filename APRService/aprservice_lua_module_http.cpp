#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_byte_buffer.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Network/HTTP/Client.hpp>

struct aprservice_lua_module_http
{
};

typedef typename AL::Get_Enum_Or_Integer_Base<AL::Network::HTTP::Versions>::Type       APRSERVICE_LUA_MODULE_HTTP_VERSION;
typedef typename AL::Get_Enum_Or_Integer_Base<AL::Network::HTTP::RequestMethods>::Type APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD;

enum APRSERVICE_LUA_MODULE_HTTP_VERSIONS : APRSERVICE_LUA_MODULE_HTTP_VERSION
{
	APRSERVICE_LUA_MODULE_HTTP_VERSION_1_0 = static_cast<APRSERVICE_LUA_MODULE_HTTP_VERSION>(AL::Network::HTTP::Versions::HTTP_1_0),
	APRSERVICE_LUA_MODULE_HTTP_VERSION_1_1 = static_cast<APRSERVICE_LUA_MODULE_HTTP_VERSION>(AL::Network::HTTP::Versions::HTTP_1_1)
};

enum APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHODS : APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD
{
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_GET     = static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(AL::Network::HTTP::RequestMethods::GET),
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_HEAD    = static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(AL::Network::HTTP::RequestMethods::HEAD),
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_POST    = static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(AL::Network::HTTP::RequestMethods::POST),
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_PUT     = static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(AL::Network::HTTP::RequestMethods::PUT),
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_DELETE  = static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(AL::Network::HTTP::RequestMethods::DELETE),
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_CONNECT = static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(AL::Network::HTTP::RequestMethods::CONNECT),
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_OPTIONS = static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(AL::Network::HTTP::RequestMethods::OPTIONS),
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_TRACE   = static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(AL::Network::HTTP::RequestMethods::TRACE),
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_PATCH   = static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(AL::Network::HTTP::RequestMethods::PATCH)
};

typedef AL::Network::HTTP::Request  aprservice_lua_module_http_request;
typedef AL::Network::HTTP::Response aprservice_lua_module_http_response;

aprservice_lua_module_http_request*       aprservice_lua_module_http_request_create(APRSERVICE_LUA_MODULE_HTTP_VERSION version, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD method)
{
	return new aprservice_lua_module_http_request(static_cast<AL::Network::HTTP::Versions>(version), static_cast<AL::Network::HTTP::RequestMethods>(method));
}
void                                      aprservice_lua_module_http_request_destroy(aprservice_lua_module_http_request* http_request)
{
	delete http_request;
}

// @return exists, string
auto                                      aprservice_lua_module_http_request_get_header(aprservice_lua_module_http_request* http_request, const AL::String& key)
{
	AL::Collections::Tuple<bool, AL::String> value(false, "");

	if (auto it = http_request->GetHeader().Find(key); it != http_request->GetHeader().end())
	{
		value.Set<0>(true);
		value.Set<1>(it->Value);
	}

	return value;
}
void                                      aprservice_lua_module_http_request_set_header(aprservice_lua_module_http_request* http_request, const AL::String& key, const AL::String& value)
{
	http_request->GetHeader()[key] = value;
}

APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD aprservice_lua_module_http_request_get_method(aprservice_lua_module_http_request* http_request)
{
	return static_cast<APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD>(http_request->GetMethod());
}
APRSERVICE_LUA_MODULE_HTTP_VERSION        aprservice_lua_module_http_request_get_version(aprservice_lua_module_http_request* http_request)
{
	return static_cast<APRSERVICE_LUA_MODULE_HTTP_VERSION>(http_request->GetVersion());
}

// @return success, status_code, string
auto                                      aprservice_lua_module_http_request_download_string(aprservice_lua_module_http_request* http_request, const AL::String& url)
{
	AL::Collections::Tuple<bool, AL::uint16, AL::String> value(false, 0, "");

	try
	{
		auto http_response = http_request->Execute(AL::Network::HTTP::Uri::FromString(url));
		value.Set<1>(static_cast<AL::uint16>(http_response.GetStatus()));

		if (http_response.GetStatus() == AL::Network::HTTP::StatusCodes::OK)
		{
			value.Set<0>(true);
			value.Set<2>(http_response.GetContent());
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error executing AL::Network::HTTP::Request");
		aprservice_console_write_exception(exception);
	}

	return value;
}
// @return success, status_code, byte_buffer
auto                                      aprservice_lua_module_http_request_download_byte_buffer(aprservice_lua_module_http_request* http_request, const AL::String& url)
{
	AL::Collections::Tuple<bool, AL::uint16, aprservice_lua_module_byte_buffer_instance*> value(false, 0, nullptr);

	try
	{
		auto http_response = http_request->Execute(AL::Network::HTTP::Uri::FromString(url));
		value.Set<1>(static_cast<AL::uint16>(http_response.GetStatus()));

		if (http_response.GetStatus() == AL::Network::HTTP::StatusCodes::OK)
		{
			auto byte_buffer = aprservice_lua_module_byte_buffer_create(APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN_MACHINE, http_response.GetContent().GetLength());
			aprservice_lua_module_byte_buffer_write(byte_buffer, http_response.GetContent().GetCString(), http_response.GetContent().GetLength());

			value.Set<0>(true);
			value.Set<2>(byte_buffer);
		}
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error executing AL::Network::HTTP::Request");
		aprservice_console_write_exception(exception);
	}

	return value;
}

aprservice_lua_module_http* aprservice_lua_module_http_init(aprservice_lua* lua)
{
	auto http = new aprservice_lua_module_http
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_VERSION_1_0);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_VERSION_1_1);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_GET);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_HEAD);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_POST);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_PUT);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_DELETE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_CONNECT);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_OPTIONS);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_TRACE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_PATCH);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_create);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_destroy);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_get_header);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_set_header);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_get_method);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_get_version);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_download_string);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_download_byte_buffer);

	return http;
}
void                        aprservice_lua_module_http_deinit(aprservice_lua_module_http* http)
{
	delete http;
}
