APRService = {};

APRService.FLAG_NONE                    = APRSERVICE_FLAG_NONE;
APRService.FLAG_STOP_ON_APRS_DISCONNECT = APRSERVICE_FLAG_STOP_ON_APRS_DISCONNECT;

-- @return service
function APRService.Init(config)
	return aprservice_init(config);
end

function APRService.Deinit(service)
	aprservice_deinit(service);
end

function APRService.IsRunning(service)
	return aprservice_is_running(service);
end

function APRService.Run(service, tick_rate, flags)
	aprservice_run(service, tonumber(tick_rate), tonumber(flags));
end

function APRService.Stop(service)
	aprservice_stop(service);
end

APRService.APRS             = {};
APRService.APRS.IS          = {};
APRService.APRS.KISS        = {};
APRService.APRS.KISS.Tcp    = {};
APRService.APRS.KISS.Serial = {};

APRService.APRS.PACKET_TYPE_UNKNOWN                     = APRSERVICE_APRS_PACKET_TYPE_UNKNOWN;
APRService.APRS.PACKET_TYPE_MESSAGE                     = APRSERVICE_APRS_PACKET_TYPE_MESSAGE;
APRService.APRS.PACKET_TYPE_WEATHER                     = APRSERVICE_APRS_PACKET_TYPE_WEATHER;
APRService.APRS.PACKET_TYPE_POSITION                    = APRSERVICE_APRS_PACKET_TYPE_POSITION;
APRService.APRS.PACKET_TYPE_TELEMETRY                   = APRSERVICE_APRS_PACKET_TYPE_TELEMETRY;

APRService.APRS.POSITION_FLAG_NONE                      = APRSERVICE_APRS_POSITION_FLAG_NONE;
APRService.APRS.POSITION_FLAG_COMPRESSED                = APRSERVICE_APRS_POSITION_FLAG_COMPRESSED;
APRService.APRS.POSITION_FLAG_MESSAGING_ENABLED         = APRSERVICE_APRS_POSITION_FLAG_MESSAGING_ENABLED;

APRService.APRS.CONNECTION_TYPE_NONE                    = APRSERVICE_APRS_CONNECTION_TYPE_NONE;
APRService.APRS.CONNECTION_TYPE_APRS_IS                 = APRSERVICE_APRS_CONNECTION_TYPE_APRS_IS;
APRService.APRS.CONNECTION_TYPE_KISS_TCP                = APRSERVICE_APRS_CONNECTION_TYPE_KISS_TCP;
APRService.APRS.CONNECTION_TYPE_KISS_SERIAL             = APRSERVICE_APRS_CONNECTION_TYPE_KISS_SERIAL;
APRService.APRS.CONNECTION_TYPE_USER_DEFINED            = APRSERVICE_APRS_CONNECTION_TYPE_USER_DEFINED;

APRService.APRS.DISCONNECT_REASON_UNDEFINED             = APRSERVICE_APRS_DISCONNECT_REASON_UNDEFINED;
APRService.APRS.DISCONNECT_REASON_USER_REQUESTED        = APRSERVICE_APRS_DISCONNECT_REASON_USER_REQUESTED;
APRService.APRS.DISCONNECT_REASON_CONNECTION_LOST       = APRSERVICE_APRS_DISCONNECT_REASON_CONNECTION_LOST;
APRService.APRS.DISCONNECT_REASON_AUTHENTICATION_FAILED = APRSERVICE_APRS_DISCONNECT_REASON_AUTHENTICATION_FAILED;

function APRService.APRS.IsConnected(service)
	return aprservice_aprs_is_connected(service);
end

-- @param is_blocking(service)->bool
-- @param is_connected(service)->bool
-- @param connect(service)->bool
-- @param disconnect(service)
-- @param read(service, value)->int (0 = connection_closed, -1 = would_block)
-- @param write(service, value)->bool
function APRService.APRS.Connect(service, is_blocking, is_connected, connect, disconnect, set_blocking, read, write)
	return lua_aprservice_aprs_connect(service, is_blocking, is_connected, connect, disconnect, set_blocking, read, write);
end

function APRService.APRS.IS.Connect(service, remote_host, remote_port, passcode)
	return aprservice_aprs_connect_is(service, tostring(remote_host), tonumber(remote_port), tonumber(passcode));
end

function APRService.APRS.KISS.Tcp.Connect(service, remote_host, remote_port)
	return aprservice_aprs_connect_kiss_tcp(service, tostring(remote_host), tonumber(remote_port));
end

function APRService.APRS.KISS.Serial.Connect(service, device, speed)
	return aprservice_aprs_connect_kiss_serial(service, tostring(device), tonumber(speed));
end

function APRService.APRS.Disconnect(service)
	aprservice_aprs_disconnect(service);
