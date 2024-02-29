#pragma once
#include <AL/Common.hpp>

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua;
struct aprservice_lua_module_thread;

typedef AL::Lua54::Function<void()> aprservice_lua_module_thread_main;

void aprservice_lua_module_thread_register_globals(aprservice_lua* lua);

bool                          aprservice_lua_module_thread_run(aprservice_lua_module_thread_main main);

bool                          aprservice_lua_module_thread_is_running(aprservice_lua_module_thread* thread);
aprservice_lua_module_thread* aprservice_lua_module_thread_start(aprservice_lua_module_thread_main main);
void                          aprservice_lua_module_thread_join(aprservice_lua_module_thread* thread);
