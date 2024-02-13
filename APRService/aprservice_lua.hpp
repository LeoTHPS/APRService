#pragma once
#include <AL/Common.hpp>

struct aprservice_lua;

aprservice_lua* aprservice_lua_init();
void            aprservice_lua_deinit(aprservice_lua* lua);

bool            aprservice_lua_run(aprservice_lua* lua, const AL::String& script);
bool            aprservice_lua_run_file(aprservice_lua* lua, const AL::String& script_path);
