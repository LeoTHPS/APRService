#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_file
{
};

aprservice_lua_module_file* aprservice_lua_module_file_init(aprservice_lua* lua)
{
	auto file = new aprservice_lua_module_file
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return file;
}
void                        aprservice_lua_module_file_deinit(aprservice_lua_module_file* file)
{
	delete file;
}
