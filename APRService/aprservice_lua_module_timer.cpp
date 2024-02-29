#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_timer.hpp"

#include <AL/OS/Timer.hpp>

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_timer
{
	AL::OS::Timer timer;
};

void aprservice_lua_module_timer_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_create);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_destroy);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_reset);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_get_elapsed_ms);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_timer_get_elapsed_us);
}

aprservice_lua_module_timer* aprservice_lua_module_timer_create()
{
	return new aprservice_lua_module_timer();
}
void                         aprservice_lua_module_timer_destroy(aprservice_lua_module_timer* timer)
{
	delete timer;
}
void                         aprservice_lua_module_timer_reset(aprservice_lua_module_timer* timer)
{
	timer->timer.Reset();
}
AL::uint64                   aprservice_lua_module_timer_get_elapsed_ms(aprservice_lua_module_timer* timer)
{
	return timer->timer.GetElapsed().ToMilliseconds();
}
AL::uint64                   aprservice_lua_module_timer_get_elapsed_us(aprservice_lua_module_timer* timer)
{
	return timer->timer.GetElapsed().ToMicroseconds();
}
