#pragma once
#include <AL/Common.hpp>

#include "aprservice_lua_module_byte_buffer.hpp"

enum APRSERVICE_LUA_MODULE_SPI_MODES : AL::uint8
{
	APRSERVICE_LUA_MODULE_SPI_MODE_ZERO,
	APRSERVICE_LUA_MODULE_SPI_MODE_ONE,
	APRSERVICE_LUA_MODULE_SPI_MODE_TWO,
	APRSERVICE_LUA_MODULE_SPI_MODE_THREE
};

struct aprservice_lua;
struct aprservice_lua_module_spi;

void                                                             aprservice_lua_module_spi_register_globals(aprservice_lua* lua);

aprservice_lua_module_spi*                                       aprservice_lua_module_spi_open(const AL::String& path, AL::uint8 mode, AL::uint32 speed, AL::uint8 bit_count);
void                                                             aprservice_lua_module_spi_close(aprservice_lua_module_spi* spi);

AL::uint8                                                        aprservice_lua_module_spi_get_mode(aprservice_lua_module_spi* spi);
AL::uint32                                                       aprservice_lua_module_spi_get_speed(aprservice_lua_module_spi* spi);
AL::uint8                                                        aprservice_lua_module_spi_get_bit_count(aprservice_lua_module_spi* spi);

// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_spi_read(aprservice_lua_module_spi* spi, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian, bool change_cs);
bool                                                             aprservice_lua_module_spi_write(aprservice_lua_module_spi* spi, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, bool change_cs);
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_spi_write_read(aprservice_lua_module_spi* spi, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN rx_byte_buffer_endian, bool change_cs);
