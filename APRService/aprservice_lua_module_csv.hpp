#pragma once
#include <AL/Common.hpp>

#include <AL/Collections/Tuple.hpp>

#include "aprservice_lua_module_text_file.hpp"

struct aprservice_lua;
struct aprservice_lua_module_csv;

void aprservice_lua_module_csv_register_globals(aprservice_lua* lua);
