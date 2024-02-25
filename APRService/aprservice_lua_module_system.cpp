#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/OS/System.hpp>

#include <AL/Lua54/Lua.hpp>

typedef typename AL::Get_Enum_Or_Integer_Base<AL::Platforms>::Type APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM;

enum APRSERVICE_LUA_MODULE_SYSTEM_PLATFORMS : APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM
{
	// Debug

	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_DEBUG   = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::Debug),

	// Compiler

	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_GNU     = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::GNU),
	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_MSVC    = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::MSVC),
	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_CLANG   = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::Clang),

	// CPU Architecture

	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_ARM     = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::ARM),
	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_ARM64   = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::ARM64),

	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_X86     = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::x86),
	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_X86_64  = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::x86_64),

	// Operating System

	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_PICO    = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::Pico),
	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_PICO_W  = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::PicoW),
	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_RP2040  = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::RP2040),
	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_LINUX   = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::Linux),
	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_MINGW   = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::MinGW),
	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_WINDOWS = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::Windows),

	// Machine Constant

	APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_MACHINE = static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::Machine)
};

struct aprservice_lua_module_system
{
};

APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM aprservice_lua_module_system_get_platform()
{
	return static_cast<APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM>(AL::Platforms::Machine);
}
// @return year, month, day, hours, minutes, seconds
auto                                  aprservice_lua_module_system_get_date_time()
{
	auto value = AL::OS::System::GetDateTime();

	return AL::Collections::Tuple<AL::uint16, AL::uint8, AL::uint8, AL::uint8, AL::uint8, AL::uint8>(static_cast<AL::uint16>(value.Year), static_cast<AL::uint8>(value.Month), static_cast<AL::uint8>(value.Day), static_cast<AL::uint8>(value.Hour), static_cast<AL::uint8>(value.Minutes), static_cast<AL::uint8>(value.Seconds));
}
AL::uint64                            aprservice_lua_module_system_get_timestamp()
{
	return AL::OS::System::GetTimestamp().ToSeconds();
}

aprservice_lua_module_system* aprservice_lua_module_system_init(aprservice_lua* lua)
{
	auto system = new aprservice_lua_module_system
	{
	};

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

	return system;
}
void                          aprservice_lua_module_system_deinit(aprservice_lua_module_system* system)
{
	delete system;
}
