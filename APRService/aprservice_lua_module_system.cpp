#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_system.hpp"

#include <AL/OS/System.hpp>

#include <AL/Lua54/Lua.hpp>

void                                                                                      aprservice_lua_module_system_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_DEBUG);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_GNU);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_MSVC);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_CLANG);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_ARM);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_ARM64);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_X86);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_X86_64);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_PICO);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_PICO_W);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_RP2040);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_LINUX);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_MINGW);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_WINDOWS);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_MACHINE);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_system_get_platform);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_system_get_date_time);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_system_get_timestamp);
}

APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM                                                     aprservice_lua_module_system_get_platform()
{
	return static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::Machine);
}
// @return year, month, day, hours, minutes, seconds
AL::Collections::Tuple<AL::uint16, AL::uint8, AL::uint8, AL::uint8, AL::uint8, AL::uint8> aprservice_lua_module_system_get_date_time()
{
	auto value = AL::OS::System::GetDateTime();

	return AL::Collections::Tuple<AL::uint16, AL::uint8, AL::uint8, AL::uint8, AL::uint8, AL::uint8>(static_cast<AL::uint16>(value.Year), static_cast<AL::uint8>(value.Month), static_cast<AL::uint8>(value.Day), static_cast<AL::uint8>(value.Hour), static_cast<AL::uint8>(value.Minutes), static_cast<AL::uint8>(value.Seconds));
}
AL::uint64                                                                                aprservice_lua_module_system_get_timestamp()
{
	return AL::OS::System::GetTimestamp().ToSeconds();
}
