APRService = {};

APRService.FLAGS_NONE                                   = APRSERVICE_FLAG_NONE;
APRService.FLAGS_STOP_ON_APRS_DISCONNECT                = APRSERVICE_FLAG_STOP_ON_APRS_DISCONNECT;

APRService.MEASUREMENT_TYPE_FEET                        = APRSERVICE_MEASUREMENT_TYPE_FEET;
APRService.MEASUREMENT_TYPE_MILES                       = APRSERVICE_MEASUREMENT_TYPE_MILES;
APRService.MEASUREMENT_TYPE_METERS                      = APRSERVICE_MEASUREMENT_TYPE_METERS;
APRService.MEASUREMENT_TYPE_KILOMETERS                  = APRSERVICE_MEASUREMENT_TYPE_KILOMETERS;

APRService.APRS_PACKET_TYPE_UNKNOWN                     = APRSERVICE_APRS_PACKET_TYPE_UNKNOWN;
APRService.APRS_PACKET_TYPE_MESSAGE                     = APRSERVICE_APRS_PACKET_TYPE_MESSAGE;
APRService.APRS_PACKET_TYPE_POSITION                    = APRSERVICE_APRS_PACKET_TYPE_POSITION;
APRService.APRS_PACKET_TYPE_TELEMETRY                   = APRSERVICE_APRS_PACKET_TYPE_TELEMETRY;

APRService.APRS_POSITION_FLAG_NONE                      = APRSERVICE_APRS_POSITION_FLAG_NONE;
APRService.APRS_POSITION_FLAG_COMPRESSED                = APRSERVICE_APRS_POSITION_FLAG_COMPRESSED;
APRService.APRS_POSITION_FLAG_MESSAGING_ENABLED         = APRSERVICE_APRS_POSITION_FLAG_MESSAGING_ENABLED;

APRService.APRS_CONNECTION_TYPE_NONE                    = APRSERVICE_APRS_CONNECTION_TYPE_NONE;
APRService.APRS_CONNECTION_TYPE_APRS_IS                 = APRSERVICE_APRS_CONNECTION_TYPE_APRS_IS;
APRService.APRS_CONNECTION_TYPE_KISS_TCP                = APRSERVICE_APRS_CONNECTION_TYPE_KISS_TCP;
APRService.APRS_CONNECTION_TYPE_KISS_SERIAL             = APRSERVICE_APRS_CONNECTION_TYPE_KISS_SERIAL;

APRService.APRS_DISCONNECT_REASON_UNDEFINED             = APRSERVICE_APRS_DISCONNECT_REASON_UNDEFINED;
APRService.APRS_DISCONNECT_REASON_USER_REQUESTED        = APRSERVICE_APRS_DISCONNECT_REASON_USER_REQUESTED;
APRService.APRS_DISCONNECT_REASON_CONNECTION_LOST       = APRSERVICE_APRS_DISCONNECT_REASON_CONNECTION_LOST;
APRService.APRS_DISCONNECT_REASON_AUTHENTICATION_FAILED = APRSERVICE_APRS_DISCONNECT_REASON_AUTHENTICATION_FAILED;

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

function APRService.APRS.IsConnected(service)
	return aprservice_aprs_is_connected(service);
end

function APRService.APRS.IS.Connect(service, remote_host, remote_port, passcode)
	return aprservice_aprs_connect_is(service, tostring(remote_host), tonumber(remote_port), tonumber(passcode));
end

function APRService.APRS.KISS.Tcp.Connect(service, remote_host, remote_port)
	return aprservice_aprs_connect_kiss_tcp(service, tostring(remote_host), tonumber(remote_port));
end

function APRService.APRS.KISS.Serial.Connect(service, device)
	return aprservice_aprs_connect_kiss_serial(service, tostring(device));
end

function APRService.APRS.Disconnect(service)
	aprservice_aprs_disconnect(service);
end

-- @param filter(service, station, tocall, path, content)->bool
-- @param callback(service, station, tocall, path, content)
function APRService.APRS.AddPacketMonitor(service, filter, callback)
	aprservice_aprs_add_packet_monitor(service, filter, callback);
end

-- @return encoding_failed, connection_closed
function APRService.APRS.SendMessage(service, destination, content)
	return aprservice_aprs_send_message(service, tostring(destination), tostring(content));
end

-- @return encoding_failed, connection_closed
function APRService.APRS.SendPosition(service, altitude, latitude, longitude, comment)
	return aprservice_aprs_send_position(service, tonumber(altitude), tonumber(latitude), tonumber(longitude), tostring(comment));
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

-- @param handler(service, station, tocall, path, content)
function APRService.Config.Events.SetOnSendPacket(config, handler)
	aprservice_config_events_set_on_send_packet(config, handler);
end
-- @param handler(service, station, tocall, path, content)
function APRService.Config.Events.SetOnReceivePacket(config, handler)
	aprservice_config_events_set_on_receive_packet(config, handler);
end

-- @param handler(service, station, path, destination, content)
function APRService.Config.Events.SetOnSendMessage(config, handler)
	aprservice_config_events_set_on_send_message(config, handler);
end
-- @param handler(service, station, path, destination, content)
function APRService.Config.Events.SetOnReceiveMessage(config, handler)
	aprservice_config_events_set_on_receive_message(config, handler);
end

-- @param handler(service, station, path, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
function APRService.Config.Events.SetOnSendPosition(config, handler)
	aprservice_config_events_set_on_send_position(config, handler);
end
-- @param handler(service, station, path, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
function APRService.Config.Events.SetOnReceivePosition(config, handler)
	aprservice_config_events_set_on_receive_position(config, handler);
end

-- @param handler(service, station, tocall, path, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
function APRService.Config.Events.SetOnSendTelemetry(config, handler)
	aprservice_config_events_set_on_send_telemetry(config, handler);
end
-- @param handler(service, station, tocall, path, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
function APRService.Config.Events.SetOnReceiveTelemetry(config, handler)
	aprservice_config_events_set_on_receive_telemetry(config, handler);
end

-- @param handler(service, station, tocall, path, content, type)
function APRService.Config.Events.SetOnReceiveInvalidPacket(config, handler)
	aprservice_config_events_set_on_receive_invalid_packet(config, handler);
end

APRService.Math = {};

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

APRService.Commands = {};

function APRService.Commands.Execute(service, sender, message)
	return aprservice_commands_execute(service, tostring(sender), tostring(message));
end

-- @param handler(service, sender, command_name, command_params)
function APRService.Commands.Register(service, name, handler)
	aprservice_commands_register(service, tostring(name), handler);
end
