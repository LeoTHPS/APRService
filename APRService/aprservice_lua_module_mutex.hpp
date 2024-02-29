#pragma once
#include <AL/Common.hpp>

struct aprservice_lua;
struct aprservice_lua_module_mutex;

void                         aprservice_lua_module_mutex_register_globals(aprservice_lua* lua);

aprservice_lua_module_mutex* aprservice_lua_module_mutex_create();
void                         aprservice_lua_module_mutex_destroy(aprservice_lua_module_mutex* mutex);
void                         aprservice_lua_module_mutex_lock(aprservice_lua_module_mutex* mutex);
void                         aprservice_lua_module_mutex_unlock(aprservice_lua_module_mutex* mutex);
