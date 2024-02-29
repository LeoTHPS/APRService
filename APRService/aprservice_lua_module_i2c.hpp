#pragma once
#include <AL/Common.hpp>

#include "aprservice_lua_module_byte_buffer.hpp"

struct aprservice_lua;
struct aprservice_lua_module_i2c_bus;
struct aprservice_lua_module_i2c_device;

void                                                             aprservice_lua_module_i2c_register_globals(aprservice_lua* lua);

aprservice_lua_module_i2c_bus*                                   aprservice_lua_module_i2c_bus_open(const AL::String& path, AL::uint32 baud);
void                                                             aprservice_lua_module_i2c_bus_close(aprservice_lua_module_i2c_bus* i2c_bus);
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_i2c_bus_read(aprservice_lua_module_i2c_bus* i2c_bus, AL::uint16 address, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian);
bool                                                             aprservice_lua_module_i2c_bus_write(aprservice_lua_module_i2c_bus* i2c_bus, AL::uint16 address, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size);
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_i2c_bus_write_read(aprservice_lua_module_i2c_bus* i2c_bus, AL::uint16 address, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, AL::size_t rx_byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN rx_byte_buffer_endian);
aprservice_lua_module_i2c_device*                                aprservice_lua_module_i2c_device_open(aprservice_lua_module_i2c_bus* bus, AL::uint16 address);
void                                                             aprservice_lua_module_i2c_device_close(aprservice_lua_module_i2c_device* i2c_device);
AL::uint16                                                       aprservice_lua_module_i2c_device_get_address(aprservice_lua_module_i2c_device* i2c_device);
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_i2c_device_read(aprservice_lua_module_i2c_device* i2c_device, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian);
bool                                                             aprservice_lua_module_i2c_device_write(aprservice_lua_module_i2c_device* i2c_device, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size);
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_i2c_device_write_read(aprservice_lua_module_i2c_device* i2c_device, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, AL::size_t rx_byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN rx_byte_buffer_endian);
