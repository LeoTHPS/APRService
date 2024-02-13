#include "aprservice_lua.hpp"

int main(int argc, char* argv[])
{
	if (auto lua = aprservice_lua_init())
	{
		aprservice_lua_run_file(lua, argv[1]);
		aprservice_lua_deinit(lua);
	}

	return 0;
}
