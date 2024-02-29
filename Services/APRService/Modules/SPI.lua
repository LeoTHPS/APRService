require('APRService');
require('APRService.Modules.ByteBuffer');

APRService.Modules.SPI = {};

APRService.Modules.SPI.MODE_ZERO  = APRSERVICE_LUA_MODULE_SPI_MODE_ZERO;
APRService.Modules.SPI.MODE_ONE   = APRSERVICE_LUA_MODULE_SPI_MODE_ONE;
APRService.Modules.SPI.MODE_TWO   = APRSERVICE_LUA_MODULE_SPI_MODE_TWO;
APRService.Modules.SPI.MODE_THREE = APRSERVICE_LUA_MODULE_SPI_MODE_THREE;

-- @return spi
function APRService.Modules.SPI.Open(path, mode, speed, bit_count)
	return aprservice_lua_module_spi_open(tostring(path), mode, tonumber(speed), tonumber(bit_count));
end

function APRService.Modules.SPI.Close(spi)
	aprservice_lua_module_spi_close(spi);
end

function APRService.Modules.SPI.GetMode(spi)
	return aprservice_lua_module_spi_get_mode(spi);
end

function APRService.Modules.SPI.GetSpeed(spi)
	return aprservice_lua_module_spi_get_speed(spi);
end

function APRService.Modules.SPI.GetBitCount(spi)
	return aprservice_lua_module_spi_get_bit_count(spi);
end

-- @return success, byte_buffer
function APRService.Modules.SPI.Read(spi, byte_buffer_size, byte_buffer_endian, change_cs)
	if change_cs == nil then
		change_cs = true;
	else
		change_cs = change_cs and true or false;
	end

	return aprservice_lua_module_spi_read(spi, tonumber(byte_buffer_size), byte_buffer_endian, change_cs);
end

function APRService.Modules.SPI.Write(spi, byte_buffer, byte_buffer_size, change_cs)
	if change_cs == nil then
		change_cs = true;
	else
		change_cs = change_cs and true or false;
	end

	return aprservice_lua_module_spi_write(spi, byte_buffer, tonumber(byte_buffer_size), change_cs);
end

-- @return success, byte_buffer
function APRService.Modules.SPI.WriteRead(spi, byte_buffer, byte_buffer_size, rx_byte_buffer_endian, change_cs)
	if change_cs == nil then
		change_cs = true;
	else
		change_cs = change_cs and true or false;
	end

	return aprservice_lua_module_spi_write_read(spi, byte_buffer, tonumber(byte_buffer_size), rx_byte_buffer_endian, change_cs);
end
