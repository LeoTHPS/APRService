require('APRService');
require('APRService.Modules.ByteBuffer');

APRService.Modules.SPI = {};

APRService.Modules.SPI.MODE_ZERO  = APRSERVICE_LUA_MODULE_SPI_MODE_ZERO;
APRService.Modules.SPI.MODE_ONE   = APRSERVICE_LUA_MODULE_SPI_MODE_ONE;
APRService.Modules.SPI.MODE_TWO   = APRSERVICE_LUA_MODULE_SPI_MODE_TWO;
APRService.Modules.SPI.MODE_THREE = APRSERVICE_LUA_MODULE_SPI_MODE_THREE;

function APRService.Modules.SPI.OpenDevice(path, mode, speed, bit_count)
	return aprservice_lua_module_spi_device_open(tostring(path), mode, tonumber(speed), tonumber(bit_count));
end

function APRService.Modules.SPI.CloseDevice(spi_device)
	aprservice_lua_module_spi_device_close(spi_device);
end

APRService.Modules.SPI.Device = {};

function APRService.Modules.SPI.Device.GetMode(spi_device)
	return aprservice_lua_module_spi_device_get_mode(spi_device);
end

function APRService.Modules.SPI.Device.GetSpeed(spi_device)
	return aprservice_lua_module_spi_device_get_speed(spi_device);
end

function APRService.Modules.SPI.Device.GetBitCount(spi_device)
	return aprservice_lua_module_spi_device_get_bit_count(spi_device);
end

-- @return success, byte_buffer
function APRService.Modules.SPI.Device.Read(spi_device, byte_buffer_size, byte_buffer_endian, change_cs)
	if change_cs == nil then
		change_cs = true;
	else
		change_cs = change_cs and true or false;
	end

	return aprservice_lua_module_spi_device_read(spi_device, tonumber(byte_buffer_size), byte_buffer_endian, change_cs);
end

function APRService.Modules.SPI.Device.Write(spi_device, byte_buffer, byte_buffer_size, change_cs)
	if change_cs == nil then
		change_cs = true;
	else
		change_cs = change_cs and true or false;
	end

	return aprservice_lua_module_spi_device_write(spi_device, byte_buffer, tonumber(byte_buffer_size), change_cs);
end

-- @return success, byte_buffer
function APRService.Modules.SPI.Device.WriteRead(spi_device, byte_buffer, byte_buffer_size, rx_byte_buffer_endian, change_cs)
	if change_cs == nil then
		change_cs = true;
	else
		change_cs = change_cs and true or false;
	end

	return aprservice_lua_module_spi_device_write_read(spi_device, byte_buffer, tonumber(byte_buffer_size), rx_byte_buffer_endian, change_cs);
end
