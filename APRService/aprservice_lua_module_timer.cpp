#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/OS/Timer.hpp>

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_timer
{
};

typedef AL::OS::Timer aprservice_lua_module_timer_instance;

aprservice_lua_module_timer_instance* aprservice_lua_module_timer_create()
{
	return new aprservice_lua_module_timer_instance();
}
void                                  aprservice_lua_module_timer_destroy(aprservice_lua_module_timer_instance* timer)
{
	delete timer;
}
void                                  aprservice_lua_module_timer_reset(aprservice_lua_module_timer_instance* timer)
{
	timer->Reset();
}
AL::uint64                            aprservice_lua_module_timer_get_elapsed_ms(aprservice_lua_module_timer_instance* timer)
{
	return timer->GetElapsed().ToMilliseconds();
}
AL::uint64                            aprservice_lua_module_timer_get_elapsed_us(aprservice_lua_module_timer_instance* timer)
{
	return timer->GetElapsed().ToMicroseconds();
}

aprservice_lua_module_timer* aprservice_lua_module_timer_init(aprservice_lua* lua)
{
	auto timer = new aprservice_lua_module_timer
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_create);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_destroy);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_reset);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_get_elapsed_ms);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_get_elapsed_us);

	return timer;
}
void                         aprservice_lua_module_timer_deinit(aprservice_lua_module_timer* timer)
{
	delete timer;
}
