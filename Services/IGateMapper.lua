require('APRService');
require('APRService.Modules.Timer');
require('APRService.Modules.SQLite3');
require('APRService.Modules.TextFile');

local aprs_is_config =
{
	['Host']              = 'noam.aprs2.net',
	['Port']              = 14580,
	['Path']              = 'WIDE1-1',
	['Callsign']          = 'N0CALL',
	['Passcode']          = 0,
	['SymbolTable']       = '/',
	['SymbolTableKey']    = 'k',
	['EnableMonitorMode'] = false
};

local database_config =
{
	['Path']           = './IGateMapper.db',
	['UpdateInterval'] = 10 * 60
};

local IGateMapper = {};

IGateMapper.DB                    = {};
IGateMapper.DB.Gateways           = {}; -- [callsign] = { packet_count, write_pending }
IGateMapper.DB.Stations           = {}; -- [callsign] = { latitude, longitude, altitude, write_pending, is_located }
IGateMapper.DB.GatewayIndex       = {}; -- [callsign] = true
IGateMapper.DB.StationIndex       = {}; -- [callsign] = true
IGateMapper.DB.PendingChangeCount = 0;

-- @return exists, latitude, longitude, altitude, is_located
function IGateMapper.DB.GetStation(callsign)
	if IGateMapper.DB.StationIndex[callsign] then
		local station = IGateMapper.DB.Stations[callsign];

		return true, station[1], station[2], station[3], station[5];
	end

	return false;
end

-- @return exists, packet_count, latitude, longitude, altitude, is_located
function IGateMapper.DB.GetGateway(callsign)
	if IGateMapper.DB.GatewayIndex[callsign] then
		local gateway                                           = IGateMapper.DB.Gateways[callsign];
		local exists, latitude, longitude, altitude, is_located = IGateMapper.DB.GetStation(callsign);

		return true, gateway[1], latitude, longitude, altitude, is_located;
	end

	return false;
end

function IGateMapper.DB.AddStation(callsign)
	if not IGateMapper.DB.StationIndex[callsign] then
		IGateMapper.DB.Stations[callsign]     = { 0, 0, 0, true, false };
		IGateMapper.DB.StationIndex[callsign] = true;
		IGateMapper.DB.PendingChangeCount     = IGateMapper.DB.PendingChangeCount + 1;
	end
end

function IGateMapper.DB.AddGateway(callsign)
	if not IGateMapper.DB.GatewayIndex[callsign] then
		IGateMapper.DB.AddStation(callsign);

		IGateMapper.DB.Gateways[callsign]     = { 0, true };
		IGateMapper.DB.GatewayIndex[callsign] = true;
		IGateMapper.DB.PendingChangeCount     = IGateMapper.DB.PendingChangeCount + 1;
	end
end

-- @param callback(callsign, packet_count, latitude, longitude, altitude, is_located)->boolean
function IGateMapper.DB.EnumerateGateways(callback)
	for callsign, gateway in pairs(IGateMapper.DB.Gateways) do
		local exists, latitude, longitude, altitude, is_located = IGateMapper.DB.GetStation(callsign);

		if not callback(callsign, gateway[1], latitude, longitude, altitude, is_located) then
			break;
		end
	end
end

-- @param callback(callsign, latitude, longitude, altitude, is_located)->boolean
function IGateMapper.DB.EnumerateStations(callback)
	for callsign, station in pairs(IGateMapper.DB.Stations) do
		if not callback(callsign, station[1], station[2], station[3], station[5]) then
			break;
		end
	end
end

function IGateMapper.DB.SetStationLocation(callsign, latitude, longitude, altitude)
	if IGateMapper.DB.StationIndex[callsign] then
		local station                     = IGateMapper.DB.Stations[callsign];
		local station_position_changed    = not station[5] or (station[1] ~= latitude) or (station[2] ~= longitude) or (station[3] ~= altitude);
		IGateMapper.DB.Stations[callsign] = { latitude, longitude, altitude, station_position_changed, true };
		IGateMapper.DB.PendingChangeCount = IGateMapper.DB.PendingChangeCount + 1;
	end
end

