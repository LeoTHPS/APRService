require('APRService');

APRService.Modules.System = {};

APRService.Modules.System.PLATFORM_DEBUG   = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_DEBUG
APRService.Modules.System.PLATFORM_GNU     = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_GNU
APRService.Modules.System.PLATFORM_MSVC    = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_MSVC
APRService.Modules.System.PLATFORM_CLANG   = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_CLANG
APRService.Modules.System.PLATFORM_ARM     = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_ARM
APRService.Modules.System.PLATFORM_ARM64   = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_ARM64
APRService.Modules.System.PLATFORM_X86     = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_X86
APRService.Modules.System.PLATFORM_X86_64  = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_X86_64
APRService.Modules.System.PLATFORM_PICO    = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_PICO
APRService.Modules.System.PLATFORM_PICO_W  = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_PICO_W
APRService.Modules.System.PLATFORM_RP2040  = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_RP2040
APRService.Modules.System.PLATFORM_LINUX   = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_LINUX
APRService.Modules.System.PLATFORM_MINGW   = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_MINGW
APRService.Modules.System.PLATFORM_WINDOWS = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_WINDOWS
APRService.Modules.System.PLATFORM_MACHINE = APRSERVICE_LUA_MODULE_SYSTEM_PLATFORM_MACHINE

function APRService.Modules.System.GetPlatform()
	return aprservice_lua_module_system_get_platform();
end

-- @return year, month, day, hours, minutes, seconds
function APRService.Modules.System.GetDateTime()
	return aprservice_lua_module_system_get_date_time();
end

function APRService.Modules.System.GetTimestamp()
	return aprservice_lua_module_system_get_timestamp();
end
