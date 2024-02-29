require('APRService');

APRService.Modules.GPIO = {};

APRService.Modules.GPIO.PIN_EDGE_BOTH     = APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_BOTH;
APRService.Modules.GPIO.PIN_EDGE_RISING   = APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_RISING;
APRService.Modules.GPIO.PIN_EDGE_FALLING  = APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_FALLING;

APRService.Modules.GPIO.PIN_VALUE_LOW     = APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW;
APRService.Modules.GPIO.PIN_VALUE_HIGH    = APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_HIGH;

APRService.Modules.GPIO.PIN_DIRECTION_IN  = APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN;
APRService.Modules.GPIO.PIN_DIRECTION_OUT = APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_OUT;

-- @return gpio
function APRService.Modules.GPIO.Open(bus, pin, direction, value)
	return aprservice_lua_module_gpio_open(tonumber(bus), tonumber(pin), direction, value);
end

function APRService.Modules.GPIO.Close(gpio)
	aprservice_lua_module_gpio_close(gpio);
end

-- @return success, value
function APRService.Modules.GPIO.Read(gpio)
	return aprservice_lua_module_gpio_read(gpio);
end

function APRService.Modules.GPIO.Write(gpio, value)
	return aprservice_lua_module_gpio_write(gpio, value);
end

function APRService.Modules.GPIO.SetPullUp(gpio)
	return aprservice_lua_module_gpio_set_pull_up(gpio);
end

function APRService.Modules.GPIO.SetPullDown(gpio)
	return aprservice_lua_module_gpio_set_pull_down(gpio);
end

function APRService.Modules.GPIO.GetDirection(gpio)
	return aprservice_lua_module_gpio_get_direction(gpio);
end

function APRService.Modules.GPIO.SetDirection(gpio, direction, value)
	return aprservice_lua_module_gpio_set_direction(gpio, direction, value);
end

-- @return success, timeout
function APRService.Modules.GPIO.WaitForEdge(gpio, edge, max_wait_time_ms)
	return aprservice_lua_module_gpio_wait_for_edge(gpio, edge, tonumber(max_wait_time_ms));
end