function IGateMapper.DB.IncrementGatewayPacketCount(callsign)
	if IGateMapper.DB.GatewayIndex[callsign] then
		local gateway                     = IGateMapper.DB.Gateways[callsign];
		IGateMapper.DB.Gateways[callsign] = { gateway[1] + 1, true };
		IGateMapper.DB.PendingChangeCount = IGateMapper.DB.PendingChangeCount + 1;
	end
end

function IGateMapper.DB.Init()
	local sqlite3_db = APRService.Modules.SQLite3.Database.Open(database_config['Path'], APRService.Modules.SQLite3.FLAG_CREATE | APRService.Modules.SQLite3.FLAG_READ_WRITE);

	if sqlite3_db then
		APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, 'CREATE TABLE IF NOT EXISTS gateways(callsign TEXT PRIMARY KEY UNIQUE, packet_count INTEGER)');
		APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, 'CREATE TABLE IF NOT EXISTS stations(callsign TEXT PRIMARY KEY UNIQUE, latitude REAL, longitude REAL, altitude INTEGER, is_location_set INTEGER)');

		local sqlite3_db_query_result = APRService.Modules.SQLite3.Database.ExecuteQuery(sqlite3_db, string.format('SELECT * FROM gateways'));

		if sqlite3_db_query_result then
			local sqlite3_db_query_result_size = APRService.Modules.SQLite3.QueryResult.GetSize(sqlite3_db_query_result);

			for i = 1, sqlite3_db_query_result_size do
				local sqlite3_db_query_result_row = APRService.Modules.SQLite3.QueryResult.GetRow(sqlite3_db_query_result, i);

				local gateway_callsign     = APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 1);
				local gateway_packet_count = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 2));

				IGateMapper.DB.Gateways[gateway_callsign]     = { gateway_packet_count, false };
				IGateMapper.DB.GatewayIndex[gateway_callsign] = true;
			end

			APRService.Modules.SQLite3.QueryResult.Release(sqlite3_db_query_result);

			sqlite3_db_query_result = APRService.Modules.SQLite3.Database.ExecuteQuery(sqlite3_db, string.format('SELECT * FROM stations'));

			if sqlite3_db_query_result then
				local sqlite3_db_query_result_size = APRService.Modules.SQLite3.QueryResult.GetSize(sqlite3_db_query_result);

				for i = 1, sqlite3_db_query_result_size do
					local sqlite3_db_query_result_row = APRService.Modules.SQLite3.QueryResult.GetRow(sqlite3_db_query_result, i);

					local station_callsign        = APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 1);
					local station_latitude        = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 2));
					local station_longitude       = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 3));
					local station_altitude        = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 4));
					local station_is_location_set = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 5)) ~= 0;

					IGateMapper.DB.Stations[station_callsign]     = { station_latitude, station_longitude, station_altitude, false, station_is_location_set };
					IGateMapper.DB.StationIndex[station_callsign] = true;
				end

				APRService.Modules.SQLite3.QueryResult.Release(sqlite3_db_query_result);
				APRService.Modules.SQLite3.Database.Close(sqlite3_db);

				return true;
			end
		end

		APRService.Modules.SQLite3.Database.Close(sqlite3_db);

		return true;
	end

	return false;
end

