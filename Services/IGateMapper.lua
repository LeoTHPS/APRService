require('APRService');
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
	['UpdateInterval'] = 600
};

local config = APRService.Config.Init();

APRService.Config.APRS.SetPath(config, aprs_is_config['Path']);
APRService.Config.APRS.SetStation(config, aprs_is_config['Callsign']);
APRService.Config.APRS.SetSymbolTable(config, aprs_is_config['SymbolTable']);
APRService.Config.APRS.SetSymbolTableKey(config, aprs_is_config['SymbolTableKey']);
APRService.Config.APRS.EnableMonitorMode(config, aprs_is_config['EnableMonitorMode']);

local IGateMapper_DB_Gateways     = {}; -- [callsign] = { packet_count, write_pending }
local IGateMapper_DB_GatewayCount = 0;

local IGateMapper_DB_Stations     = {}; -- [callsign] = { latitude, longitude, altitude, write_pending }
local IGateMapper_DB_StationCount = 0;

local function IGateMapper_DB_Init()
	local sqlite3_db = APRService.Modules.SQLite3.Database.Open(database_config['Path'], APRService.Modules.SQLite3.FLAG_CREATE | APRService.Modules.SQLite3.FLAG_READ_WRITE);

	if sqlite3_db ~= nil then
		APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, 'CREATE TABLE IF NOT EXISTS gateways(callsign TEXT PRIMARY KEY UNIQUE, packet_count INTEGER)');
		APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, 'CREATE TABLE IF NOT EXISTS stations(callsign TEXT PRIMARY KEY UNIQUE, latitude REAL, longitude REAL, altitude INTEGER)');

		local sqlite3_db_query_result = APRService.Modules.SQLite3.Database.ExecuteQuery(sqlite3_db, string.format('SELECT * FROM gateways'));

		if sqlite3_db_query_result ~= nil then
			local sqlite3_db_query_result_size = APRService.Modules.SQLite3.QueryResult.GetSize(sqlite3_db_query_result);

			for i = 1, sqlite3_db_query_result_size do
				local sqlite3_db_query_result_row = APRService.Modules.SQLite3.QueryResult.GetRow(sqlite3_db_query_result, i);

				local gateway_callsign     = APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 1);
				local gateway_packet_count = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 2));

				IGateMapper_DB_Gateways[gateway_callsign] = { gateway_packet_count, false };
				IGateMapper_DB_GatewayCount               = IGateMapper_DB_GatewayCount + 1;
			end

			APRService.Modules.SQLite3.QueryResult.Release(sqlite3_db_query_result);
		end

		sqlite3_db_query_result = APRService.Modules.SQLite3.Database.ExecuteQuery(sqlite3_db, string.format('SELECT * FROM stations'));

		if sqlite3_db_query_result ~= nil then
			local sqlite3_db_query_result_size = APRService.Modules.SQLite3.QueryResult.GetSize(sqlite3_db_query_result);

			for i = 1, sqlite3_db_query_result_size do
				local sqlite3_db_query_result_row = APRService.Modules.SQLite3.QueryResult.GetRow(sqlite3_db_query_result, i);

				local station_callsign  = APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 1);
				local station_latitude  = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 2));
				local station_longitude = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 3));
				local station_altitude  = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 4));

				IGateMapper_DB_Stations[station_callsign] = { station_latitude, station_longitude, station_altitude, false };
				IGateMapper_DB_StationCount               = IGateMapper_DB_StationCount + 1;
			end

			APRService.Modules.SQLite3.QueryResult.Release(sqlite3_db_query_result);
		end

		APRService.Modules.SQLite3.Database.Close(sqlite3_db);
	end
end

local function IGateMapper_DB_Export()
	local text_file = APRService.Modules.TextFile.Open(string.format('%s.stations.kml', database_config['Path']), APRService.Modules.TextFile.OPEN_MODE_WRITE | APRService.Modules.TextFile.OPEN_MODE_TRUNCATE, APRService.Modules.TextFile.LINE_ENDING_LF);

	if text_file then
		APRService.Modules.TextFile.WriteLine(text_file, '<?xml version="1.0" encoding="UTF-8"?>');
		APRService.Modules.TextFile.WriteLine(text_file, '<kml xmlns="http://www.opengis.net/kml/2.2">');
		APRService.Modules.TextFile.WriteLine(text_file, '\t<Document>');

		for callsign, station in pairs(IGateMapper_DB_Stations) do
			APRService.Modules.TextFile.WriteLine(text_file, '\t\t<Placemark>');
			APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<name>%s</name>', callsign));
			APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<description>http://aprs.fi/#!call=%s</description>', callsign));
			APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<Point><coordinates>%f,%f,%i</coordinates></Point>', station[2], station[1], station[3]));
			APRService.Modules.TextFile.WriteLine(text_file, '\t\t</Placemark>');
		end

		APRService.Modules.TextFile.WriteLine(text_file, '\t</Document>');
		APRService.Modules.TextFile.WriteLine(text_file, '</kml>');
		APRService.Modules.TextFile.Close(text_file);
	end

	text_file = APRService.Modules.TextFile.Open(string.format('%s.gateways.kml', database_config['Path']), APRService.Modules.TextFile.OPEN_MODE_WRITE | APRService.Modules.TextFile.OPEN_MODE_TRUNCATE, APRService.Modules.TextFile.LINE_ENDING_LF);

	if text_file then
		APRService.Modules.TextFile.WriteLine(text_file, '<?xml version="1.0" encoding="UTF-8"?>');
		APRService.Modules.TextFile.WriteLine(text_file, '<kml xmlns="http://www.opengis.net/kml/2.2">');
		APRService.Modules.TextFile.WriteLine(text_file, '\t<Document>');

		for callsign, gateway in pairs(IGateMapper_DB_Gateways) do
			local station = IGateMapper_DB_Stations[callsign];

			if station then
				APRService.Modules.TextFile.WriteLine(text_file, '\t\t<Placemark>');
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<name>%s</name>', callsign));
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<description>http://aprs.fi/#!call=%s</description>', callsign));
				APRService.Modules.TextFile.WriteLine(text_file, string.format('\t\t\t<Point><coordinates>%f,%f,%i</coordinates></Point>', station[2], station[1], station[3]));
				APRService.Modules.TextFile.WriteLine(text_file, '\t\t</Placemark>');
			end
		end

		APRService.Modules.TextFile.WriteLine(text_file, '\t</Document>');
		APRService.Modules.TextFile.WriteLine(text_file, '</kml>');
		APRService.Modules.TextFile.Close(text_file);
	end
