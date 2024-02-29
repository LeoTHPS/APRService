#pragma once
#include <AL/Common.hpp>

#include <AL/SQLite3/Database.hpp>

enum APRSERVICE_LUA_MODULE_SQLITE3_FLAGS : AL::uint16
{
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NONE          = 0x0000,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_URI           = 0x0001,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_CREATE        = 0x0002,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_ONLY     = 0x0004,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_WRITE    = 0x0008,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_MEMORY        = 0x0010,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_MUTEX      = 0x0020,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_FULL_MUTEX    = 0x0040,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_FOLLOW     = 0x0080,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_SHARED_CACHE  = 0x0100,
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_PRIVATE_CACHE = 0x0200
};

struct                                      aprservice_lua;
typedef AL::SQLite3::Database               aprservice_lua_module_sqlite3;
typedef AL::SQLite3::DatabaseQueryResult    aprservice_lua_module_sqlite3_query_result;
typedef AL::SQLite3::DatabaseQueryResultRow aprservice_lua_module_sqlite3_query_result_row;

void                                            aprservice_lua_module_sqlite3_register_globals(aprservice_lua* lua);

aprservice_lua_module_sqlite3*                  aprservice_lua_module_sqlite3_open(const AL::String& path, AL::uint16 flags);
void                                            aprservice_lua_module_sqlite3_close(aprservice_lua_module_sqlite3* sqlite3);

aprservice_lua_module_sqlite3_query_result*     aprservice_lua_module_sqlite3_execute_query(aprservice_lua_module_sqlite3* sqlite3, const AL::String& query);
bool                                            aprservice_lua_module_sqlite3_execute_non_query(aprservice_lua_module_sqlite3* sqlite3, const AL::String& query);

AL::uint32                                      aprservice_lua_module_sqlite3_query_result_get_size(aprservice_lua_module_sqlite3_query_result* query_result);
aprservice_lua_module_sqlite3_query_result_row* aprservice_lua_module_sqlite3_query_result_get_row(aprservice_lua_module_sqlite3_query_result* query_result, AL::uint32 index);
void                                            aprservice_lua_module_sqlite3_query_result_release(aprservice_lua_module_sqlite3_query_result* query_result);

AL::uint32                                      aprservice_lua_module_sqlite3_query_result_row_get_size(aprservice_lua_module_sqlite3_query_result_row* query_result_row);
const AL::String&                               aprservice_lua_module_sqlite3_query_result_row_get_value(aprservice_lua_module_sqlite3_query_result_row* query_result_row, AL::uint32 index);
const AL::String&                               aprservice_lua_module_sqlite3_query_result_row_get_column(aprservice_lua_module_sqlite3_query_result_row* query_result_row, AL::uint32 index);
