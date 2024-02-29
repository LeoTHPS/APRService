#include "aprservice.hpp"
#include "aprservice_lua.hpp"
#include "aprservice_lua_module_sqlite3.hpp"

#include <AL/Lua54/Lua.hpp>

#include <AL/Collections/UnorderedSet.hpp>

void                                            aprservice_lua_module_sqlite3_register_globals(aprservice_lua* lua)
{
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

	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_open);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_close);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_execute_query);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_execute_non_query);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_query_result_get_size);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_query_result_get_row);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_query_result_release);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_query_result_row_get_size);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_query_result_row_get_value);
	aprservice_lua_state_register_global_function(lua_state, aprservice_lua_module_sqlite3_query_result_row_get_column);
}

aprservice_lua_module_sqlite3*                  aprservice_lua_module_sqlite3_open(const AL::String& path, AL::uint16 flags)
{
	AL::BitMask<AL::SQLite3::DatabaseFlags> sqlite3_flags;
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::URI,          AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_URI));
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::Create,       AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_CREATE));
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::ReadOnly,     AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_ONLY));
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::ReadWrite,    AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_READ_WRITE));
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::Memory,       AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_MEMORY));
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::NoMutex,      AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_MUTEX));
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::FullMutex,    AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_FULL_MUTEX));
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::NoFollow,     AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_NO_FOLLOW));
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::SharedCache,  AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_SHARED_CACHE));
	sqlite3_flags.Set(AL::SQLite3::DatabaseFlags::PrivateCache, AL::BitMask<AL::uint16>::IsSet(flags, APRSERVICE_LUA_MODULE_SQLITE3_FLAG_PRIVATE_CACHE));

	auto sqlite3 = new aprservice_lua_module_sqlite3(AL::FileSystem::Path(path), sqlite3_flags.Value);

	try
	{
		sqlite3->Open();
	}
	catch (const AL::Exception& exception)
	{
		delete sqlite3;

		aprservice_console_write_line(AL::String::Format("Error opening AL::SQLite3::Database [Path: %s, Flags: 0x%04X]", path.GetCString(), flags));
		aprservice_console_write_exception(exception);

		return nullptr;
	}

	return sqlite3;
}
void                                            aprservice_lua_module_sqlite3_close(aprservice_lua_module_sqlite3* sqlite3)
{
	sqlite3->Close();

	delete sqlite3;
}

aprservice_lua_module_sqlite3_query_result*     aprservice_lua_module_sqlite3_execute_query(aprservice_lua_module_sqlite3* sqlite3, const AL::String& query)
{
	auto query_result = new aprservice_lua_module_sqlite3_query_result();

	try
	{
		*query_result = sqlite3->Query(query.GetCString());
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
bool                                            aprservice_lua_module_sqlite3_execute_non_query(aprservice_lua_module_sqlite3* sqlite3, const AL::String& query)
{
	try
	{
		sqlite3->Query(query.GetCString());
	}
	catch (const AL::Exception& exception)
	{
		aprservice_console_write_line(AL::String::Format("Error executing query: %s", query.GetCString()));
		aprservice_console_write_exception(exception);

		return false;
	}

	return true;
}

AL::uint32                                      aprservice_lua_module_sqlite3_query_result_get_size(aprservice_lua_module_sqlite3_query_result* query_result)
{
	return static_cast<AL::uint32>(query_result->GetSize());
}
aprservice_lua_module_sqlite3_query_result_row* aprservice_lua_module_sqlite3_query_result_get_row(aprservice_lua_module_sqlite3_query_result* query_result, AL::uint32 index)
{
	if ((index - 1) >= query_result->GetSize())
		return nullptr;

	for (auto& query_result_row : *query_result)
		if (--index == 0)
			return &query_result_row;

	return nullptr;
}
void                                            aprservice_lua_module_sqlite3_query_result_release(aprservice_lua_module_sqlite3_query_result* query_result)
{
	delete query_result;
}

AL::uint32                                      aprservice_lua_module_sqlite3_query_result_row_get_size(aprservice_lua_module_sqlite3_query_result_row* query_result_row)
{
	return static_cast<AL::uint32>(query_result_row->Columns.GetSize());
}
const AL::String&                               aprservice_lua_module_sqlite3_query_result_row_get_value(aprservice_lua_module_sqlite3_query_result_row* query_result_row, AL::uint32 index)
{
	if ((index - 1) >= query_result_row->Values.GetSize())
	{
		static AL::String DEFAULT_VALUE;

		return DEFAULT_VALUE;
	}

	return query_result_row->Values[index - 1];
}
const AL::String&                               aprservice_lua_module_sqlite3_query_result_row_get_column(aprservice_lua_module_sqlite3_query_result_row* query_result_row, AL::uint32 index)
{
	if ((index - 1) >= query_result_row->Columns.GetSize())
	{
		static AL::String DEFAULT_VALUE;

		return DEFAULT_VALUE;
	}

	return query_result_row->Columns[index - 1];
}
