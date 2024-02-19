#pragma once
#include <AL/Common.hpp>

namespace AL::Lua54
{
	class Lua;
}

struct                 aprservice_lua;
typedef AL::Lua54::Lua aprservice_lua_state;

aprservice_lua*       aprservice_lua_init();
void                  aprservice_lua_deinit(aprservice_lua* lua);

aprservice_lua_state* aprservice_lua_get_state(aprservice_lua* lua);

bool                  aprservice_lua_run(aprservice_lua* lua, const AL::String& script);
bool                  aprservice_lua_run_file(aprservice_lua* lua, const AL::String& script_path);

#define               aprservice_lua_state_register_global(lua_state, value)                      aprservice_lua_state_register_global_ex(lua_state, value, #value)
#define               aprservice_lua_state_register_global_ex(lua_state, value, name)             lua_state->SetGlobal(name, value)
#define               aprservice_lua_state_register_global_function(lua_state, function)          aprservice_lua_state_register_global_function_ex(lua_state, function, #function)
#define               aprservice_lua_state_register_global_function_ex(lua_state, function, name) lua_state->SetGlobalFunction<function>(name)
