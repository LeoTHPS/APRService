#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_i2c.hpp"

#include <AL/Lua54/Lua.hpp>

#if defined(AL_PLATFORM_LINUX)
	#define APRSERVICE_I2C_SUPPORTED

	#include <AL/Hardware/I2C.hpp>
#endif

struct aprservice_lua_module_i2c_bus
{
#if defined(APRSERVICE_I2C_SUPPORTED)
	AL::Hardware::I2CBus bus;
#endif
};

struct aprservice_lua_module_i2c_device
{
#if defined(APRSERVICE_I2C_SUPPORTED)
	AL::Hardware::I2CDevice device;
#endif
};

void                                                             aprservice_lua_module_i2c_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_bus_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_bus_close);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_bus_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_bus_write);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_bus_write_read);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_device_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_device_close);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_device_get_address);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_device_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_device_write);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_i2c_device_write_read);
}

aprservice_lua_module_i2c_bus*                                   aprservice_lua_module_i2c_bus_open(const AL::String& path, AL::uint32 baud)
{
#if defined(APRSERVICE_I2C_SUPPORTED)
	auto i2c_bus = new aprservice_lua_module_i2c_bus
	{
	#if defined(AL_PLATFORM_LINUX)
		.bus = AL::Hardware::I2CBus(AL::FileSystem::Path(path), baud)
	#else
		#error Platform not implemented
	#endif
	};

	try
	{
		i2c_bus->bus.Open();
	}
	catch (const AL::Exception& exception)
	{
		delete i2c_bus;

		aprservice_console_write_line("Error opening AL::Hardware::I2CBus");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return i2c_bus;
#else
	aprservice_console_write_line("Error opening AL::Hardware::I2CBus");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return nullptr;
#endif
}
void                                                             aprservice_lua_module_i2c_bus_close(aprservice_lua_module_i2c_bus* i2c_bus)
{
#if defined(APRSERVICE_I2C_SUPPORTED)
	i2c_bus->bus.Close();
#endif

	delete i2c_bus;
}
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_i2c_bus_read(aprservice_lua_module_i2c_bus* i2c_bus, AL::uint16 address, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> value(false, aprservice_lua_module_byte_buffer_create(byte_buffer_endian, byte_buffer_size));

#if defined(APRSERVICE_I2C_SUPPORTED)
	try
	{
		i2c_bus->bus.Read(address, const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>())), byte_buffer_size);
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_lua_module_byte_buffer_destroy(value.Get<1>());

		aprservice_console_write_line("Error reading AL::Hardware::I2CBus");
		aprservice_console_write_exception(exception);
	}
#else
	aprservice_console_write_line("Error reading AL::Hardware::I2CBus");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());
#endif

	return value;
}
bool                                                             aprservice_lua_module_i2c_bus_write(aprservice_lua_module_i2c_bus* i2c_bus, AL::uint16 address, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size)
{
#if defined(APRSERVICE_I2C_SUPPORTED)
	try
	{
		i2c_bus->bus.Write(address, aprservice_lua_module_byte_buffer_get_buffer(byte_buffer), byte_buffer_size);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error writing AL::Hardware::I2CBus");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
#else
	aprservice_console_write_line("Error writing AL::Hardware::I2CBus");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return false;
#endif
}
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_i2c_bus_write_read(aprservice_lua_module_i2c_bus* i2c_bus, AL::uint16 address, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, AL::size_t rx_byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN rx_byte_buffer_endian)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> value(false, aprservice_lua_module_byte_buffer_create(rx_byte_buffer_endian, byte_buffer_size));

#if defined(APRSERVICE_I2C_SUPPORTED)
	try
	{
		i2c_bus->bus.WriteRead(address, aprservice_lua_module_byte_buffer_get_buffer(byte_buffer), byte_buffer_size, const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>())), rx_byte_buffer_size);
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_lua_module_byte_buffer_destroy(value.Get<1>());

		aprservice_console_write_line("Error writing AL::Hardware::I2CBus");
		aprservice_console_write_exception(exception);
	}
#else
	aprservice_console_write_line("Error writing AL::Hardware::I2CBus");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());
#endif

	return value;
}

aprservice_lua_module_i2c_device*                                aprservice_lua_module_i2c_device_open(aprservice_lua_module_i2c_bus* bus, AL::uint16 address)
{
#if defined(APRSERVICE_I2C_SUPPORTED)
	auto i2c_device = new aprservice_lua_module_i2c_device
	{
	#if defined(AL_PLATFORM_LINUX)
		.device = AL::Hardware::I2CDevice(bus->bus, address)
	#else
		#error Platform not implemented
	#endif
	};

	return i2c_device;
#else
	aprservice_console_write_line("Error opening AL::Hardware::I2CDevice");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return nullptr;
#endif
}
void                                                             aprservice_lua_module_i2c_device_close(aprservice_lua_module_i2c_device* i2c_device)
{
	delete i2c_device;
}
AL::uint16                                                       aprservice_lua_module_i2c_device_get_address(aprservice_lua_module_i2c_device* i2c_device)
{
#if defined(APRSERVICE_I2C_SUPPORTED)
	return i2c_device->device.GetAddress();
#else
	return 0;
#endif
}
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_i2c_device_read(aprservice_lua_module_i2c_device* i2c_device, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> value(false, aprservice_lua_module_byte_buffer_create(byte_buffer_endian, byte_buffer_size));

#if defined(APRSERVICE_I2C_SUPPORTED)
	try
	{
		i2c_device->device.Read(const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>())), byte_buffer_size);
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_lua_module_byte_buffer_destroy(value.Get<1>());

		aprservice_console_write_line("Error reading AL::Hardware::I2CDevice");
		aprservice_console_write_exception(exception);
	}
#else
	aprservice_console_write_line("Error reading AL::Hardware::I2CDevice");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());
#endif

	return value;
}
bool                                                             aprservice_lua_module_i2c_device_write(aprservice_lua_module_i2c_device* i2c_device, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size)
{
#if defined(APRSERVICE_I2C_SUPPORTED)
	try
	{
		i2c_device->device.Write(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer), byte_buffer_size);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error writing AL::Hardware::I2CDevice");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
#else
	aprservice_console_write_line("Error writing AL::Hardware::I2CDevice");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());

	return false;
#endif
}
// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_i2c_device_write_read(aprservice_lua_module_i2c_device* i2c_device, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size, AL::size_t rx_byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN rx_byte_buffer_endian)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> value(false, aprservice_lua_module_byte_buffer_create(rx_byte_buffer_endian, byte_buffer_size));

#if defined(APRSERVICE_I2C_SUPPORTED)
	try
	{
		i2c_device->device.WriteRead(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer), byte_buffer_size, const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>())), rx_byte_buffer_size);
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_lua_module_byte_buffer_destroy(value.Get<1>());

		aprservice_console_write_line("Error writing AL::Hardware::I2CDevice");
		aprservice_console_write_exception(exception);
	}
#else
	aprservice_console_write_line("Error writing AL::Hardware::I2CDevice");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());
#endif

	return value;
}
