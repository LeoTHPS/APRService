#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

struct aprservice_lua_module_byte_buffer
{
};

aprservice_lua_module_byte_buffer* aprservice_lua_module_byte_buffer_init(aprservice_lua* lua)
{
	auto byte_buffer = new aprservice_lua_module_byte_buffer
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return byte_buffer;
}
void                               aprservice_lua_module_byte_buffer_deinit(aprservice_lua_module_byte_buffer* byte_buffer)
{
	delete byte_buffer;
}
