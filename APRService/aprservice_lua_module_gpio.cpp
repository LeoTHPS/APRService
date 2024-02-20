#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

#if defined(AL_PLATFORM_LINUX)
	#define APRSERVICE_GPIO_SUPPORTED

	#include <AL/Hardware/GPIO.hpp>
#endif

#if defined(APRSERVICE_GPIO_SUPPORTED)
	typedef AL::Hardware::GPIO                                                        aprservice_lua_module_gpio_pin;

	typedef typename AL::Get_Enum_Or_Integer_Base<AL::Hardware::GPIOPinEdges>::Type   APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE;
	typedef typename AL::Get_Enum_Or_Integer_Base<AL::Hardware::GPIOPinValues>::Type  APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE;
	typedef typename AL::Get_Enum_Or_Integer_Base<AL::Hardware::GPIOPinDirectionsType APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION;
#else
	typedef void*                                                                     aprservice_lua_module_gpio_pin;

	typedef AL::uint8                                                                 APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE;
	typedef AL::uint8                                                                 APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE;
	typedef AL::uint8                                                                 APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION;
#endif

enum APRSERVICE_LUA_MODULE_GPIO_PIN_EDGES : APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE
{
#if defined(APRSERVICE_GPIO_SUPPORTED)
	APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_BOTH    = static_cast<APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE>(AL::Hardware::GPIOPinEdges::Both),
	APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_RISING  = static_cast<APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE>(AL::Hardware::GPIOPinEdges::Rising),
	APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_FALLING = static_cast<APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE>(AL::Hardware::GPIOPinEdges::Falling)
#else
	APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_BOTH,
	APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_RISING,
	APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_FALLING
#endif
};

enum APRSERVICE_LUA_MODULE_GPIO_PIN_VALUES : APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE
{
#if defined(APRSERVICE_GPIO_SUPPORTED)
	APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW  = static_cast<APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE>(AL::Hardware::GPIOPinValues::Low),
	APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_HIGH = static_cast<APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE>(AL::Hardware::GPIOPinValues::High)
#else
	APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW,
	APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_HIGH
#endif
};

enum APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTIONS : APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION
{
#if defined(APRSERVICE_GPIO_SUPPORTED)
	APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN  = static_cast<APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION>(AL::Harware::GPIOPinDirections::In),
	APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_OUT = static_cast<APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION>(AL::Harware::GPIOPinDirections::Out)
#else
	APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN,
	APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_OUT
#endif
};

struct aprservice_lua_module_gpio
{
};

aprservice_lua_module_gpio_pin* aprservice_lua_module_gpio_open_pin(AL::uint8 bus, AL::uint8 pin, AL::uint8 direction, AL::uint8 value)
{
#if defined(APRSERVICE_GPIO_SUPPORTED)
	#if defined(AL_PLATFORM_LINUX)
		auto gpio_pin = new aprservice_lua_module_gpio_pin(bus, pin, static_cast<AL::Hardware::GPIOPinDirections>(direction), static_cast<AL::Hardware::GPIOPinValues>(value));

		try
		{
			gpio_pin->Open();
		}
		catch (const AL::Exception& exception)
		{
			delete gpio_pin;

			aprservice_console_write_line("Error opening AL::Hardware::GPIO");
			aprservice_console_write_exception(exception);

			return nullptr;
		}

		return gpio_pin;
	#else
		#error Platform not implemented
	#endif
#else
	aprservice_console_write_line("Error opening AL::Hardware::GPIO");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());
#endif

	return nullptr;
}
void                            aprservice_lua_module_gpio_close_pin(aprservice_lua_module_gpio_pin* gpio_pin)
{
#if defined(APRSERVICE_GPIO_SUPPORTED)
	gpio_pin->Close();

	delete gpio_pin;
#endif
}

// @return success, value
auto                            aprservice_lua_module_gpio_pin_read(aprservice_lua_module_gpio_pin* gpio_pin)
{
	AL::Collections::Tuple<bool, AL::uint8> value(false, APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW);

	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		gpio_pin->Read(reinterpret_cast<AL::Hardware::GPIOPinValues&>(value.Get<1>()));
		value.Set<0>(true);
#endif
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error reading AL::Hardware::GPIO");
		aprservice_console_write_exception(exception);
	}

	return value;
}
bool                            aprservice_lua_module_gpio_pin_write(aprservice_lua_module_gpio_pin* gpio_pin, AL::uint8 value)
{
	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		gpio_pin->Write(static_cast<AL::Harware::GPIOPinValues>(value));
#endif
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error writing AL::Hardware::GPIO");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
// @return success, timeout
auto                            aprservice_lua_module_gpio_pin_wait_for_edge(aprservice_lua_module_gpio_pin* gpio_pin, AL::uint8 edge, AL::uint32 max_wait_time_ms)
{
	AL::Collections::Tuple<bool, bool> value(false, false);

	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		value.Set<1>(gpio_pin->WaitForEdge(static_cast<AL::Hardware::GPIOPinEdges>(edge), AL::TimeSpan::FromMilliseconds(max_wait_time_ms)));
		value.Set<0>(true);
#endif
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error waiting for AL::Hardware::GPIO edge");
		aprservice_console_write_exception(exception);
	}

	return value;
}
bool                            aprservice_lua_module_gpio_pin_set_pull_up(aprservice_lua_module_gpio_pin* gpio_pin)
{
	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		gpio_pin->SetPullUp();
#endif
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error setting AL::Hardware::GPIO pull up");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
bool                            aprservice_lua_module_gpio_pin_set_pull_down(aprservice_lua_module_gpio_pin* gpio_pin)
{
	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		gpio_pin->SetPullDown();
#endif
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error setting AL::Hardware::GPIO pull down");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}
AL::uint8                       aprservice_lua_module_gpio_pin_get_direction(aprservice_lua_module_gpio_pin* gpio_pin)
{
	AL::uint8 value = APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN;

	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		value = gpio_pin->GetDirection();
#endif
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error getting AL::Hardware::GPIO direction");
		aprservice_console_write_exception(exception);
	}

	return value;
}
bool                            aprservice_lua_module_gpio_pin_set_direction(aprservice_lua_module_gpio_pin* gpio_pin, AL::uint8 direction, AL::uint8 value)
{
	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		gpio_pin->SetDirection(static_cast<AL::Hardware::GPIOPinDirections>(direction), static_cast<AL::Hardware::GPIOPinValues>(value));
#endif
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error setting AL::Hardware::GPIO direction");
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}

aprservice_lua_module_gpio* aprservice_lua_module_gpio_init(aprservice_lua* lua)
{
	auto gpio = new aprservice_lua_module_gpio
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_BOTH);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_RISING);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_FALLING);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_HIGH);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_OUT);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_open_pin);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_close_pin);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_write);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_wait_for_edge);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_set_pull_up);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_set_pull_down);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_get_direction);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_set_direction);

	return gpio;
}
void                        aprservice_lua_module_gpio_deinit(aprservice_lua_module_gpio* gpio)
{
	delete gpio;
}