end

local function IGateMapper_DB_Update()
	local sqlite3_db = APRService.Modules.SQLite3.Database.Open(database_config['Path'], APRService.Modules.SQLite3.FLAG_READ_WRITE);

	if sqlite3_db ~= nil then
		for callsign, gateway in pairs(IGateMapper_DB_Gateways) do
			if gateway[2] and APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, string.format("INSERT OR IGNORE INTO gateways VALUES('%s', %u); UPDATE gateways SET packet_count = %u WHERE callsign = '%s'", callsign, gateway[1], gateway[1], callsign)) then
				IGateMapper_DB_Gateways[callsign][2] = false;
			end
		end

		for callsign, station in pairs(IGateMapper_DB_Stations) do
			if station[4] and APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, string.format("INSERT OR IGNORE INTO stations VALUES('%s', %f, %f, %i); UPDATE stations SET latitude = %f, longitude = %f, altitude = %i WHERE callsign = '%s'", callsign, station[1], station[2], station[3], station[1], station[2], station[3], callsign)) then
				IGateMapper_DB_Stations[callsign][4] = false;
			end
		end

		APRService.Modules.SQLite3.Database.Close(sqlite3_db);
	end
end

local function IGateMapper_APRS_Connect(service)
	return APRService.APRS.IS.Connect(service, aprs_is_config['Host'], aprs_is_config['Port'], aprs_is_config['Passcode']);
end

local function IGateMapper_Connect(service)
	if not IGateMapper_APRS_Connect(service) then
		APRService.Console.WriteLine('Connection failed. Trying again in 3 seconds.');
		APRService.Events.Schedule(service, 3, IGateMapper_Connect);
	end
end

local function IGateMapper_Update(service)
	IGateMapper_DB_Update();
	IGateMapper_DB_Export();

	APRService.Events.Schedule(service, database_config['UpdateInterval'], IGateMapper_Update);
end

APRService.Config.Events.SetOnDisconnect(config, function(service, reason)
	if reason == APRService.APRS.DISCONNECT_REASON_USER_REQUESTED then
		APRService.Stop(service);
	elseif reason == APRService.APRS.DISCONNECT_REASON_AUTHENTICATION_FAILED then
		APRService.Stop(service);
	else
		APRService.Events.Schedule(service, 0, IGateMapper_Connect);
	end
end);

APRService.Config.Events.SetOnReceivePacket(config, function(service, station, tocall, path, igate, content)
	if not IGateMapper_DB_Stations[station] then
		IGateMapper_DB_Stations[station] = { 0, 0, 0, true };
		IGateMapper_DB_StationCount      = IGateMapper_DB_StationCount + 1;

		APRService.Console.WriteLine(string.format('Identified station #%u: %s', IGateMapper_DB_StationCount, station));
	end

	if string.len(igate) ~= 0 then
		if not IGateMapper_DB_Gateways[igate] then
			IGateMapper_DB_Gateways[igate] = { 0, true };
			IGateMapper_DB_GatewayCount    = IGateMapper_DB_GatewayCount + 1;

			APRService.Console.WriteLine(string.format('Identified gateway #%u: %s', IGateMapper_DB_GatewayCount, igate));
		end

		IGateMapper_DB_Gateways[igate][1] = IGateMapper_DB_Gateways[igate][1] + 1;
	end
end);

APRService.Config.Events.SetOnReceivePosition(config, function(service, station, path, igate, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
	local station_info = IGateMapper_DB_Stations[station];

	if not station_info and IGateMapper_DB_Gateways[station] then
		APRService.Console.WriteLine(string.format('Discovered gateway %s [Latitude: %f, Longitude: %f, Altitude: %i]', station, latitude, longitude, altitude));
	end

	IGateMapper_DB_Stations[station] = { latitude, longitude, altitude, not (station_info and (latitude == station_info[1]) and (longitude == station_info[2]) and (altitude == station_info[3])) };
end);

local service = APRService.Init(config);

IGateMapper_DB_Init();
IGateMapper_DB_Export();

IGateMapper_Connect(service);

APRService.Events.Schedule(service, database_config['UpdateInterval'], IGateMapper_Update);
APRService.Run(service, 4, APRService.FLAG_NONE);

APRService.Deinit(service);
APRService.Config.Deinit(config);
