#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_json
{
};

aprservice_lua_module_json* aprservice_lua_module_json_init(aprservice_lua* lua)
{
	auto json = new aprservice_lua_module_json
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return json;
}
void                       aprservice_lua_module_json_deinit(aprservice_lua_module_json* json)
{
	delete json;
}
