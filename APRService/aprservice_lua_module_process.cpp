#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_process.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/OS/Process.hpp>

struct aprservice_lua_module_process
{
};

void aprservice_lua_module_process_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	// TODO: implement
}
