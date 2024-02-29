#pragma once
#include <AL/Common.hpp>

#include "aprservice_lua_module_byte_buffer.hpp"

enum APRSERVICE_LUA_MODULE_HTTP_VERSIONS : AL::uint8
{
	APRSERVICE_LUA_MODULE_HTTP_VERSION_1_0,
	APRSERVICE_LUA_MODULE_HTTP_VERSION_1_1
};

enum APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHODS : AL::uint8
{
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_GET,
	APRSERVICE_LUA_MODULE_HTTP_REQUEST_METHOD_POST
};

struct aprservice_lua;
struct aprservice_lua_module_http_request;
struct aprservice_lua_module_http_response;

void                                                                         aprservice_lua_module_http_register_globals(aprservice_lua* lua);

aprservice_lua_module_http_request*                                          aprservice_lua_module_http_request_create(AL::uint8 version, AL::uint8 method);
void                                                                         aprservice_lua_module_http_request_destroy(aprservice_lua_module_http_request* http_request);

// @return exists, string
AL::Collections::Tuple<bool, AL::String>                                     aprservice_lua_module_http_request_get_header(aprservice_lua_module_http_request* http_request, const AL::String& key);
void                                                                         aprservice_lua_module_http_request_set_header(aprservice_lua_module_http_request* http_request, const AL::String& key, const AL::String& value);

AL::uint8                                                                    aprservice_lua_module_http_request_get_method(aprservice_lua_module_http_request* http_request);
AL::uint8                                                                    aprservice_lua_module_http_request_get_version(aprservice_lua_module_http_request* http_request);

// @return success, status_code, string
AL::Collections::Tuple<bool, AL::uint16, AL::String>                         aprservice_lua_module_http_request_download_string(aprservice_lua_module_http_request* http_request, const AL::String& url);
// @return success, status_code, byte_buffer
AL::Collections::Tuple<bool, AL::uint16, aprservice_lua_module_byte_buffer*> aprservice_lua_module_http_request_download_byte_buffer(aprservice_lua_module_http_request* http_request, const AL::String& url, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian);
