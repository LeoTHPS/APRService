require('APRService');

APRService.Modules.Environment = {};

-- @return exists, value
function APRService.Modules.Environment.Get(name)
	return aprservice_lua_module_environment_get(tostring(name));
end

function APRService.Modules.Environment.Set(name, value)
	return aprservice_lua_module_environment_set(tostring(name), tostring(value));
end

function APRService.Modules.Environment.Delete(name)
	return aprservice_lua_module_environment_delete(tostring(name));
end

-- @param callback(name, value)->bool
function APRService.Modules.Environment.Enumerate(callback)
	return aprservice_lua_module_environment_enumerate(callback);
end
