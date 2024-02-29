#pragma once
#include <AL/Common.hpp>

#include <AL/Collections/Tuple.hpp>

#include "aprservice_lua_module_byte_buffer.hpp"

struct aprservice_lua;
struct aprservice_lua_module_process;

void aprservice_lua_module_process_register_globals(aprservice_lua* lua);
