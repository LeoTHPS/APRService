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

APRService.Modules.SQLite3.Database = {};

-- @return database
function APRService.Modules.SQLite3.Database.Open(path, flags)
	return aprservice_lua_module_sqlite3_database_open(tostring(path), tonumber(flags));
end

function APRService.Modules.SQLite3.Database.Close(database)
	aprservice_lua_module_sqlite3_database_close(database);
end

-- @return query_result
function APRService.Modules.SQLite3.Database.ExecuteQuery(database, query)
	return aprservice_lua_module_sqlite3_database_execute_query(database, tostring(query));
end

function APRService.Modules.SQLite3.Database.ExecuteNonQuery(database, query)
	return aprservice_lua_module_sqlite3_database_execute_non_query(database, tostring(query));
end

APRService.Modules.SQLite3.QueryResult = {};

function APRService.Modules.SQLite3.QueryResult.GetSize(query_result)
	return aprservice_lua_module_sqlite3_database_query_result_get_size(query_result);
end

-- @return query_result_row
function APRService.Modules.SQLite3.QueryResult.GetRow(query_result, index)
	return aprservice_lua_module_sqlite3_database_query_result_get_row(query_result, tonumber(index));
end

function APRService.Modules.SQLite3.QueryResult.Release(query_result)
	aprservice_lua_module_sqlite3_database_query_result_release(query_result);
end

APRService.Modules.SQLite3.QueryResult.Row = {};

function APRService.Modules.SQLite3.QueryResult.Row.GetSize(query_result_row)
	return aprservice_lua_module_sqlite3_database_query_result_row_get_size(query_result_row);
end

function APRService.Modules.SQLite3.QueryResult.Row.GetValue(query_result_row, index)
	return aprservice_lua_module_sqlite3_database_query_result_row_get_value(query_result_row, tonumber(index));
end

function APRService.Modules.SQLite3.QueryResult.Row.GetColumn(query_result_row, index)
	return aprservice_lua_module_sqlite3_database_query_result_row_get_column(query_result_row, tonumber(index));
end
