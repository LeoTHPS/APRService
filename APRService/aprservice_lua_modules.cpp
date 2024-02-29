#include "aprservice_lua.hpp"

#include "aprservice_lua_module_byte_buffer.hpp"
#include "aprservice_lua_module_csv.hpp"
#include "aprservice_lua_module_environment.hpp"
#include "aprservice_lua_module_file.hpp"
#include "aprservice_lua_module_gpio.hpp"
#include "aprservice_lua_module_http.hpp"
#include "aprservice_lua_module_i2c.hpp"
#include "aprservice_lua_module_ini.hpp"
#include "aprservice_lua_module_json.hpp"
#include "aprservice_lua_module_mutex.hpp"
#include "aprservice_lua_module_process.hpp"
#include "aprservice_lua_module_socket.hpp"
#include "aprservice_lua_module_spi.hpp"
#include "aprservice_lua_module_sqlite3.hpp"
#include "aprservice_lua_module_system.hpp"
#include "aprservice_lua_module_text_file.hpp"
#include "aprservice_lua_module_thread.hpp"
#include "aprservice_lua_module_timer.hpp"
#include "aprservice_lua_module_uart.hpp"

void aprservice_lua_modules_register_globals(aprservice_lua* lua)
{
	aprservice_lua_module_byte_buffer_register_globals(lua);
	aprservice_lua_module_csv_register_globals(lua);
	aprservice_lua_module_environment_register_globals(lua);
	aprservice_lua_module_file_register_globals(lua);
	aprservice_lua_module_gpio_register_globals(lua);
	aprservice_lua_module_http_register_globals(lua);
	aprservice_lua_module_i2c_register_globals(lua);
	aprservice_lua_module_ini_register_globals(lua);
	aprservice_lua_module_json_register_globals(lua);
	aprservice_lua_module_mutex_register_globals(lua);
	aprservice_lua_module_process_register_globals(lua);
	aprservice_lua_module_socket_register_globals(lua);
	aprservice_lua_module_spi_register_globals(lua);
	aprservice_lua_module_sqlite3_register_globals(lua);
	aprservice_lua_module_system_register_globals(lua);
	aprservice_lua_module_text_file_register_globals(lua);
	aprservice_lua_module_thread_register_globals(lua);
	aprservice_lua_module_timer_register_globals(lua);
	aprservice_lua_module_uart_register_globals(lua);
}
