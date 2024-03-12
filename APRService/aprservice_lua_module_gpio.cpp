#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_gpio.hpp"

#include <AL/Lua54/Lua.hpp>

#if defined(AL_PLATFORM_LINUX)
	#define APRSERVICE_GPIO_SUPPORTED

	#include <AL/Hardware/GPIO.hpp>
#endif

struct aprservice_lua_module_gpio
{
#if defined(APRSERVICE_GPIO_SUPPORTED)
	AL::Hardware::GPIO gpio;
#endif
};

void                                    aprservice_lua_module_gpio_register_globals(aprservice_lua* lua)
{
	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_BOTH);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_RISING);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_FALLING);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_HIGH);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_OUT);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_close);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_read);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_write);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_wait_for_edge);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_set_pull_up);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_set_pull_down);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_get_direction);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_gpio_pin_set_direction);
}

aprservice_lua_module_gpio*             aprservice_lua_module_gpio_open(AL::uint8 bus, AL::uint8 pin, AL::uint8 direction, AL::uint8 value)
{
#if defined(APRSERVICE_GPIO_SUPPORTED)
	#if defined(AL_PLATFORM_LINUX)
		auto gpio = new aprservice_lua_module_gpio
		{
			.gpio = AL::Hardware::GPIO(bus, pin, (direction == APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN) ? AL::Hardware::GPIOPinDirections::In : AL::Hardware::GPIOPinDirections::Out, (value == APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW) ? AL::Hardware::GPIOPinValues::Low : AL::Hardware::GPIOPinValues::High)
		};

		try
		{
			gpio->gpio.Open();
		}
		catch (const AL::Exception& exception)
		{
			delete gpio;

			aprservice_console_write_line("Error opening AL::Hardware::GPIO");
			aprservice_console_write_exception(exception);

			return nullptr;
		}

		return gpio;
	#else
		#error Platform not implemented
	#endif
#else
	aprservice_console_write_line("Error opening AL::Hardware::GPIO");
	aprservice_console_write_exception(AL::PlatformNotSupportedException());
#endif

	return nullptr;
}
void                                    aprservice_lua_module_gpio_close(aprservice_lua_module_gpio* gpio)
{
#if defined(APRSERVICE_GPIO_SUPPORTED)
	gpio->gpio.Close();

	delete gpio;
#endif
}

// @return success, value
AL::Collections::Tuple<bool, AL::uint8> aprservice_lua_module_gpio_pin_read(aprservice_lua_module_gpio* gpio)
{
	AL::Collections::Tuple<bool, AL::uint8> value(false, APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW);

	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		AL::Hardware::GPIOPinValues _value;

		gpio->gpio.Read(_value);

		if (_value == AL::Hardware::GPIOPinValues::High)
			value.Set<1>(APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_HIGH);

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
bool                                    aprservice_lua_module_gpio_pin_write(aprservice_lua_module_gpio* gpio, AL::uint8 value)
{
	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		gpio->gpio.Write((value == APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW) ? AL::Hardware::GPIOPinValues::Low : AL::Hardware::GPIOPinValues::High);
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

bool                                    aprservice_lua_module_gpio_pin_set_pull_up(aprservice_lua_module_gpio* gpio)
{
	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		gpio->gpio.SetPullUp();
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
bool                                    aprservice_lua_module_gpio_pin_set_pull_down(aprservice_lua_module_gpio* gpio)
{
	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		gpio->gpio.SetPullDown();
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

AL::uint8                               aprservice_lua_module_gpio_pin_get_direction(aprservice_lua_module_gpio* gpio)
{
	AL::uint8 value = APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN;

	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		if (gpio->gpio.GetDirection() == AL::Hardware::GPIOPinDirections::Out)
			value = APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_OUT;
#endif
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line("Error getting AL::Hardware::GPIO direction");
		aprservice_console_write_exception(exception);
	}

	return value;
}
bool                                    aprservice_lua_module_gpio_pin_set_direction(aprservice_lua_module_gpio* gpio, AL::uint8 direction, AL::uint8 value)
{
	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		gpio->gpio.SetDirection(
			(value == APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN) ? AL::Hardware::GPIOPinDirections::In : AL::Hardware::GPIOPinDirections::Out,
			(value == APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW) ? AL::Hardware::GPIOPinValues::Low : AL::Hardware::GPIOPinValues::High
		);
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

// @return success, timeout
AL::Collections::Tuple<bool, bool>      aprservice_lua_module_gpio_pin_wait_for_edge(aprservice_lua_module_gpio* gpio, AL::uint8 edge, AL::uint32 max_wait_time_ms)
{
	AL::Collections::Tuple<bool, bool> value(false, false);

	try
	{
#if defined(APRSERVICE_GPIO_SUPPORTED)
		switch (edge)
		{
			case APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_BOTH:
				value.Set<1>(gpio->gpio.WaitForEdge(AL::Hardware::GPIOPinEdges::Both, AL::TimeSpan::FromMilliseconds(max_wait_time_ms)));
				break;

			case APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_RISING:
				value.Set<1>(gpio->gpio.WaitForEdge(AL::Hardware::GPIOPinEdges::Rising, AL::TimeSpan::FromMilliseconds(max_wait_time_ms)));
				break;

			case APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_FALLING:
				value.Set<1>(gpio->gpio.WaitForEdge(AL::Hardware::GPIOPinEdges::Falling, AL::TimeSpan::FromMilliseconds(max_wait_time_ms)));
				break;
		}

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
