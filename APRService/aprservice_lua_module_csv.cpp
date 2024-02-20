#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_csv
{
};

aprservice_lua_module_csv* aprservice_lua_module_csv_init(aprservice_lua* lua)
{
	auto csv = new aprservice_lua_module_csv
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return csv;
}
void                       aprservice_lua_module_csv_deinit(aprservice_lua_module_csv* csv)
{
	delete csv;
}