end

-- @param filter(service, station, tocall, path, igate, content)->bool
-- @param callback(service, station, tocall, path, igate, content)
function APRService.APRS.AddPacketMonitor(service, filter, callback)
	aprservice_aprs_add_packet_monitor(service, filter, callback);
end

-- @return encoding_failed, connection_closed
function APRService.APRS.SendMessage(service, destination, content)
	return aprservice_aprs_send_message(service, tostring(destination), tostring(content));
end

-- @return encoding_failed, connection_closed
function APRService.APRS.SendWeather(service, month, day, hours, minutes, wind_speed_mph, wind_speed_gust_mph, wind_direction, rainfall_last_hour_inches, rainfall_last_24_hours_inches, rainfall_since_midnight_inches, humidity, temperature_f, barometric_pressure_pa)
	return aprservice_aprs_send_weather(service, tonumber(month), tonumber(day), tonumber(hours), tonumber(minutes), tonumber(wind_speed_mph), tonumber(wind_speed_gust_mph), tonumber(wind_direction), tonumber(rainfall_last_hour_inches), tonumber(rainfall_last_24_hours_inches), tonumber(rainfall_since_midnight_inches), tonumber(humidity), tonumber(temperature_f), tonumber(barometric_pressure_pa));
end

-- @return encoding_failed, connection_closed
function APRService.APRS.SendPosition(service, altitude_ft, latitude, longitude, speed_mph, course, comment)
	return aprservice_aprs_send_position(service, tonumber(altitude_ft), tonumber(latitude), tonumber(longitude), tonumber(speed_mph), tonumber(course), tostring(comment));
end

