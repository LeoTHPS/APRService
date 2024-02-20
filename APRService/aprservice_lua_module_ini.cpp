#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_ini
{
};

aprservice_lua_module_ini* aprservice_lua_module_ini_init(aprservice_lua* lua)
{
	auto ini = new aprservice_lua_module_ini
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return ini;
}
void                       aprservice_lua_module_ini_deinit(aprservice_lua_module_ini* ini)
{
	delete ini;
}
