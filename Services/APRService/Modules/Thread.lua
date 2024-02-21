require('APRService');

APRService.Modules.Thread = {};

function APRService.Modules.Thread.IsRunning(thread)
	return aprservice_lua_module_thread_is_running(thread);
end

-- @param main()
function APRService.Modules.Thread.Run(main)
	return aprservice_lua_module_thread_run(main);
end

-- @param main()
-- @return thread
function APRService.Modules.Thread.Start(main)
	return aprservice_lua_module_thread_start(main);
end

function APRService.Modules.Thread.Join(thread)
	aprservice_lua_module_thread_join(thread);
end
