#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_ssh
{
};

aprservice_lua_module_ssh* aprservice_lua_module_ssh_init(aprservice_lua* lua)
{
	auto ssh = new aprservice_lua_module_ssh
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return ssh;
}
void                       aprservice_lua_module_ssh_deinit(aprservice_lua_module_ssh* ssh)
{
	delete ssh;
}
