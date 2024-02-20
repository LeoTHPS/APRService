#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_http
{
};

aprservice_lua_module_http* aprservice_lua_module_http_init(aprservice_lua* lua)
{
	auto http = new aprservice_lua_module_http
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return http;
}
void                        aprservice_lua_module_http_deinit(aprservice_lua_module_http* http)
{
	delete http;
}
