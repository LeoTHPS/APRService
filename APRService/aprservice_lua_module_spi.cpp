#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_spi.hpp"

#include <AL/Lua54/Lua.hpp>

#if defined(AL_PLATFORM_LINUX)
	#define APRSERVICE_SPI_SUPPORTED

	#include <AL/Hardware/SPI.hpp>
#endif

struct aprservice_lua_module_spi
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	AL::Hardware::SPIDevice device;
#endif
};

void                                                             aprservice_lua_module_spi_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SPI_MODE_ZERO);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SPI_MODE_ONE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SPI_MODE_TWO);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SPI_MODE_THREE);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_close);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_get_mode);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_get_speed);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_get_bit_count);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_write);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_write_read);
}

aprservice_lua_module_spi*                                       aprservice_lua_module_spi_open(const AL::String& path, AL::uint8 mode, AL::uint32 speed, AL::uint8 bit_count)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	AL::Hardware::SPIModes spi_device_mode;

	switch (mode)
	{
		case APRSERVICE_LUA_MODULE_SPI_MODE_ZERO:
			spi_device_mode = AL::Hardware::SPIModes::Zero;
			break;

		case APRSERVICE_LUA_MODULE_SPI_MODE_ONE:
			spi_device_mode = AL::Hardware::SPIModes::One;
			break;

		case APRSERVICE_LUA_MODULE_SPI_MODE_TWO:
			spi_device_mode = AL::Hardware::SPIModes::Two;
			break;

		case APRSERVICE_LUA_MODULE_SPI_MODE_THREE:
			spi_device_mode = AL::Hardware::SPIModes::Three;
			break;

		default:
			aprservice_console_write_line("Invalid SPI mode");
			return nullptr;
	}

	auto spi = new aprservice_lua_module_spi
	{
	#if defined(AL_PLATFORM_LINUX)
		.device = AL::Hardware::SPIDevice(AL::FileSystem::Path(path), spi_device_mode, speed, bit_count)
	#else
		#error Platform not implemented
	#endif
	};

	try
	{
		spi->device.Open();
	}
	catch (const AL::Exception& exception)
	{
		delete spi;

		aprservice_console_write_line("Error opening AL::Hardware::SPIDevice");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return spi;
#else
	aprservice_console_write_line("Error opening AL::Hardware::SPIDevice");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return nullptr;
#endif
}
void                                                             aprservice_lua_module_spi_close(aprservice_lua_module_spi* spi)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	spi->device.Close();

	delete spi;
#endif
}

AL::uint8                                                        aprservice_lua_module_spi_get_mode(aprservice_lua_module_spi* spi)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	switch (spi->device.GetMode())
	{
		case AL::Hardware::SPIModes::Zero:  return APRSERVICE_LUA_MODULE_SPI_MODE_ZERO;
		case AL::Hardware::SPIModes::One:   return APRSERVICE_LUA_MODULE_SPI_MODE_ONE;
		case AL::Hardware::SPIModes::Two:   return APRSERVICE_LUA_MODULE_SPI_MODE_TWO;
		case AL::Hardware::SPIModes::Three: return APRSERVICE_LUA_MODULE_SPI_MODE_THREE;
	}

	return 0xFF;
#else
	aprservice_console_write_line("Error getting AL::Hardware::SPIDevice mode");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return APRSERVICE_LUA_MODULE_SPI_MODE_ZERO;
#endif
}
AL::uint32                                                       aprservice_lua_module_spi_get_speed(aprservice_lua_module_spi* spi)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	return spi->device.GetSpeed();
#else
	aprservice_console_write_line("Error getting AL::Hardware::SPIDevice speed");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return 0;
#endif
}
AL::uint8                                                        aprservice_lua_module_spi_get_bit_count(aprservice_lua_module_spi* spi)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	return spi->device.GetBitCount();
#else
	aprservice_console_write_line("Error getting AL::Hardware::SPIDevice bit count");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return 0;
#endif
}

// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_spi_read(aprservice_lua_module_spi* spi, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian, bool change_cs)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> value(false, aprservice_lua_module_byte_buffer_create(byte_buffer_endian, byte_buffer_size));

#if defined(APRSERVICE_SPI_SUPPORTED)
	try
	{
		spi->device.Read(aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>()), byte_buffer_size, change_cs);
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_lua_module_byte_buffer_destroy(value.Get<1>());

		aprservice_console_write_line("Error reading AL::Hardware::SPIDevice");
		aprservice_console_write_exception(exception);
	}
#else
	aprservice_lua_module_byte_buffer_destroy(value.Get<1>());
	aprservice_console_write_line("Error reading AL::Hardware::SPIDevice");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());
#endif

	return value;
}
bool                                                             aprservice_lua_module_spi_write(aprservice_lua_module_spi* spi, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, bool change_cs)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	try
	{
		spi->device.Write(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer), byte_buffer_size, change_cs);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error writing AL::Hardware::SPIDevice");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
#else
	aprservice_console_write_line("Error writing AL::Hardware::SPIDevice");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return false;
#endif
}
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_spi_write_read(aprservice_lua_module_spi* spi, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN rx_byte_buffer_endian, bool change_cs)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> value(false, aprservice_lua_module_byte_buffer_create(rx_byte_buffer_endian, byte_buffer_size));

#if defined(APRSERVICE_SPI_SUPPORTED)
	try
	{
		spi->device.WriteRead(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer), aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>()), byte_buffer_size, change_cs);
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_lua_module_byte_buffer_destroy(value.Get<1>());

		aprservice_console_write_line("Error writing AL::Hardware::SPIDevice");
		aprservice_console_write_exception(exception);
	}
#else
	aprservice_lua_module_byte_buffer_destroy(value.Get<1>());
	aprservice_console_write_line("Error writing AL::Hardware::SPIDevice");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());
#endif

	return value;
}
