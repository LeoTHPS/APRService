#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_mutex.hpp"

#include <AL/OS/Mutex.hpp>

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_mutex
{
	AL::OS::Mutex mutex;
};

void                         aprservice_lua_module_mutex_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_mutex_create);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_mutex_destroy);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_mutex_lock);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_mutex_unlock);
}

aprservice_lua_module_mutex* aprservice_lua_module_mutex_create()
{
	return new aprservice_lua_module_mutex();
}
void                         aprservice_lua_module_mutex_destroy(aprservice_lua_module_mutex* mutex)
{
	delete mutex;
}
void                         aprservice_lua_module_mutex_lock(aprservice_lua_module_mutex* mutex)
{
	mutex->mutex.Lock();
}
void                         aprservice_lua_module_mutex_unlock(aprservice_lua_module_mutex* mutex)
{
	mutex->mutex.Unlock();
}
