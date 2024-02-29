#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_socket.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Network/TcpSocket.hpp>
#include <AL/Network/UdpSocket.hpp>
#include <AL/Network/UnixSocket.hpp>

struct aprservice_lua_module_socket
{
};

void                          aprservice_lua_module_socket_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	// TODO: implement
}
