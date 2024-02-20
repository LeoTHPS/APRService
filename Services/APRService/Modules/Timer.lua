require('APRService');

APRService.Modules.Timer = {};

-- @return timer
function APRService.Modules.Timer.Create()
	return aprservice_lua_module_timer_create();
end

function APRService.Modules.Timer.Destroy(timer)
	aprservice_lua_module_timer_destroy(timer);
end

function APRService.Modules.Timer.Reset(timer)
	aprservice_lua_module_timer_reset(timer);
end

function APRService.Modules.Timer.GetElapsedMS(timer)
	return aprservice_lua_module_timer_get_elapsed_ms(timer);
end

function APRService.Modules.Timer.GetElapsedUS(timer)
	return aprservice_lua_module_timer_get_elapsed_us(timer);
end