function IGateMapper.DB.Export()
	local text_file = APRService.Modules.TextFile.Open(string.format('%s.gateways.kml', database_config['Path']), APRService.Modules.TextFile.OPEN_MODE_WRITE | APRService.Modules.TextFile.OPEN_MODE_TRUNCATE, APRService.Modules.TextFile.LINE_ENDING_LF);

	if text_file then
		APRService.Modules.TextFile.WriteLine(text_file, '<?xml version="1.0" encoding="UTF-8"?>');
		APRService.Modules.TextFile.WriteLine(text_file, '<kml xmlns="http://www.opengis.net/kml/2.2">');
		APRService.Modules.TextFile.WriteLine(text_file, '\t<Document>');

		IGateMapper.DB.EnumerateGateways(function(callsign, packet_count, latitude, longitude, altitude, is_located)
			if is_located then
				APRService.Modules.TextFile.WriteLine(text_file, '\t\t<Placemark>');
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<name>%s</name>', callsign));
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<description>http://aprs.fi/#!call=%s</description>', callsign));
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<Point><coordinates>%f,%f,%i</coordinates></Point>', longitude, latitude, altitude));
				APRService.Modules.TextFile.WriteLine(text_file, '\t\t</Placemark>');
			end

			return true;
		end);

		APRService.Modules.TextFile.WriteLine(text_file, '\t</Document>');
		APRService.Modules.TextFile.WriteLine(text_file, '</kml>');
		APRService.Modules.TextFile.Close(text_file);
	end

	text_file = APRService.Modules.TextFile.Open(string.format('%s.stations.kml', database_config['Path']), APRService.Modules.TextFile.OPEN_MODE_WRITE | APRService.Modules.TextFile.OPEN_MODE_TRUNCATE, APRService.Modules.TextFile.LINE_ENDING_LF);

	if text_file then
		APRService.Modules.TextFile.WriteLine(text_file, '<?xml version="1.0" encoding="UTF-8"?>');
		APRService.Modules.TextFile.WriteLine(text_file, '<kml xmlns="http://www.opengis.net/kml/2.2">');
		APRService.Modules.TextFile.WriteLine(text_file, '\t<Document>');

		IGateMapper.DB.EnumerateStations(function(callsign, latitude, longitude, altitude, is_located)
			if is_located then
				APRService.Modules.TextFile.WriteLine(text_file, '\t\t<Placemark>');
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<name>%s</name>', callsign));
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<description>http://aprs.fi/#!call=%s</description>', callsign));
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<Point><coordinates>%f,%f,%i</coordinates></Point>', longitude, latitude, altitude));
				APRService.Modules.TextFile.WriteLine(text_file, '\t\t</Placemark>');
			end

			return true;
		end);

		APRService.Modules.TextFile.WriteLine(text_file, '\t</Document>');
		APRService.Modules.TextFile.WriteLine(text_file, '</kml>');
		APRService.Modules.TextFile.Close(text_file);
	end
end

function IGateMapper.DB.Update()
	local sqlite3_db = APRService.Modules.SQLite3.Database.Open(database_config['Path'], APRService.Modules.SQLite3.FLAG_READ_WRITE);

	if sqlite3_db then
		for callsign, gateway in pairs(IGateMapper.DB.Gateways) do
			if gateway[2] and APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, string.format("INSERT OR IGNORE INTO gateways VALUES('%s', %u); UPDATE gateways SET packet_count = %u WHERE callsign = '%s'", callsign, gateway[1], gateway[1], callsign)) then
				IGateMapper.DB.Gateways[callsign][2] = false;
				IGateMapper.DB.PendingChangeCount    = IGateMapper.DB.PendingChangeCount - 1;
			end
		end

		for callsign, station in pairs(IGateMapper.DB.Stations) do
			if station[4] and APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, string.format("INSERT OR IGNORE INTO stations VALUES('%s', %f, %f, %i, %u); UPDATE stations SET latitude = %f, longitude = %f, altitude = %i, is_location_set = %u WHERE callsign = '%s'", callsign, station[1], station[2], station[3], station[5] and 1 or 0, station[1], station[2], station[3], station[5] and 1 or 0, callsign)) then
				IGateMapper.DB.Stations[callsign][4] = false;
				IGateMapper.DB.PendingChangeCount    = IGateMapper.DB.PendingChangeCount - 1;
			end
		end

		APRService.Modules.SQLite3.Database.Close(sqlite3_db);
	end
end

IGateMapper.APRS = {};

function IGateMapper.APRS.Connect()
	return APRService.APRS.IS.Connect(IGateMapper.Service, aprs_is_config['Host'], aprs_is_config['Port'], aprs_is_config['Passcode']);
end

