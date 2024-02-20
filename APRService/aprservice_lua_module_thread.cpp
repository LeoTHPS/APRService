#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/OS/Thread.hpp>

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_thread
{
};

aprservice_lua_module_thread* aprservice_lua_module_thread_init(aprservice_lua* lua)
{
	auto thread = new aprservice_lua_module_thread
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return thread;
}
void                          aprservice_lua_module_thread_deinit(aprservice_lua_module_thread* thread)
{
	delete thread;
}
