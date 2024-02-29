#pragma once
#include <AL/Common.hpp>

#include <AL/Collections/Tuple.hpp>

enum APRSERVICE_LUA_MODULE_GPIO_PIN_EDGES : AL::uint8
{
	APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_BOTH,
	APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_RISING,
	APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_FALLING
};

enum APRSERVICE_LUA_MODULE_GPIO_PIN_VALUES : AL::uint8
{
	APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW,
	APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_HIGH
};

enum APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTIONS : AL::uint8
{
	APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN,
	APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_OUT
};

struct aprservice_lua;
struct aprservice_lua_module_gpio;

void                                    aprservice_lua_module_gpio_register_globals(aprservice_lua* lua);

aprservice_lua_module_gpio*             aprservice_lua_module_gpio_open(AL::uint8 bus, AL::uint8 pin, AL::uint8 direction, AL::uint8 value);
void                                    aprservice_lua_module_gpio_close(aprservice_lua_module_gpio* gpio);

// @return success, value
AL::Collections::Tuple<bool, AL::uint8> aprservice_lua_module_gpio_pin_read(aprservice_lua_module_gpio* gpio);
bool                                    aprservice_lua_module_gpio_pin_write(aprservice_lua_module_gpio* gpio, AL::uint8 value);

bool                                    aprservice_lua_module_gpio_pin_set_pull_up(aprservice_lua_module_gpio* gpio);
bool                                    aprservice_lua_module_gpio_pin_set_pull_down(aprservice_lua_module_gpio* gpio);

AL::uint8                               aprservice_lua_module_gpio_pin_get_direction(aprservice_lua_module_gpio* gpio);
bool                                    aprservice_lua_module_gpio_pin_set_direction(aprservice_lua_module_gpio* gpio, AL::uint8 direction, AL::uint8 value);

// @return success, timeout
AL::Collections::Tuple<bool, bool>      aprservice_lua_module_gpio_pin_wait_for_edge(aprservice_lua_module_gpio* gpio, AL::uint8 edge, AL::uint32 max_wait_time_ms);
