#include "aprservice_lua.hpp"

struct aprservice_lua;
struct aprservice_lua_modules;
struct aprservice_lua_module_byte_buffer;
struct aprservice_lua_module_csv;
struct aprservice_lua_module_environment;
struct aprservice_lua_module_file;
struct aprservice_lua_module_gpio;
struct aprservice_lua_module_http;
struct aprservice_lua_module_i2c;
struct aprservice_lua_module_ini;
struct aprservice_lua_module_irc;
struct aprservice_lua_module_json;
struct aprservice_lua_module_mutex;
struct aprservice_lua_module_process;
struct aprservice_lua_module_socket;
struct aprservice_lua_module_spi;
struct aprservice_lua_module_sqlite3;
struct aprservice_lua_module_ssh;
struct aprservice_lua_module_system;
struct aprservice_lua_module_text_file;
struct aprservice_lua_module_thread;
struct aprservice_lua_module_timer;
struct aprservice_lua_module_uart;

struct aprservice_lua_modules
{
	aprservice_lua*                    lua;
	aprservice_lua_module_byte_buffer* byte_buffer;
	aprservice_lua_module_csv*         csv;
	aprservice_lua_module_environment* environment;
	aprservice_lua_module_file*        file;
	aprservice_lua_module_gpio*        gpio;
	aprservice_lua_module_http*        http;
	aprservice_lua_module_i2c*         i2c;
	aprservice_lua_module_ini*         ini;
	aprservice_lua_module_irc*         irc;
	aprservice_lua_module_json*        json;
	aprservice_lua_module_mutex*       mutex;
	aprservice_lua_module_process*     process;
	aprservice_lua_module_socket*      socket;
	aprservice_lua_module_spi*         spi;
	aprservice_lua_module_sqlite3*     sqlite3;
	aprservice_lua_module_ssh*         ssh;
	aprservice_lua_module_system*      system;
	aprservice_lua_module_text_file*   text_file;
	aprservice_lua_module_thread*      thread;
	aprservice_lua_module_timer*       timer;
	aprservice_lua_module_uart*        uart;
};

aprservice_lua_module_byte_buffer* aprservice_lua_module_byte_buffer_init(aprservice_lua* lua);
void                               aprservice_lua_module_byte_buffer_deinit(aprservice_lua_module_byte_buffer* byte_buffer);

aprservice_lua_module_csv*         aprservice_lua_module_csv_init(aprservice_lua* lua);
void                               aprservice_lua_module_csv_deinit(aprservice_lua_module_csv* csv);

aprservice_lua_module_environment* aprservice_lua_module_environment_init(aprservice_lua* lua);
void                               aprservice_lua_module_environment_deinit(aprservice_lua_module_environment* environment);

aprservice_lua_module_file*        aprservice_lua_module_file_init(aprservice_lua* lua);
void                               aprservice_lua_module_file_deinit(aprservice_lua_module_file* file);

aprservice_lua_module_gpio*        aprservice_lua_module_gpio_init(aprservice_lua* lua);
void                               aprservice_lua_module_gpio_deinit(aprservice_lua_module_gpio* gpio);

aprservice_lua_module_http*        aprservice_lua_module_http_init(aprservice_lua* lua);
void                               aprservice_lua_module_http_deinit(aprservice_lua_module_http* http);

aprservice_lua_module_i2c*         aprservice_lua_module_i2c_init(aprservice_lua* lua);
void                               aprservice_lua_module_i2c_deinit(aprservice_lua_module_i2c* i2c);

aprservice_lua_module_ini*         aprservice_lua_module_ini_init(aprservice_lua* lua);
void                               aprservice_lua_module_ini_deinit(aprservice_lua_module_ini* ini);

aprservice_lua_module_irc*         aprservice_lua_module_irc_init(aprservice_lua* lua);
void                               aprservice_lua_module_irc_deinit(aprservice_lua_module_irc* irc);

aprservice_lua_module_json*        aprservice_lua_module_json_init(aprservice_lua* lua);
void                               aprservice_lua_module_json_deinit(aprservice_lua_module_json* json);

aprservice_lua_module_mutex*       aprservice_lua_module_mutex_init(aprservice_lua* lua);
void                               aprservice_lua_module_mutex_deinit(aprservice_lua_module_mutex* mutex);

aprservice_lua_module_process*     aprservice_lua_module_process_init(aprservice_lua* lua);
void                               aprservice_lua_module_process_deinit(aprservice_lua_module_process* process);

aprservice_lua_module_socket*      aprservice_lua_module_socket_init(aprservice_lua* lua);
void                               aprservice_lua_module_socket_deinit(aprservice_lua_module_socket* socket);

aprservice_lua_module_spi*         aprservice_lua_module_spi_init(aprservice_lua* lua);
void                               aprservice_lua_module_spi_deinit(aprservice_lua_module_spi* spi);

aprservice_lua_module_sqlite3*     aprservice_lua_module_sqlite3_init(aprservice_lua* lua);
void                               aprservice_lua_module_sqlite3_deinit(aprservice_lua_module_sqlite3* sqlite3);

