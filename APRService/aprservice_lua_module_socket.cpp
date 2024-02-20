#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_socket
{
};

aprservice_lua_module_socket* aprservice_lua_module_socket_init(aprservice_lua* lua)
{
	auto socket = new aprservice_lua_module_socket
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return socket;
}
void                          aprservice_lua_module_socket_deinit(aprservice_lua_module_socket* socket)
{
	delete socket;
}
