#pragma once
#include <AL/Common.hpp>

#include <AL/Collections/Tuple.hpp>

struct aprservice_lua;
struct aprservice_lua_module_json;

void aprservice_lua_module_json_register_globals(aprservice_lua* lua);
