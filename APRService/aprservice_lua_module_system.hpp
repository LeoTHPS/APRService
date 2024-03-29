#pragma once
#include <AL/Common.hpp>

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

struct aprservice_lua;

void                                                                                      aprservice_lua_module_system_register_globals(aprservice_lua* lua);

APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM                                                     aprservice_lua_module_system_get_platform();
// @return year, month, day, hours, minutes, seconds
AL::Collections::Tuple<AL::uint16, AL::uint8, AL::uint8, AL::uint8, AL::uint8, AL::uint8> aprservice_lua_module_system_get_date_time();
AL::uint64                                                                                aprservice_lua_module_system_get_timestamp();
