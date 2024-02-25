#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_byte_buffer.hpp"

#include <AL/Lua54/Lua.hpp>

#if defined(AL_PLATFORM_LINUX)
	#define APRSERVICE_SPI_SUPPORTED

	#include <AL/Hardware/SPI.hpp>
#endif

struct aprservice_lua_module_spi
{
};

#if defined(APRSERVICE_SPI_SUPPORTED)
	typedef AL::Hardware::SPIDevice                                             aprservice_lua_module_spi_device;

	typedef typename AL::Get_Enum_Or_Integer_Base<AL::Hardware::SPIModes>::Type APRSERVICE_LUA_MODULE_SPI_MODE;
#else
	typedef void*                                                               aprservice_lua_module_spi_device;

	typedef AL::uint8                                                           APRSERVICE_LUA_MODULE_SPI_MODE;
#endif

enum APRSERVICE_LUA_MODULE_SPI_MODES : APRSERVICE_LUA_MODULE_SPI_MODE
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	APRSERVICE_LUA_MODULE_SPI_MODE_ZERO  = static_cast<APRSERVICE_LUA_MODULE_SPI_MODE>(AL::Hardware::SPIModes::Zero),
	APRSERVICE_LUA_MODULE_SPI_MODE_ONE   = static_cast<APRSERVICE_LUA_MODULE_SPI_MODE>(AL::Hardware::SPIModes::One),
	APRSERVICE_LUA_MODULE_SPI_MODE_TWO   = static_cast<APRSERVICE_LUA_MODULE_SPI_MODE>(AL::Hardware::SPIModes::Two),
	APRSERVICE_LUA_MODULE_SPI_MODE_THREE = static_cast<APRSERVICE_LUA_MODULE_SPI_MODE>(AL::Hardware::SPIModes::Three)
#else
	APRSERVICE_LUA_MODULE_SPI_MODE_ZERO  = 0x1,
	APRSERVICE_LUA_MODULE_SPI_MODE_ONE   = 0x2,
	APRSERVICE_LUA_MODULE_SPI_MODE_TWO   = 0x4,
	APRSERVICE_LUA_MODULE_SPI_MODE_THREE = 0x8,
#endif
};

aprservice_lua_module_spi_device* aprservice_lua_module_spi_device_open(const AL::String& path, APRSERVICE_LUA_MODULE_SPI_MODE mode, AL::uint32 speed, AL::uint8 bit_count)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	auto spi_device = new aprservice_lua_module_spi_device(AL::FileSystem::Path(path), static_cast<AL::Hardware::SPIModes>(mode), speed, bit_count);

	try
	{
		spi_device->Open();
	}
	catch (const AL::Exception& exception)
	{
		delete spi_device;

		aprservice_console_write_line("Error opening AL::Hardware::SPIDevice");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return spi_device;
#else
	aprservice_console_write_line("Error opening AL::Hardware::SPIDevice");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return nullptr;
#endif
}
void                              aprservice_lua_module_spi_device_close(aprservice_lua_module_spi_device* spi_device)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	spi_device->Close();

	delete spi_device;
#endif
}
APRSERVICE_LUA_MODULE_SPI_MODE    aprservice_lua_module_spi_device_get_mode(aprservice_lua_module_spi_device* spi_device)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	return static_cast<APRSERVICE_LUA_MODULE_SPI_MODE>(spi_device->GetMode());
#else
	aprservice_console_write_line("Error getting AL::Hardware::SPIDevice mode");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return APRSERVICE_LUA_MODULE_SPI_MODE_ZERO;
#endif
}
AL::uint32                        aprservice_lua_module_spi_device_get_speed(aprservice_lua_module_spi_device* spi_device)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	return spi_device->GetSpeed();
#else
	aprservice_console_write_line("Error getting AL::Hardware::SPIDevice speed");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return 0;
#endif
}
AL::uint8                         aprservice_lua_module_spi_device_get_bit_count(aprservice_lua_module_spi_device* spi_device)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	return spi_device->GetBitCount();
#else
	aprservice_console_write_line("Error getting AL::Hardware::SPIDevice bit count");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return 0;
#endif
}
// @return success, byte_buffer
auto                              aprservice_lua_module_spi_device_read(aprservice_lua_module_spi_device* spi_device, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian, bool change_cs)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer_instance*> value(false, aprservice_lua_module_byte_buffer_create(byte_buffer_endian, byte_buffer_size));

#if defined(APRSERVICE_SPI_SUPPORTED)
	try
	{
		spi_device->Read(aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>()), byte_buffer_size, change_cs);
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
bool                              aprservice_lua_module_spi_device_write(aprservice_lua_module_spi_device* spi_device, aprservice_lua_module_byte_buffer_instance* byte_buffer, AL::size_t byte_buffer_size, bool change_cs)
{
#if defined(APRSERVICE_SPI_SUPPORTED)
	try
	{
		spi_device->Write(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer), byte_buffer_size, change_cs);
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
auto                              aprservice_lua_module_spi_device_write_read(aprservice_lua_module_spi_device* spi_device, aprservice_lua_module_byte_buffer_instance* byte_buffer, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN rx_byte_buffer_endian, bool change_cs)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer_instance*> value(false, aprservice_lua_module_byte_buffer_create(rx_byte_buffer_endian, byte_buffer_size));

#if defined(APRSERVICE_SPI_SUPPORTED)
	try
	{
		spi_device->WriteRead(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer), aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>()), byte_buffer_size, change_cs);
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

aprservice_lua_module_spi* aprservice_lua_module_spi_init(aprservice_lua* lua)
{
	auto spi = new aprservice_lua_module_spi
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SPI_MODE_ZERO);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SPI_MODE_ONE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SPI_MODE_TWO);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SPI_MODE_THREE);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_device_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_device_close);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_device_get_mode);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_device_get_speed);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_device_get_bit_count);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_device_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_device_write);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_spi_device_write_read);

	return spi;
}
void                       aprservice_lua_module_spi_deinit(aprservice_lua_module_spi* spi)
{
	delete spi;
}
