#include "aprservice_lua.hpp"

#include <AL/OS/Console.hpp>

int main(int argc, char* argv[])
{
	AL::String script_path;

	if (argc == 2)
		script_path = argv[1];
	else if (!AL::OS::Console::Write("Script Path: ") || !AL::OS::Console::ReadLine(script_path))
		return 0;

	if (auto lua = aprservice_lua_init())
	{
		aprservice_lua_run_file(lua, script_path);
		aprservice_lua_deinit(lua);
	}

	return 0;
}
