#include "aprservice.hpp"
#include "aprservice_lua.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/SQLite3/Database.hpp>

#include <AL/Collections/UnorderedSet.hpp>

enum APRSERVICE_LUA_MODULE_SQLITE3_FLAGS : AL::uint16
{
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NONE          = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::None),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_URI           = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::URI),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_CREATE        = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::Create),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_ONLY     = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::ReadOnly),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_WRITE    = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::ReadWrite),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_MEMORY        = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::Memory),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_MUTEX      = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::NoMutex),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_FULL_MUTEX    = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::FullMutex),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_FOLLOW     = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::NoFollow),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_SHARED_CACHE  = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::SharedCache),
	APRSERVICE_LUA_MODULE_SQLITE3_FLAG_PRIVATE_CACHE = static_cast<AL::uint16>(AL::SQLite3::DatabaseFlags::PrivateCache)
};

typedef AL::SQLite3::Database               aprservice_lua_module_sqlite3_database;
typedef AL::SQLite3::DatabaseQueryResult    aprservice_lua_module_sqlite3_database_query_result;
typedef AL::SQLite3::DatabaseQueryResultRow aprservice_lua_module_sqlite3_database_query_result_row;

struct aprservice_lua_module_sqlite3
{
};

aprservice_lua_module_sqlite3_database*                  aprservice_lua_module_sqlite3_database_open(const AL::String& path, AL::uint16 flags)
{
	auto database = new aprservice_lua_module_sqlite3_database(AL::FileSystem::Path(path), static_cast<AL::SQLite3::DatabaseFlags>(flags));

	try
	{
		database->Open();
	}
	catch (const AL::Exception& exception)
	{
		delete database;

		aprservice_console_write_line(AL::String::Format("Error opening AL::SQLite3::Database [Path: %s, Flags: 0x%04X]", path.GetCString(), flags));
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return database;
}
void                                                     aprservice_lua_module_sqlite3_database_close(aprservice_lua_module_sqlite3_database* database)
{
	database->Close();

	delete database;
}
aprservice_lua_module_sqlite3_database_query_result*     aprservice_lua_module_sqlite3_database_execute_query(aprservice_lua_module_sqlite3_database* database, const AL::String& query)
{
	auto query_result = new aprservice_lua_module_sqlite3_database_query_result();

	try
	{
		*query_result = database->Query(query.GetCString());
	}
	catch (const AL::Exception& exception)
	{
		delete query_result;

		aprservice_console_write_line(AL::String::Format("Error executing query: %s", query.GetCString()));
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return query_result;
}
AL::uint32                                               aprservice_lua_module_sqlite3_database_query_result_get_size(aprservice_lua_module_sqlite3_database_query_result* query_result)
{
	return static_cast<AL::uint32>(query_result->GetSize());
}
aprservice_lua_module_sqlite3_database_query_result_row* aprservice_lua_module_sqlite3_database_query_result_get_row(aprservice_lua_module_sqlite3_database_query_result* query_result, AL::uint32 index)
{
	if ((index - 1) >= query_result->GetSize())
		return nullptr;

	for (auto& query_result_row : *query_result)
		if (--index == 0)
			return &query_result_row;

	return nullptr;
}
void                                                     aprservice_lua_module_sqlite3_database_query_result_release(aprservice_lua_module_sqlite3_database_query_result* query_result)
{
	delete query_result;
}
AL::uint32                                               aprservice_lua_module_sqlite3_database_query_result_row_get_size(aprservice_lua_module_sqlite3_database_query_result_row* query_result_row)
{
	return static_cast<AL::uint32>(query_result_row->Columns.GetSize());
}
const AL::String&                                        aprservice_lua_module_sqlite3_database_query_result_row_get_value(aprservice_lua_module_sqlite3_database_query_result_row* query_result_row, AL::uint32 index)
{
	if ((index - 1) >= query_result_row->Values.GetSize())
	{
		static AL::String DEFAULT_VALUE;

		return DEFAULT_VALUE;
	}

	return query_result_row->Values[index - 1];
}
const AL::String&                                        aprservice_lua_module_sqlite3_database_query_result_row_get_column(aprservice_lua_module_sqlite3_database_query_result_row* query_result_row, AL::uint32 index)
{
	if ((index - 1) >= query_result_row->Columns.GetSize())
	{
		static AL::String DEFAULT_VALUE;

		return DEFAULT_VALUE;
	}

	return query_result_row->Columns[index - 1];
}

aprservice_lua_module_sqlite3* aprservice_lua_module_sqlite3_init(aprservice_lua* lua)
{
	auto sqlite3 = new aprservice_lua_module_sqlite3
	{
	};

	auto lua_state = aprservice_lua_get_state(lua);

	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NONE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_URI);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_CREATE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_ONLY);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_WRITE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_MEMORY);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_MUTEX);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_FULL_MUTEX);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_FOLLOW);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_SHARED_CACHE);
	aprservice_lua_state_register_global(lua_state, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_PRIVATE_CACHE);

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_database_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_database_close);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_database_execute_query);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_database_query_result_get_size);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_database_query_result_get_row);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_database_query_result_release);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_database_query_result_row_get_size);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_database_query_result_row_get_value);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_database_query_result_row_get_column);

	return sqlite3;
}
void                           aprservice_lua_module_sqlite3_deinit(aprservice_lua_module_sqlite3* sqlite3)
{
	delete sqlite3;
}
