#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_csv.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Serialization/CSV.hpp>

struct aprservice_lua_module_csv
{
};

void aprservice_lua_module_csv_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	// TODO: implement
}
