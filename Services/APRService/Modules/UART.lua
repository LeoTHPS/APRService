require('APRService');
require('APRService.Modules.ByteBuffer');

APRService.Modules.UART = {};

APRService.Modules.UART.FLAG_NONE            = APRSERVICE_LUA_MODULE_UART_FLAG_NONE;
APRService.Modules.UART.FLAG_PARITY          = APRSERVICE_LUA_MODULE_UART_FLAG_PARITY;
APRService.Modules.UART.FLAG_PARITY_ODD      = APRSERVICE_LUA_MODULE_UART_FLAG_PARITY_ODD;
APRService.Modules.UART.FLAG_PARITY_EVEN     = APRSERVICE_LUA_MODULE_UART_FLAG_PARITY_EVEN;
APRService.Modules.UART.FLAG_USE_2_STOP_BITS = APRSERVICE_LUA_MODULE_UART_FLAG_USE_2_STOP_BITS;

-- @return uart
function APRService.Modules.UART.Open(path, speed, flags)
	return aprservice_lua_module_uart_open(tostring(path), tonumber(speed), flags);
end

function APRService.Modules.UART.Close(uart)
	aprservice_lua_module_uart_close(uart);
end

-- @return success, byte_buffer
function APRService.Modules.UART.Read(uart, byte_buffer_size, byte_buffer_endian)
	return aprservice_lua_module_uart_read(uart, tonumber(byte_buffer_size), byte_buffer_endian);
end

function APRService.Modules.UART.Write(uart, byte_buffer, byte_buffer_size)
	return aprservice_lua_module_uart_write(uart, byte_buffer, tonumber(byte_buffer_size));
end

-- @return success, would_block, byte_buffer
function APRService.Modules.UART.TryRead(uart, byte_buffer_size, byte_buffer_endian)
	return aprservice_lua_module_uart_try_read(uart, tonumber(byte_buffer_size), byte_buffer_endian);
end
