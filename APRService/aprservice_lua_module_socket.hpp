#pragma once
#include <AL/Common.hpp>

#include <AL/Network/IPAddress.hpp>

#include <AL/Collections/Tuple.hpp>

#include "aprservice_lua_module_byte_buffer.hpp"

enum APRSERVICE_LUA_MODULE_SOCKET_TYPES : AL::uint8
{
	APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP      = 0x01,
	APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP      = 0x02,
	APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX     = 0x04,

	APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_TCP = APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX | APRSERVICE_LUA_MODULE_SOCKET_TYPE_TCP,
	APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX_UDP = APRSERVICE_LUA_MODULE_SOCKET_TYPE_UNIX | APRSERVICE_LUA_MODULE_SOCKET_TYPE_UDP
};

enum APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILIES : AL::uint8
{
	APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILY_IPV4          = static_cast<AL::uint8>(AL::Network::AddressFamilies::IPv4),
	APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILY_IPV6          = static_cast<AL::uint8>(AL::Network::AddressFamilies::IPv6),
	APRSERVICE_LUA_MODULE_SOCKET_ADDRESS_FAMILY_NOT_SPECIFIED = static_cast<AL::uint8>(AL::Network::AddressFamilies::NotSpecified)
};

struct aprservice_lua;
struct aprservice_lua_module_socket;

void                                                                                                       aprservice_lua_module_socket_register_globals(aprservice_lua* lua);

aprservice_lua_module_socket*                                                                              aprservice_lua_module_socket_open_tcp(AL::uint8 address_family);
aprservice_lua_module_socket*                                                                              aprservice_lua_module_socket_open_udp(AL::uint8 address_family);
aprservice_lua_module_socket*                                                                              aprservice_lua_module_socket_open_unix_tcp(const AL::String& path, AL::uint8 address_family);
aprservice_lua_module_socket*                                                                              aprservice_lua_module_socket_open_unix_udp(const AL::String& path, AL::uint8 address_family);
void                                                                                                       aprservice_lua_module_socket_close(aprservice_lua_module_socket* socket);

bool                                                                                                       aprservice_lua_module_socket_is_blocking(aprservice_lua_module_socket* socket);
bool                                                                                                       aprservice_lua_module_socket_is_connected(aprservice_lua_module_socket* socket);
bool                                                                                                       aprservice_lua_module_socket_is_listening(aprservice_lua_module_socket* socket);

AL::uint8                                                                                                  aprservice_lua_module_socket_get_type(aprservice_lua_module_socket* socket);
AL::uint8                                                                                                  aprservice_lua_module_socket_get_address_family(aprservice_lua_module_socket* socket);

bool                                                                                                       aprservice_lua_module_socket_set_blocking(aprservice_lua_module_socket* socket, bool set);

bool                                                                                                       aprservice_lua_module_socket_bind(aprservice_lua_module_socket* socket, const AL::String& local_address, AL::uint16 local_port);
bool                                                                                                       aprservice_lua_module_socket_listen(aprservice_lua_module_socket* socket, AL::size_t backlog);
bool                                                                                                       aprservice_lua_module_socket_connect(aprservice_lua_module_socket* socket, const AL::String& remote_host, AL::uint16 remote_port);

// @return success, would_block, socket
AL::Collections::Tuple<bool, bool, aprservice_lua_module_socket*>                                          aprservice_lua_module_socket_accept(aprservice_lua_module_socket* socket);

bool                                                                                                       aprservice_lua_module_socket_send(aprservice_lua_module_socket* socket, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size);
// @return number of bytes sent
AL::size_t                                                                                                 aprservice_lua_module_socket_send_to(aprservice_lua_module_socket* socket, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, const AL::String& remote_host, AL::uint16 remote_port);

// @return connection_closed, would_block, byte_buffer, byte_buffer_size
AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*, AL::size_t>                         aprservice_lua_module_socket_receive(aprservice_lua_module_socket* socket, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian);
// @return connection_closed, would_block, byte_buffer, byte_buffer_size, remote_address, remote_port
AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*, AL::size_t, AL::String, AL::uint16> aprservice_lua_module_socket_receive_from(aprservice_lua_module_socket* socket, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian);
