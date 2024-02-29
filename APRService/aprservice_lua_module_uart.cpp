#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_uart.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Hardware/UART.hpp>

struct aprservice_lua_module_uart
{
	AL::Hardware::UARTDevice device;
};

void                                                                   aprservice_lua_module_uart_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_FLAG_NONE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_FLAG_PARITY);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_FLAG_PARITY_ODD);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_FLAG_PARITY_EVEN);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_FLAG_USE_2_STOP_BITS);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_close);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_write);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_try_read);
}

aprservice_lua_module_uart*                                            aprservice_lua_module_uart_open(const AL::String& path, AL::uint32 speed, AL::uint8 flags)
{
	AL::BitMask<AL::Hardware::UARTDeviceFlags> uart_device_flags;
	uart_device_flags.Set(AL::Hardware::UARTDeviceFlags::Parity,       AL::BitMask<AL::uint8>::IsSet(flags, APRSERVICE_LUA_MODULE_UART_FLAG_PARITY));
	uart_device_flags.Set(AL::Hardware::UARTDeviceFlags::Parity_Odd,   AL::BitMask<AL::uint8>::IsSet(flags, APRSERVICE_LUA_MODULE_UART_FLAG_PARITY_ODD));
	uart_device_flags.Set(AL::Hardware::UARTDeviceFlags::Parity_Even,  AL::BitMask<AL::uint8>::IsSet(flags, APRSERVICE_LUA_MODULE_UART_FLAG_PARITY_EVEN));
	uart_device_flags.Set(AL::Hardware::UARTDeviceFlags::Use2StopBits, AL::BitMask<AL::uint8>::IsSet(flags, APRSERVICE_LUA_MODULE_UART_FLAG_USE_2_STOP_BITS));

	auto uart = new aprservice_lua_module_uart
	{
		.device = AL::Hardware::UARTDevice(AL::FileSystem::Path(path), speed, uart_device_flags.Value)
	};

	try
	{
		uart->device.Open();
	}
	catch (const AL::Exception& exception)
	{
		delete uart;

		aprservice_console_write_line("Error opening AL::Hardware::UARTDevice");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return uart;
}
void                                                                   aprservice_lua_module_uart_close(aprservice_lua_module_uart* uart)
{
	uart->device.Close();

	delete uart;
}

// @return success, byte_buffer
AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*>       aprservice_lua_module_uart_read(aprservice_lua_module_uart* uart, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> value(false, aprservice_lua_module_byte_buffer_create(byte_buffer_endian, byte_buffer_size));

	try
	{
		uart->device.Read(const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<1>())), byte_buffer_size);
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_lua_module_byte_buffer_destroy(value.Get<1>());

		aprservice_console_write_line("Error reading AL::Hardware::UARTDevice");
		aprservice_console_write_exception(exception);
	}

	return value;
}
bool                                                                   aprservice_lua_module_uart_write(aprservice_lua_module_uart* uart, aprservice_lua_module_byte_buffer* byte_buffer, AL::size_t byte_buffer_size)
{
	try
	{
		uart->device.Write(aprservice_lua_module_byte_buffer_get_buffer(byte_buffer), byte_buffer_size);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error reading AL::Hardware::UARTDevice");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}

// @return success, would_block, byte_buffer
AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*> aprservice_lua_module_uart_try_read(aprservice_lua_module_uart* uart, AL::size_t byte_buffer_size, APRSERVICE_LUA_MODULE_BYTE_BUFFER_ENDIAN byte_buffer_endian)
{
	AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*> value(false, false, aprservice_lua_module_byte_buffer_create(byte_buffer_endian, byte_buffer_size));

	try
	{
		value.Set<1>(!uart->device.TryRead(const_cast<void*>(aprservice_lua_module_byte_buffer_get_buffer(value.Get<2>())), byte_buffer_size));
		value.Set<0>(true);
	}
	catch (const AL::Exception& exception)
	{
		aprservice_lua_module_byte_buffer_destroy(value.Get<2>());

		aprservice_console_write_line("Error reading AL::Hardware::UARTDevice");
		aprservice_console_write_exception(exception);
	}

	return value;
}
