require('APRService');

APRService.Modules.GPIO = {};

APRService.Modules.GPIO.PIN_EDGE_BOTH     = APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_BOTH;
APRService.Modules.GPIO.PIN_EDGE_RISING   = APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_RISING;
APRService.Modules.GPIO.PIN_EDGE_FALLING  = APRSERVICE_LUA_MODULE_GPIO_PIN_EDGE_FALLING;

APRService.Modules.GPIO.PIN_VALUE_LOW     = APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_LOW;
APRService.Modules.GPIO.PIN_VALUE_HIGH    = APRSERVICE_LUA_MODULE_GPIO_PIN_VALUE_HIGH;

APRService.Modules.GPIO.PIN_DIRECTION_IN  = APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_IN;
APRService.Modules.GPIO.PIN_DIRECTION_OUT = APRSERVICE_LUA_MODULE_GPIO_PIN_DIRECTION_OUT;

-- @return pin
function APRService.Modules.GPIO.OpenPin(bus, pin, direction, value)
	return aprservice_lua_module_gpio_open_pin(tonumber(bus), tonumber(pin), direction, value);
end

function APRService.Modules.GPIO.ClosePin(pin)
	aprservice_lua_module_gpio_close_pin(pin);
end

APRService.Modules.GPIO.Pin = {};

-- @return success, value
function APRService.Modules.GPIO.Pin.Read(pin)
	return aprservice_lua_module_gpio_pin_read(pin);
end

function APRService.Modules.GPIO.Pin.Write(pin, value)
	return aprservice_lua_module_gpio_pin_write(pin, value);
end

-- @return success, timeout
function APRService.Modules.GPIO.Pin.WaitForEdge(pin, edge, max_wait_time_ms)
	return aprservice_lua_module_gpio_pin_wait_for_edge(pin, edge, tonumber(max_wait_time_ms));
end

function APRService.Modules.GPIO.Pin.SetPullUp(pin)
	return aprservice_lua_module_gpio_pin_set_pull_up(pin);
end

function APRService.Modules.GPIO.Pin.SetPullDown(pin)
	return aprservice_lua_module_gpio_pin_set_pull_down(pin);
end

function APRService.Modules.GPIO.Pin.GetDirection(pin)
	return aprservice_lua_module_gpio_pin_get_direction(pin);
end

function APRService.Modules.GPIO.Pin.SetDirection(pin, direction, value)
	return aprservice_lua_module_gpio_pin_set_direction(pin, direction, value);
end
