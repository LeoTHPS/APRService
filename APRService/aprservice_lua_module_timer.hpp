#pragma once
#include <AL/Common.hpp>

struct aprservice_lua;
struct aprservice_lua_module_timer;

void aprservice_lua_module_timer_register_globals(aprservice_lua* lua);

aprservice_lua_module_timer* aprservice_lua_module_timer_create();
void                         aprservice_lua_module_timer_destroy(aprservice_lua_module_timer* timer);
void                         aprservice_lua_module_timer_reset(aprservice_lua_module_timer* timer);
AL::uint64                   aprservice_lua_module_timer_get_elapsed_ms(aprservice_lua_module_timer* timer);
AL::uint64                   aprservice_lua_module_timer_get_elapsed_us(aprservice_lua_module_timer* timer);
