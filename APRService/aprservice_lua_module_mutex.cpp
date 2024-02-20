#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/OS/Mutex.hpp>

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_mutex
{
};

typedef AL::OS::Mutex aprservice_lua_module_mutex_instance;

aprservice_lua_module_mutex_instance* aprservice_lua_module_mutex_create()
{
	return new aprservice_lua_module_mutex_instance();
}
void                                  aprservice_lua_module_mutex_destroy(aprservice_lua_module_mutex_instance* mutex)
{
	delete mutex;
}
void                                  aprservice_lua_module_mutex_lock(aprservice_lua_module_mutex_instance* mutex)
{
	mutex->Lock();
}
void                                  aprservice_lua_module_mutex_unlock(aprservice_lua_module_mutex_instance* mutex)
{
	mutex->Unlock();
}

aprservice_lua_module_mutex* aprservice_lua_module_mutex_init(aprservice_lua* lua)
{
	auto mutex = new aprservice_lua_module_mutex
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_mutex_create);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_mutex_destroy);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_mutex_lock);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_mutex_unlock);

	return mutex;
}
void                         aprservice_lua_module_mutex_deinit(aprservice_lua_module_mutex* mutex)
{
	delete mutex;
}
