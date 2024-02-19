#include "aprservice_lua.hpp"

struct aprservice_lua;
struct aprservice_lua_modules;
struct aprservice_lua_module_sqlite3;

struct aprservice_lua_modules
{
	aprservice_lua*                lua;
	aprservice_lua_module_sqlite3* sqlite3;
};

aprservice_lua_module_sqlite3* aprservice_lua_module_sqlite3_init(aprservice_lua* lua);
void                           aprservice_lua_module_sqlite3_deinit(aprservice_lua_module_sqlite3* sqlite3);

aprservice_lua_modules* lua_aprservice_modules_init(aprservice_lua* lua)
{
	auto lua_modules = new aprservice_lua_modules
	{
		.lua = lua
	};

	if ((lua_modules->sqlite3 = aprservice_lua_module_sqlite3_init(lua)) == nullptr)
	{
		delete lua_modules;

		return nullptr;
	}

	return lua_modules;
}
void                    lua_aprservice_modules_deinit(aprservice_lua_modules* lua_modules)
{
	aprservice_lua_module_sqlite3_deinit(lua_modules->sqlite3);

	delete lua_modules;
}