function IGateMapper.Init()
	IGateMapper.ServiceConfig = APRService.Config.Init();

	if not IGateMapper.ServiceConfig then
		return false;
	end

	APRService.Config.APRS.SetPath(IGateMapper.ServiceConfig, aprs_is_config['Path']);
	APRService.Config.APRS.SetStation(IGateMapper.ServiceConfig, aprs_is_config['Callsign']);
	APRService.Config.APRS.SetSymbolTable(IGateMapper.ServiceConfig, aprs_is_config['SymbolTable']);
	APRService.Config.APRS.SetSymbolTableKey(IGateMapper.ServiceConfig, aprs_is_config['SymbolTableKey']);
	APRService.Config.APRS.EnableMonitorMode(IGateMapper.ServiceConfig, aprs_is_config['EnableMonitorMode']);

	APRService.Config.Events.SetOnDisconnect(IGateMapper.ServiceConfig, function(service, reason)
		if reason == APRService.APRS.DISCONNECT_REASON_USER_REQUESTED then
			APRService.Stop(service);
		elseif reason == APRService.APRS.DISCONNECT_REASON_AUTHENTICATION_FAILED then
			APRService.Stop(service);
		else
			APRService.Events.Schedule(service, 0, function(service) IGateMapper.Private.Connect(); end);
		end
	end);

	APRService.Config.Events.SetOnReceivePacket(IGateMapper.ServiceConfig, function(service, station, tocall, path, igate, content)
		local station_exists, _ = IGateMapper.DB.GetStation(station);
	
		if not station_exists then
			IGateMapper.DB.AddStation(station);

			APRService.Console.WriteLine(string.format('Identified station %s', station));
		end

		if string.len(igate) ~= 0 then
			local gateway_exists, _ = IGateMapper.DB.GetGateway(igate);

			if not gateway_exists then
				IGateMapper.DB.AddGateway(igate);

				APRService.Console.WriteLine(string.format('Identified gateway %s', igate));
			end

			IGateMapper.DB.IncrementGatewayPacketCount(igate);
		end
	end);

	APRService.Config.Events.SetOnReceivePosition(IGateMapper.ServiceConfig, function(service, station, path, igate, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
		local station_exists, station_packet_count, station_latitude, station_longitude, station_altitude, station_is_located = IGateMapper.DB.GetGateway(station);

		IGateMapper.DB.SetStationLocation(station, latitude, longitude, altitude);

		if not station_exists then
			station_exists, station_latitude, station_longitude, station_altitude, station_is_located = IGateMapper.DB.GetStation(station);

			if station_exists and not station_is_located then
				APRService.Console.WriteLine(string.format('Discovered station %s', station));
			end
		elseif not station_is_located then
			APRService.Console.WriteLine(string.format('Discovered gateway %s', station));
		end
	end);

	IGateMapper.Service = APRService.Init(IGateMapper.ServiceConfig);

	if not IGateMapper.Service then
		APRService.Config.Deinit(IGateMapper.ServiceConfig);

		return false;
	end

	if not IGateMapper.DB.Init() then
		APRService.Deinit(IGateMapper.Service);
		APRService.Config.Deinit(IGateMapper.ServiceConfig);

		return false;
	end

	IGateMapper.Private.Connect();

	return true;
end

function IGateMapper.Deinit()
	APRService.Deinit(IGateMapper.Service);
	APRService.Config.Deinit(IGateMapper.ServiceConfig);
end

function IGateMapper.Run(tick_rate)
	APRService.Events.Schedule(IGateMapper.Service, database_config['UpdateInterval'], function(service) IGateMapper.Private.UpdateDB(); end);
	APRService.Run(IGateMapper.Service, tick_rate, APRService.FLAG_NONE);
end

IGateMapper.Private = {};

function IGateMapper.Private.Connect()
	if not IGateMapper.APRS.Connect() then
		APRService.Console.WriteLine('Connection failed. Trying again in 3 seconds.');
		APRService.Events.Schedule(IGateMapper.Service, 3, IGateMapper.Private.Connect);
	end
end

function IGateMapper.Private.UpdateDB()
	local timer = APRService.Modules.Timer.Create();

	APRService.Console.WriteLine('Updating database');

	APRService.Console.WriteLine(string.format('Flushing %u pending changes to database', IGateMapper.DB.PendingChangeCount));
	IGateMapper.DB.Update();

	APRService.Console.WriteLine('Exporting database to KML');
	IGateMapper.DB.Export();

	APRService.Console.WriteLine(string.format('Updated database in %.2f seconds', APRService.Modules.Timer.GetElapsedMS(timer) / 1000));
	APRService.Modules.Timer.Destroy(timer);

	APRService.Events.Schedule(IGateMapper.Service, database_config['UpdateInterval'], function(service) IGateMapper.Private.UpdateDB(); end);
end

if IGateMapper.Init() then
	IGateMapper.Run(5);
	IGateMapper.Deinit();
end
