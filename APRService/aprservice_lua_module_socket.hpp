#pragma once
#include <AL/Common.hpp>

#include <AL/Collections/Tuple.hpp>

#include "aprservice_lua_module_byte_buffer.hpp"

enum APRSERVICE_LUA_MODULE_SOCKET_TYPES : AL::uint8
{
	APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP,
	APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP,
	APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX
};

struct aprservice_lua;
struct aprservice_lua_module_socket;

void                          aprservice_lua_module_socket_register_globals(aprservice_lua* lua);

aprservice_lua_module_socket* aprservice_lua_module_socket_open_tcp();
aprservice_lua_module_socket* aprservice_lua_module_socket_open_udp();
aprservice_lua_module_socket* aprservice_lua_module_socket_open_unix();
void                          aprservice_lua_module_socket_close(aprservice_lua_module_socket* socket);

AL::uint8                     aprservice_lua_module_socket_get_type(aprservice_lua_module_socket* socket);
