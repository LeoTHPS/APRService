#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_thread.hpp"

#include <AL/OS/Thread.hpp>

struct aprservice_lua_module_thread
{
	AL::OS::Thread thread;
};

void                          aprservice_lua_module_thread_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_thread_run);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_thread_is_running);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_thread_start);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_thread_join);
}

bool                          aprservice_lua_module_thread_run(aprservice_lua_module_thread_main main)
{
	try
	{
		AL::OS::Thread::StartAndDetach([main]() { main(); });
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error starting and detaching AL::OS::Thread");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}

bool                          aprservice_lua_module_thread_is_running(aprservice_lua_module_thread* thread)
{
	return thread->thread.IsRunning();
}

aprservice_lua_module_thread* aprservice_lua_module_thread_start(aprservice_lua_module_thread_main main)
{
	auto thread = new aprservice_lua_module_thread();

	try
	{
		thread->thread.Start([main]() { main(); });
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
void                          aprservice_lua_module_thread_join(aprservice_lua_module_thread* thread)
{
	try
	{
		thread->thread.Join();
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error joining AL::OS::Thread");
		aprservice_console_write_exception(exception);
	}

	delete thread;
}