-- @return encoding_failed, connection_closed
function APRService.APRS.SendTelemetry(service, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
	return aprservice_aprs_send_telemetry(service, tonumber(analog_1), tonumber(analog_2), tonumber(analog_3), tonumber(analog_4), tonumber(analog_5), digital_1 and true or false, digital_2 and true or false, digital_3 and true or false, digital_4 and true or false, digital_5 and true or false, digital_6 and true or false, digital_7 and true or false, digital_8 and true or false);
end

-- @param callback(service)
-- @return encoding_failed, connection_closed
function APRService.APRS.BeginSendMessage(service, destination, content, callback)
	return aprservice_aprs_begin_send_message(service, tostring(destination), tostring(content), callback);
end

APRService.Config = {};

-- @return config
function APRService.Config.Init()
	return aprservice_config_init();
end

function APRService.Config.Deinit(config)
	aprservice_config_deinit(config);
end

APRService.Config.APRS = {};

function APRService.Config.APRS.IsMonitorModeEnabled(config)
	return aprservice_config_get_monitor_mode(config);
end
function APRService.Config.APRS.EnableMonitorMode(config, value)
	aprservice_config_set_monitor_mode(config, value and true or false);
end

function APRService.Config.APRS.GetPath(config)
	return aprservice_config_get_path(config);
end
function APRService.Config.APRS.SetPath(config, value)
	aprservice_config_set_path(config, tostring(value));
end

function APRService.Config.APRS.GetStation(config)
	return aprservice_config_get_station(config);
end
function APRService.Config.APRS.SetStation(config, value)
	aprservice_config_set_station(config, tostring(value));
end

function APRService.Config.APRS.GetSymbolTable(config)
	return aprservice_config_get_symbol_table(config);
end
function APRService.Config.APRS.SetSymbolTable(config, value)
	aprservice_config_set_symbol_table(config, tostring(value));
end

function APRService.Config.APRS.GetSymbolTableKey(config)
	return aprservice_config_get_symbol_table_key(config);
end
function APRService.Config.APRS.SetSymbolTableKey(config, value)
	aprservice_config_set_symbol_table_key(config, tostring(value));
end

APRService.Config.Events = {};

-- @param handler(service, type)
function APRService.Config.Events.SetOnConnect(config, handler)
	aprservice_config_events_set_on_connect(config, handler);
end
-- @param handler(service, reason)
function APRService.Config.Events.SetOnDisconnect(config, handler)
	aprservice_config_events_set_on_disconnect(config, handler);
end

-- @param handler(service, value)
function APRService.Config.Events.SetOnSend(config, handler)
	aprservice_config_events_set_on_send(config, handler);
end
-- @param handler(service, value)
function APRService.Config.Events.SetOnReceive(config, handler)
	aprservice_config_events_set_on_receive(config, handler);
end

-- @param handler(service, station, tocall, path, igate, content)
function APRService.Config.Events.SetOnSendPacket(config, handler)
	aprservice_config_events_set_on_send_packet(config, handler);
end
-- @param handler(service, station, tocall, path, igate, content)
function APRService.Config.Events.SetOnReceivePacket(config, handler)
	aprservice_config_events_set_on_receive_packet(config, handler);
end

-- @param handler(service, station, path, igate, destination, content)
function APRService.Config.Events.SetOnSendMessage(config, handler)
	aprservice_config_events_set_on_send_message(config, handler);
end
-- @param handler(service, station, path, igate, destination, content)
function APRService.Config.Events.SetOnReceiveMessage(config, handler)
	aprservice_config_events_set_on_receive_message(config, handler);
end

-- @param handler(service, station, path, igate, month, day, hours, minutes, wind_speed_mph, wind_speed_gust_mph, wind_direction, rainfall_last_hour_inches, rainfall_last_24_hours_inches, rainfall_since_midnight_inches, humidity, temperature_f, barometric_pressure_pa)
function APRService.Config.Events.SetOnSendWeather(config, handler)
	aprservice_config_events_set_on_send_weather(config, handler);
end
-- @param handler(service, station, path, igate, month, day, hours, minutes, wind_speed_mph, wind_speed_gust_mph, wind_direction, rainfall_last_hour_inches, rainfall_last_24_hours_inches, rainfall_since_midnight_inches, humidity, temperature_f, barometric_pressure_pa)
function APRService.Config.Events.SetOnReceiveWeather(config, handler)
	aprservice_config_events_set_on_receive_weather(config, handler);
end

-- @param handler(service, station, path, igate, altitude_ft, latitude, longitude, speed_mph, course, symbol_table, symbol_table_key, comment, flags)
function APRService.Config.Events.SetOnSendPosition(config, handler)
	aprservice_config_events_set_on_send_position(config, handler);
end
-- @param handler(service, station, path, igate, altitude_ft, latitude, longitude, speed_mph, course, symbol_table, symbol_table_key, comment, flags)
function APRService.Config.Events.SetOnReceivePosition(config, handler)
	aprservice_config_events_set_on_receive_position(config, handler);
end

-- @param handler(service, station, tocall, path, igate, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
function APRService.Config.Events.SetOnSendTelemetry(config, handler)
	aprservice_config_events_set_on_send_telemetry(config, handler);
end
-- @param handler(service, station, tocall, path, igate, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
function APRService.Config.Events.SetOnReceiveTelemetry(config, handler)
	aprservice_config_events_set_on_receive_telemetry(config, handler);
end

-- @param handler(service, station, tocall, path, igate, content, type)
function APRService.Config.Events.SetOnReceiveInvalidPacket(config, handler)
	aprservice_config_events_set_on_receive_invalid_packet(config, handler);
end

APRService.Math = {};

APRService.Math.MEASUREMENT_TYPE_FEET       = APRSERVICE_MEASUREMENT_TYPE_FEET;
APRService.Math.MEASUREMENT_TYPE_MILES      = APRSERVICE_MEASUREMENT_TYPE_MILES;
APRService.Math.MEASUREMENT_TYPE_METERS     = APRSERVICE_MEASUREMENT_TYPE_METERS;
APRService.Math.MEASUREMENT_TYPE_KILOMETERS = APRSERVICE_MEASUREMENT_TYPE_KILOMETERS;

function APRService.Math.ConvertSpeed(value, measurement_type_input, measurement_type_output)
	return aprservice_math_convert_speed(tonumber(value), measurement_type_input, measurement_type_output);
end

function APRService.Math.GetDistanceBetweenPoints(latitude1, longitude1, latitude2, longitude2, measurement_type)
	return aprservice_math_get_distance_between_points(tonumber(latitude1), tonumber(longitude1), tonumber(latitude2), tonumber(longitude2), measurement_type);
end

APRService.Events = {};

function APRService.Events.GetCount(service)
	return aprservice_events_get_count(service);
end

function APRService.Events.Clear(service)
	aprservice_events_clear(service);
end

-- @param handler(service)
function APRService.Events.Schedule(service, seconds, handler)
	aprservice_events_schedule(service, tonumber(seconds), handler);
end

APRService.Console = {};

function APRService.Console.SetTitle(value)
	aprservice_console_set_title(tostring(value));
end

-- @return success, char
function APRService.Console.Read()
	return aprservice_console_read();
end

-- @return success, string
function APRService.Console.ReadLine()
	return aprservice_console_read_line();
end

function APRService.Console.Write(value)
	return aprservice_console_write(tostring(value));
end

function APRService.Console.WriteLine(value)
	return aprservice_console_write_line(tostring(value));
end

APRService.Modules = {};

APRService.Commands = {};

function APRService.Commands.Execute(service, sender, message)
	return aprservice_commands_execute(service, tostring(sender), tostring(message));
end

-- @param handler(service, sender, command_name, command_params)
function APRService.Commands.Register(service, name, handler)
	aprservice_commands_register(service, tostring(name), handler);
end
