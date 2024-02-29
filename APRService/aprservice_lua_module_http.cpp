#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_http.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Network/HTTP/Request.hpp>
#include <AL/Network/HTTP/Response.hpp>

struct aprservice_lua_module_http_request
{
	AL::Network::HTTP::Request request;
};

struct aprservice_lua_module_http_response
{
	AL::Network::HTTP::Response response;
};

void                                                                         aprservice_lua_module_http_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_VERSION_1_0);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_VERSION_1_1);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_GET);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_POST);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_create);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_destroy);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_get_header);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_set_header);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_get_method);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_get_version);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_download_string);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_http_request_download_byte_buffer);
}

AL::uint8                                                                    aprservice_lua_module_http_version_to_lua(AL::Network::HTTP::Versions value)
{
	switch (value)
	{
		case AL::Network::HTTP::Versions::HTTP_1_0: return APRSERVICE_LUA_MODULE_HTTP_VERSION_1_0;
		case AL::Network::HTTP::Versions::HTTP_1_1: return APRSERVICE_LUA_MODULE_HTTP_VERSION_1_1;
	}

	return APRSERVICE_LUA_MODULE_HTTP_VERSION_1_1;
}
AL::Network::HTTP::Versions                                                  aprservice_lua_module_http_version_from_lua(AL::uint8 value)
{
	switch (value)
	{
		case APRSERVICE_LUA_MODULE_HTTP_VERSION_1_0: return AL::Network::HTTP::Versions::HTTP_1_0;
		case APRSERVICE_LUA_MODULE_HTTP_VERSION_1_1: return AL::Network::HTTP::Versions::HTTP_1_1;
	}

	return AL::Network::HTTP::Versions::HTTP_1_1;
}

AL::uint8                                                                    aprservice_lua_module_http_request_method_to_lua(AL::Network::HTTP::RequestMethods value)
{
	switch (value)
	{
		case AL::Network::HTTP::RequestMethods::GET:  return APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_GET;
		case AL::Network::HTTP::RequestMethods::POST: return APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_POST;
		default:                                      break;
	}

	return APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_GET;
}
AL::Network::HTTP::RequestMethods                                            aprservice_lua_module_http_request_method_from_lua(AL::uint8 value)
{
	switch (value)
	{
		case APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_GET:  return AL::Network::HTTP::RequestMethods::GET;
		case APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_POST: return AL::Network::HTTP::RequestMethods::POST;
	}

	return AL::Network::HTTP::RequestMethods::GET;
}

aprservice_lua_module_http_request*                                          aprservice_lua_module_http_request_create(AL::uint8 version, AL::uint8 method)
{
	auto http_request = new aprservice_lua_module_http_request
	{
		.request = AL::Network::HTTP::Request(aprservice_lua_module_http_version_from_lua(version), aprservice_lua_module_http_request_method_from_lua(method))
	};

	return http_request;
}
void                                                                         aprservice_lua_module_http_request_destroy(aprservice_lua_module_http_request* http_request)
{
	delete http_request;
}

// @return exists, string
AL::Collections::Tuple<bool, AL::String>                                     aprservice_lua_module_http_request_get_header(aprservice_lua_module_http_request* http_request, const AL::String& key)
{
	AL::Collections::Tuple<bool, AL::String> value(false, "");

	if (auto it = http_request->request.GetHeader().Find(key); it != http_request->request.GetHeader().end())
	{
		value.Set<0>(true);
		value.Set<1>(it->Value);
	}

	return value;
}
void                                                                         aprservice_lua_module_http_request_set_header(aprservice_lua_module_http_request* http_request, const AL::String& key, const AL::String& value)
{
	http_request->request.GetHeader()[key] = value;
}

AL::uint8                                                                    aprservice_lua_module_http_request_get_method(aprservice_lua_module_http_request* http_request)
{
	return aprservice_lua_module_http_request_method_to_lua(http_request->request.GetMethod());
}
AL::uint8                                                                    aprservice_lua_module_http_request_get_version(aprservice_lua_module_http_request* http_request)
{
	return aprservice_lua_module_http_version_to_lua(http_request->request.GetVersion());
}

// @return success, status_code, string
AL::Collections::Tuple<bool, AL::uint16, AL::String>                         aprservice_lua_module_http_request_download_string(aprservice_lua_module_http_request* http_request, const AL::String& url)
{
	AL::Collections::Tuple<bool, AL::uint16, AL::String> value(false, 0, "");

	try
	{
		auto http_response = http_request->request.Execute(AL::Network::HTTP::Uri::FromString(url));
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
AL::Collections::Tuple<bool, AL::uint16, aprservice_lua_module_byte_buffer*> aprservice_lua_module_http_request_download_byte_buffer(aprservice_lua_module_http_request* http_request, const AL::String& url, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian)
{
	AL::Collections::Tuple<bool, AL::uint16, aprservice_lua_module_byte_buffer*> value(false, 0, nullptr);

	try
	{
		auto http_response = http_request->request.Execute(AL::Network::HTTP::Uri::FromString(url));
		value.Set<1>(static_cast<AL::uint16>(http_response.GetStatus()));

		if (http_response.GetStatus() == AL::Network::HTTP::StatusCodes::OK)
		{
			auto byte_buffer = aprservice_lua_module_byte_buffer_create(byte_buffer_endian, http_response.GetContent().GetLength());
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
