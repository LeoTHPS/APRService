#pragma once
#include <AL/Common.hpp>

#include "aprservice_lua_module_byte_buffer.hpp"

enum APRSERVICE_LUA_MODULE_UART_FLAGS : AL::uint8
{
	APRSERVICE_LUA_MODULE_UART_FLAG_NONE            = 0x01,
	APRSERVICE_LUA_MODULE_UART_FLAG_PARITY          = 0x02,
	APRSERVICE_LUA_MODULE_UART_FLAG_PARITY_ODD      = 0x04,
	APRSERVICE_LUA_MODULE_UART_FLAG_PARITY_EVEN     = 0x08,
	APRSERVICE_LUA_MODULE_UART_FLAG_USE_2_STOP_BITS = 0x10
};

struct aprservice_lua;
struct aprservice_lua_module_uart;

void aprservice_lua_module_uart_register_globals(aprservice_lua* lua);

aprservice_lua_module_uart*                                            aprservice_lua_module_uart_open(const AL::String& path, AL::uint32 speed, AL::uint8 flags);
void                                                                   aprservice_lua_module_uart_close(aprservice_lua_module_uart* uart);
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*>       aprservice_lua_module_uart_read(aprservice_lua_module_uart* uart, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian);
bool                                                                   aprservice_lua_module_uart_write(aprservice_lua_module_uart* uart, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size);
// @return success, would_block, byte_buffer
AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_uart_try_read(aprservice_lua_module_uart* uart, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian);
