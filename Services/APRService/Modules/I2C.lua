require('APRService');
require('APRService.Modules.ByteBuffer');

APRService.Modules.I2C = {};

function APRService.Modules.I2C.OpenBus(path, baud)
	return aprservice_lua_module_i2c_bus_open(tostring(path), tonumber(baud));
end

function APRService.Modules.I2C.CloseBus(i2c_bus)
	aprservice_lua_module_i2c_bus_close(i2c_bus);
end

function APRService.Modules.I2C.OpenDevice(i2c_bus, address)
	return aprservice_lua_module_i2c_device_open(i2c_bus, tonumber(address));
end

function APRService.Modules.I2C.CloseDevice(i2c_device)
	aprservice_lua_module_i2c_device_close(i2c_device);
end

APRService.Modules.I2C.Bus = {};

-- @return success, byte_buffer
function APRService.Modules.I2C.Bus.Read(i2c_bus, address, byte_buffer_size, byte_buffer_endian)
	return aprservice_lua_module_i2c_bus_read(i2c_bus, tonumber(address), tonumber(byte_buffer_size), byte_buffer_endian);
end

function APRService.Modules.I2C.Bus.Write(i2c_bus, address, byte_buffer, byte_buffer_size)
	return aprservice_lua_module_i2c_bus_write(i2c_bus, tonumber(address), byte_buffer, tonumber(byte_buffer_size));
end

-- @return success, byte_buffer
function APRService.Modules.I2C.Bus.WriteRead(i2c_bus, address, byte_buffer, byte_buffer_size, rx_byte_buffer_size, rx_byte_buffer_endian)
	return aprservice_lua_module_i2c_bus_write_read(i2c_bus, tonumber(address), byte_buffer, tonumber(byte_buffer_size), tonumber(rx_byte_buffer_size), rx_byte_buffer_endian);
end

APRService.Modules.I2C.Device = {};

function APRService.Modules.I2C.Device.GetAddress(i2c_device)
	return aprservice_lua_module_i2c_device_get_address(i2c_device);
end

-- @return success, byte_buffer
function APRService.Modules.I2C.Device.Read(i2c_device, byte_buffer_size, byte_buffer_endian)
	return aprservice_lua_module_i2c_device_read(i2c_device, tonumber(byte_buffer_size), byte_buffer_endian);
end

function APRService.Modules.I2C.Device.Write(i2c_device, byte_buffer, byte_buffer_size)
	return aprservice_lua_module_i2c_device_write(i2c_device, byte_buffer, tonumber(byte_buffer_size));
end

-- @return success, byte_buffer
function APRService.Modules.I2C.Device.WriteRead(i2c_device, byte_buffer, byte_buffer_size, rx_byte_buffer_size, rx_byte_buffer_endian)
	return aprservice_lua_module_i2c_device_write_read(i2c_device, byte_buffer, tonumber(byte_buffer_size), tonumber(rx_byte_buffer_size), rx_byte_buffer_endian);
end
