#pragma once
#include <AL/Common.hpp>

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua;

// @return false to stop enumerating
typedef AL::Lua54::Function<bool(const AL::String& name, const AL::String& value)> aprservice_lua_module_environment_enum_callback;

void                                     aprservice_lua_module_environment_register_globals(aprservice_lua* lua);

// @return exists, value
AL::Collections::Tuple<bool, AL::String> aprservice_lua_module_environment_get(const AL::String& name);
bool                                     aprservice_lua_module_environment_set(const AL::String& name, const AL::String& value);
bool                                     aprservice_lua_module_environment_delete(const AL::String& name);
bool                                     aprservice_lua_module_environment_enumerate(aprservice_lua_module_environment_enum_callback callback);
