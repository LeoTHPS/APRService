require('APRService');

local aprs_is_config =
{
	['Host']     = 'noam.aprs2.net',
	['Port']     = 14580,
	['Passcode'] = 0
};

local config = APRService.Config.Init();

APRService.Config.APRS.SetPath(config, 'WIDE1-1');
APRService.Config.APRS.SetStation(config, 'N0CALL');
APRService.Config.APRS.SetSymbolTable(config, '/');
APRService.Config.APRS.SetSymbolTableKey(config, 'k');
APRService.Config.APRS.EnableMonitorMode(config, true);

APRService.Config.Events.SetOnConnect(config, function(service, type)
	if type == APRService.APRS_CONNECTION_TYPE_APRS_IS then
		APRService.Console.WriteLine('Connected via APRS-IS');
	elseif type == APRService.APRS_CONNECTION_TYPE_KISS_TCP then
		APRService.Console.WriteLine('Connected via KISS TNC (TCP)');
	elseif type == APRService.APRS_CONNECTION_TYPE_KISS_SERIAL then
		APRService.Console.WriteLine('Connected via KISS TNC (Serial)');
	end
end);
APRService.Config.Events.SetOnDisconnect(config, function(service, reason)
	if reason == APRService.APRS_DISCONNECT_REASON_USER_REQUESTED then
		APRService.Console.WriteLine('Disconnected');
	elseif reason == APRService.APRS_DISCONNECT_REASON_AUTHENTICATION_FAILED then
		APRService.Console.WriteLine('Authentication failed');
	else
		APRService.Console.WriteLine('Connection closed');
	end
end);

APRService.Config.Events.SetOnSendMessage(config, function(service, station, path, destination, content)
	APRService.Console.WriteLine(string.format('[Message] %s -> %s: %s', station, destination, content));
end);
APRService.Config.Events.SetOnReceiveMessage(config, function(service, station, path, destination, content)
	APRService.Console.WriteLine(string.format('[Message] %s -> %s: %s', station, destination, content));
end);

APRService.Config.Events.SetOnSendPosition(config, function(service, station, path, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
	APRService.Console.WriteLine(string.format('[Position] %s: Latitude = %.06f, Longitude = %.06f, Altitude = %i ft, Comment = %s', station, latitude, longitude, altitude, comment));
end);
APRService.Config.Events.SetOnReceivePosition(config, function(service, station, path, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
	APRService.Console.WriteLine(string.format('[Position] %s: Latitude = %.06f, Longitude = %.06f, Altitude = %i ft, Comment = %s', station, latitude, longitude, altitude, comment));
end);

APRService.Config.Events.SetOnSendTelemetry(config, function(service, station, tocall, path, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
	APRService.Console.WriteLine(string.format('[Telemetry] %s Analog = { %u, %u, %u, %u, %u }, Digital = %u%u%u%u%u%u%u%u', station, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1 and 1 or 0, digital_2 and 1 or 0, digital_3 and 1 or 0, digital_4 and 1 or 0, digital_5 and 1 or 0, digital_6 and 1 or 0, digital_7 and 1 or 0, digital_8 and 1 or 0));
end);
APRService.Config.Events.SetOnReceiveTelemetry(config, function(service, station, tocall, path, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1, digital_2, digital_3, digital_4, digital_5, digital_6, digital_7, digital_8)
	APRService.Console.WriteLine(string.format('[Telemetry] %s Analog = { %u, %u, %u, %u, %u }, Digital = %u%u%u%u%u%u%u%u', station, analog_1, analog_2, analog_3, analog_4, analog_5, digital_1 and 1 or 0, digital_2 and 1 or 0, digital_3 and 1 or 0, digital_4 and 1 or 0, digital_5 and 1 or 0, digital_6 and 1 or 0, digital_7 and 1 or 0, digital_8 and 1 or 0));
end);

local service = APRService.Init(config);

APRService.Commands.Register(service, 'echo', function(service, sender, command_name, command_params)
	APRService.APRS.SendMessage(service, sender, command_params);
end);

APRService.Events.Schedule(service, 5, function(service)
	APRService.Console.WriteLine('Event handler');
end);

if APRService.APRS.IS.Connect(service, aprs_is_config['Host'], aprs_is_config['Port'], aprs_is_config['Passcode']) then
	APRService.Run(service, 5, APRService.FLAGS_STOP_ON_APRS_DISCONNECT);
end

APRService.Deinit(service);
APRService.Config.Deinit(config);
