require('APRService');
require('APRService.Modules.ByteBuffer');

APRService.Modules.UART = {};

APRService.Modules.UART.DEVICE_FLAG_NONE            = APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_NONE;
APRService.Modules.UART.DEVICE_FLAG_PARITY          = APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_PARITY;
APRService.Modules.UART.DEVICE_FLAG_PARITY_ODD      = APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_PARITY_ODD;
APRService.Modules.UART.DEVICE_FLAG_PARITY_EVEN     = APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_PARITY_EVEN;
APRService.Modules.UART.DEVICE_FLAG_USE_2_STOP_BITS = APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_USE_2_STOP_BITS;

-- @return device
function APRService.Modules.UART.OpenDevice(path, speed, flags)
	return aprservice_lua_module_uart_open_device(tostring(path), tonumber(speed), flags);
end

function APRService.Modules.UART.CloseDevice(device)
	aprservice_lua_module_uart_close_device(device);
end

APRService.Modules.UART.Device = {};

-- @return success, byte_buffer
function APRService.Modules.UART.Device.Read(device, byte_buffer_size, byte_buffer_endian)
	return aprservice_lua_module_uart_device_read(device, tonumber(byte_buffer_size), byte_buffer_endian);
end

function APRService.Modules.UART.Device.Write(device, byte_buffer, byte_buffer_size)
	return aprservice_lua_module_uart_device_write(device, byte_buffer, tonumber(byte_buffer_size));
end

-- @return success, would_block, byte_buffer
function APRService.Modules.UART.Device.TryRead(device, byte_buffer_size, byte_buffer_endian)
	return aprservice_lua_module_uart_device_try_read(device, tonumber(byte_buffer_size), byte_buffer_endian);
end