aprservice_lua_module_ssh*         aprservice_lua_module_ssh_init(aprservice_lua* lua);
void                               aprservice_lua_module_ssh_deinit(aprservice_lua_module_ssh* ssh);

aprservice_lua_module_system*      aprservice_lua_module_system_init(aprservice_lua* lua);
void                               aprservice_lua_module_system_deinit(aprservice_lua_module_system* system);

aprservice_lua_module_text_file*   aprservice_lua_module_text_file_init(aprservice_lua* lua);
void                               aprservice_lua_module_text_file_deinit(aprservice_lua_module_text_file* text_file);

aprservice_lua_module_thread*      aprservice_lua_module_thread_init(aprservice_lua* lua);
void                               aprservice_lua_module_thread_deinit(aprservice_lua_module_thread* thread);

aprservice_lua_module_timer*       aprservice_lua_module_timer_init(aprservice_lua* lua);
void                               aprservice_lua_module_timer_deinit(aprservice_lua_module_timer* timer);

aprservice_lua_module_uart*        aprservice_lua_module_uart_init(aprservice_lua* lua);
void                               aprservice_lua_module_uart_deinit(aprservice_lua_module_uart* uart);

void lua_aprservice_modules_deinit(aprservice_lua_modules* lua_modules);

#define                 lua_aprservice_modules_init_module(lua_modules, lua, module_name)   \
	if ((lua_modules->module_name = aprservice_lua_module_##module_name##_init(lua)) == nullptr) \
	{ \
		lua_aprservice_modules_deinit(lua_modules); \
		\
		return nullptr; \
	}
aprservice_lua_modules* lua_aprservice_modules_init(aprservice_lua* lua)
{
	auto lua_modules = new aprservice_lua_modules
	{
		.lua = lua
	};

	lua_aprservice_modules_init_module(lua_modules, lua, byte_buffer);
	lua_aprservice_modules_init_module(lua_modules, lua, csv);
	lua_aprservice_modules_init_module(lua_modules, lua, environment);
	lua_aprservice_modules_init_module(lua_modules, lua, file);
	lua_aprservice_modules_init_module(lua_modules, lua, gpio);
	lua_aprservice_modules_init_module(lua_modules, lua, http);
	lua_aprservice_modules_init_module(lua_modules, lua, i2c);
	lua_aprservice_modules_init_module(lua_modules, lua, ini);
	lua_aprservice_modules_init_module(lua_modules, lua, irc);
	lua_aprservice_modules_init_module(lua_modules, lua, json);
	lua_aprservice_modules_init_module(lua_modules, lua, mutex);
	lua_aprservice_modules_init_module(lua_modules, lua, process);
	lua_aprservice_modules_init_module(lua_modules, lua, socket);
	lua_aprservice_modules_init_module(lua_modules, lua, spi);
	lua_aprservice_modules_init_module(lua_modules, lua, sqlite3);
	lua_aprservice_modules_init_module(lua_modules, lua, ssh);
	lua_aprservice_modules_init_module(lua_modules, lua, system);
	lua_aprservice_modules_init_module(lua_modules, lua, text_file);
	lua_aprservice_modules_init_module(lua_modules, lua, thread);
	lua_aprservice_modules_init_module(lua_modules, lua, timer);
	lua_aprservice_modules_init_module(lua_modules, lua, uart);

	return lua_modules;
}
#define                 lua_aprservice_modules_deinit_module(lua_modules, module_name) \
	if (auto module = lua_modules->module_name) \
		aprservice_lua_module_##module_name##_deinit(module)
void                    lua_aprservice_modules_deinit(aprservice_lua_modules* lua_modules)
{
	lua_aprservice_modules_deinit_module(lua_modules, byte_buffer);
	lua_aprservice_modules_deinit_module(lua_modules, csv);
	lua_aprservice_modules_deinit_module(lua_modules, environment);
	lua_aprservice_modules_deinit_module(lua_modules, file);
	lua_aprservice_modules_deinit_module(lua_modules, gpio);
	lua_aprservice_modules_deinit_module(lua_modules, http);
	lua_aprservice_modules_deinit_module(lua_modules, i2c);
	lua_aprservice_modules_deinit_module(lua_modules, ini);
	lua_aprservice_modules_deinit_module(lua_modules, irc);
	lua_aprservice_modules_deinit_module(lua_modules, json);
	lua_aprservice_modules_deinit_module(lua_modules, mutex);
	lua_aprservice_modules_deinit_module(lua_modules, process);
	lua_aprservice_modules_deinit_module(lua_modules, socket);
	lua_aprservice_modules_deinit_module(lua_modules, spi);
	lua_aprservice_modules_deinit_module(lua_modules, sqlite3);
	lua_aprservice_modules_deinit_module(lua_modules, ssh);
	lua_aprservice_modules_deinit_module(lua_modules, system);
	lua_aprservice_modules_deinit_module(lua_modules, text_file);
	lua_aprservice_modules_deinit_module(lua_modules, thread);
	lua_aprservice_modules_deinit_module(lua_modules, timer);
	lua_aprservice_modules_deinit_module(lua_modules, uart);

	delete lua_modules;
}
