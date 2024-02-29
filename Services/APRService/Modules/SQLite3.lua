require('APRService');

APRService.Modules.SQLite3 = {};

APRService.Modules.SQLite3.FLAG_NONE          = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NONE;
APRService.Modules.SQLite3.FLAG_URI           = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_URI;
APRService.Modules.SQLite3.FLAG_CREATE        = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_CREATE;
APRService.Modules.SQLite3.FLAG_READ_ONLY     = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_ONLY;
APRService.Modules.SQLite3.FLAG_READ_WRITE    = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_WRITE;
APRService.Modules.SQLite3.FLAG_MEMORY        = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_MEMORY;
APRService.Modules.SQLite3.FLAG_NO_MUTEX      = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_MUTEX;
APRService.Modules.SQLite3.FLAG_FULL_MUTEX    = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_FULL_MUTEX;
APRService.Modules.SQLite3.FLAG_NO_FOLLOW     = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_FOLLOW;
APRService.Modules.SQLite3.FLAG_SHARED_CACHE  = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_SHARED_CACHE;
APRService.Modules.SQLite3.FLAG_PRIVATE_CACHE = APRSERVICE_LUA_MODULE_SQLITE3_FLAG_PRIVATE_CACHE;

-- @return sqlite3
function APRService.Modules.SQLite3.Open(path, flags)
	return aprservice_lua_module_sqlite3_open(tostring(path), tonumber(flags));
end

function APRService.Modules.SQLite3.Close(sqlite3)
	aprservice_lua_module_sqlite3_close(sqlite3);
end

-- @return query_result
function APRService.Modules.SQLite3.ExecuteQuery(sqlite3, query)
	return aprservice_lua_module_sqlite3_execute_query(sqlite3, tostring(query));
end

function APRService.Modules.SQLite3.ExecuteNonQuery(sqlite3, query)
	return aprservice_lua_module_sqlite3_execute_non_query(sqlite3, tostring(query));
end

APRService.Modules.SQLite3.QueryResult = {};

function APRService.Modules.SQLite3.QueryResult.GetSize(query_result)
	return aprservice_lua_module_sqlite3_query_result_get_size(query_result);
end

-- @return query_result_row
function APRService.Modules.SQLite3.QueryResult.GetRow(query_result, index)
	return aprservice_lua_module_sqlite3_query_result_get_row(query_result, tonumber(index));
end

function APRService.Modules.SQLite3.QueryResult.Release(query_result)
	aprservice_lua_module_sqlite3_query_result_release(query_result);
end

APRService.Modules.SQLite3.QueryResult.Row = {};

function APRService.Modules.SQLite3.QueryResult.Row.GetSize(query_result_row)
	return aprservice_lua_module_sqlite3_query_result_row_get_size(query_result_row);
end

function APRService.Modules.SQLite3.QueryResult.Row.GetValue(query_result_row, index)
	return aprservice_lua_module_sqlite3_query_result_row_get_value(query_result_row, tonumber(index));
end

function APRService.Modules.SQLite3.QueryResult.Row.GetColumn(query_result_row, index)
	return aprservice_lua_module_sqlite3_query_result_row_get_column(query_result_row, tonumber(index));
end
