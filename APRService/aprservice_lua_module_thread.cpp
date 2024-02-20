#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/OS/Thread.hpp>

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_thread
{
};

typedef AL::Lua54::Function<void()> aprservice_lua_module_thread_main;
typedef AL::OS::Thread              aprservice_lua_module_thread_instance;

bool                                   aprservice_lua_module_thread_is_running(aprservice_lua_module_thread_instance* thread)
{
	return thread->IsRunning();
}
aprservice_lua_module_thread_instance* aprservice_lua_module_thread_start(aprservice_lua_module_thread_main main)
{
	auto thread = new aprservice_lua_module_thread_instance();

	try
	{
		thread->Start([main]() { main(); });
	}
	catch (const AL::Exception& exception)
	{
		delete thread;

		aprservice_console_write_line("Error starting AL::OS::Thread");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return thread;
}
void                                   aprservice_lua_module_thread_join(aprservice_lua_module_thread_instance* thread)
{
	try
	{
		thread->Join();
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error joining AL::OS::Thread");
		aprservice_console_write_exception(exception);
	}

	delete thread;
}

aprservice_lua_module_thread* aprservice_lua_module_thread_init(aprservice_lua* lua)
{
	auto thread = new aprservice_lua_module_thread
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_thread_is_running);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_thread_start);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_thread_join);

	return thread;
}
void                          aprservice_lua_module_thread_deinit(aprservice_lua_module_thread* thread)
{
	delete thread;
}
