#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

#if defined(AL_PLATFORM_LINUX)
	#define APRSERVICE_SPI_SUPPORTED

	#include <AL/Hardware/SPI.hpp>
#endif

struct aprservice_lua_module_spi
{
};

aprservice_lua_module_spi* aprservice_lua_module_spi_init(aprservice_lua* lua)
{
	auto spi = new aprservice_lua_module_spi
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	return spi;
}
void                       aprservice_lua_module_spi_deinit(aprservice_lua_module_spi* spi)
{
	delete spi;
}
