#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_process
{
};

aprservice_lua_module_process* aprservice_lua_module_process_init(aprservice_lua* lua)
{
	auto process = new aprservice_lua_module_process
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return process;
}
void                       aprservice_lua_module_process_deinit(aprservice_lua_module_process* process)
{
	delete process;
}
