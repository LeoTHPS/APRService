#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Hardware/UART.hpp>

typedef typename AL::Get_Enum_Or_Integer_Base<AL::Hardware::UARTDeviceFlags>::Type APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG;

enum APRSERVICE_LUA_MODULE_UART_DEVICE_FLAGS : APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG
{
	APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_NONE            = static_cast<APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG>(AL::Hardware::UARTDeviceFlags::None),
	APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_PARITY          = static_cast<APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG>(AL::Hardware::UARTDeviceFlags::Parity),
	APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_PARITY_ODD      = static_cast<APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG>(AL::Hardware::UARTDeviceFlags::Parity_Odd),
	APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_PARITY_EVEN     = static_cast<APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG>(AL::Hardware::UARTDeviceFlags::Parity_Even),
	APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_USE_2_STOP_BITS = static_cast<APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG>(AL::Hardware::UARTDeviceFlags::Use2StopBits)
};

struct aprservice_lua_module_uart
{
};

struct aprservice_lua_module_byte_buffer;

typedef AL::Hardware::UARTDevice aprservice_lua_module_uart_device;

aprservice_lua_module_uart_device* aprservice_lua_module_uart_open_device(const AL::String& path, AL::uint32 speed, AL::uint8 flags)
{
	auto uart_device = new aprservice_lua_module_uart_device(AL::FileSystem::Path(path), speed, static_cast<AL::Hardware::UARTDeviceFlags>(flags));

	try
	{
		uart_device->Open();
	}
	catch (const AL::Exception& exception)
	{
		delete uart_device;

		aprservice_console_write_line("Error opening AL::Hardware::UARTDevice");
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return uart_device;
}
void                               aprservice_lua_module_uart_close_device(aprservice_lua_module_uart_device* device)
{
	device->Close();

	delete device;
}

// @return success, byte_buffer
auto                               aprservice_lua_module_uart_device_read(aprservice_lua_module_uart_device* uart_device, AL::size_t buffer_size)
{
	AL::Collections::Tuple<bool, aprservice_lua_module_byte_buffer*> value(false, nullptr);

	// TODO: implement
	aprservice_console_write_exception(AL::NotImplementedException());

	return value;
}
bool                               aprservice_lua_module_uart_device_write(aprservice_lua_module_uart_device* uart_device, aprservice_lua_module_byte_buffer* buffer)
{
	// TODO: implement
	aprservice_console_write_exception(AL::NotImplementedException());

	return false;
}

// @return success, would_block, byte_buffer
auto                               aprservice_lua_module_uart_device_try_read(aprservice_lua_module_uart_device* uart_device, AL::size_t buffer_size)
{
	AL::Collections::Tuple<bool, bool, aprservice_lua_module_byte_buffer*> value(false, false, nullptr);

	// TODO: implement
	aprservice_console_write_exception(AL::NotImplementedException());

	return value;
}

aprservice_lua_module_uart* aprservice_lua_module_uart_init(aprservice_lua* lua)
{
	auto uart = new aprservice_lua_module_uart
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_NONE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_PARITY);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_PARITY_ODD);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_PARITY_EVEN);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_UART_DEVICE_FLAG_USE_2_STOP_BITS);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_open_device);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_close_device);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_device_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_device_write);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_uart_device_try_read);

	return uart;
}
void                        aprservice_lua_module_uart_deinit(aprservice_lua_module_uart* uart)
{
	delete uart;
}
