#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_irc
{
};

aprservice_lua_module_irc* aprservice_lua_module_irc_init(aprservice_lua* lua)
{
	auto irc = new aprservice_lua_module_irc
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return irc;
}
void                       aprservice_lua_module_irc_deinit(aprservice_lua_module_irc* irc)
{
	delete irc;
}
