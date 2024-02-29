#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_text_file.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Serialization/INI.hpp>

struct aprservice_lua_module_ini
{
};

void aprservice_lua_module_ini_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	// TODO: implement
}
