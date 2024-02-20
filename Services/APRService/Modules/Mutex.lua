require('APRService');

APRService.Modules.Mutex = {};

-- @return mutex
function APRService.Modules.Mutex.Create()
	return aprservice_lua_module_mutex_create();
end

function APRService.Modules.Mutex.Destroy(mutex)
	aprservice_lua_module_mutex_destroy(mutex);
end

function APRService.Modules.Mutex.Lock(mutex)
	aprservice_lua_module_mutex_lock(mutex);
end

function APRService.Modules.Mutex.Unlock(mutex)
	aprservice_lua_module_mutex_unlock(mutex);
end
