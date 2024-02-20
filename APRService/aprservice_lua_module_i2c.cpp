#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

#if defined(AL_PLATFORM_LINUX)
	#define APRSERVICE_I2C_SUPPORTED

	#include <AL/Hardware/I2C.hpp>
#endif

struct aprservice_lua_module_i2c
{
};

aprservice_lua_module_i2c* aprservice_lua_module_i2c_init(aprservice_lua* lua)
{
	auto i2c = new aprservice_lua_module_i2c
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return i2c;
}
void                       aprservice_lua_module_i2c_deinit(aprservice_lua_module_i2c* i2c)
{
	delete i2c;
}
