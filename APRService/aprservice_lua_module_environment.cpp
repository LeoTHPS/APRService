#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/OS/Environment.hpp>

struct aprservice_lua_module_environment
{
};

// @return false to stop enumerating
typedef AL::Lua54::Function<bool(const AL::String& name, const AL::String& value)> aprservice_lua_module_environment_enum_callback;

// @return exists, value
auto aprservice_lua_module_environment_get(const AL::String& name)
{
	AL::Collections::Tuple<bool, AL::String> value;

	try
	{
		value.Set<0>(AL::OS::Environment::Get(value.Get<1>(), name));
	}
	catch (const AL::Exception& exception)
	{
		value.Set<0>(false);

		aprservice_console_write_line("Error calling AL::OS::Environment::Get");
		aprservice_console_write_exception(exception);
	}

	return value;
}
bool aprservice_lua_module_environment_set(const AL::String& name, const AL::String& value)
{
	try
	{
		AL::OS::Environment::Set(name, value);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error calling AL::OS::Environment::Set");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
bool aprservice_lua_module_environment_delete(const AL::String& name)
{
	try
	{
		AL::OS::Environment::Delete(name);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error calling AL::OS::Environment::Delete");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
bool aprservice_lua_module_environment_enumerate(aprservice_lua_module_environment_enum_callback callback)
{
	AL::OS::EnvironmentOnEnumCallback callback_detour([&callback](const AL::String& name, const AL::String& value)
	{
		return callback(name, value);
	});

	try
	{
		AL::OS::Environment::Enumerate(callback_detour);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error calling AL::OS::Environment::Enumerate");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}

aprservice_lua_module_environment* aprservice_lua_module_environment_init(aprservice_lua* lua)
{
	auto environment = new aprservice_lua_module_environment
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_environment_get);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_environment_set);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_environment_delete);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_environment_enumerate);

	return environment;
}
void                               aprservice_lua_module_environment_deinit(aprservice_lua_module_environment* environment)
{
	delete environment;
}
