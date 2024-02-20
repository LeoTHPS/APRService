require('APRService');
require('APRService.Modules.SQLite3');

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

local IGateMapper_Stations     = {}; -- [station] = true
local IGateMapper_Positions    = {}; -- [station] = { latitude, longitude, written_to_disk }
local IGateMapper_StationCount = 0;

local function IGateMapper_Connect_APRS_IS(service)
	if not APRService.APRS.IS.Connect(service, aprs_is_config['Host'], aprs_is_config['Port'], aprs_is_config['Passcode']) then
		APRService.Events.Schedule(service, 3, IGateMapper_Connect_APRS_IS);
		APRService.Console.WriteLine('Connection failed. Trying again in 3 seconds.');
	end
end

local function IGateMapper_InitDisk(service)
	local sqlite3_db = APRService.Modules.SQLite3.Database.Open(database_config['Path'], APRService.Modules.SQLite3.FLAG_CREATE | APRService.Modules.SQLite3.FLAG_READ_WRITE);

	if sqlite3_db ~= nil then
		APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, 'CREATE TABLE IF NOT EXISTS gateways(station TEXT PRIMARY KEY UNIQUE, latitude REAL, longitude REAL)');
		APRService.Modules.SQLite3.Database.Close(sqlite3_db);
	end
end

local function IGateMapper_ReadStationsFromDisk(service)
	local sqlite3_db = APRService.Modules.SQLite3.Database.Open(database_config['Path'], APRService.Modules.SQLite3.FLAG_READ_ONLY);

	if sqlite3_db ~= nil then
		local sqlite3_db_query_result = APRService.Modules.SQLite3.Database.ExecuteQuery(sqlite3_db, string.format('SELECT * FROM gateways'));

		if sqlite3_db_query_result ~= nil then
			local sqlite3_db_query_result_size = APRService.Modules.SQLite3.QueryResult.GetSize(sqlite3_db_query_result);

			for i = 1, sqlite3_db_query_result_size do
				local sqlite3_db_query_result_row = APRService.Modules.SQLite3.QueryResult.GetRow(sqlite3_db_query_result, i);

				local station           = APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 1);
				local station_latitude  = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 2));
				local station_longitude = tonumber(APRService.Modules.SQLite3.QueryResult.Row.GetValue(sqlite3_db_query_result_row, 3));

				IGateMapper_Stations[station]  = true;
				IGateMapper_Positions[station] = { station_latitude, station_longitude, true };
				IGateMapper_StationCount       = IGateMapper_StationCount + 1;
			end

			APRService.Modules.SQLite3.QueryResult.Release(sqlite3_db_query_result);
		end

		APRService.Modules.SQLite3.Database.Close(sqlite3_db);
	end
end

local function IGateMapper_WriteStationsToDisk(service)
	local sqlite3_db = APRService.Modules.SQLite3.Database.Open(database_config['Path'], APRService.Modules.SQLite3.FLAG_READ_WRITE);

	if sqlite3_db ~= nil then
		for station, station_info in pairs(IGateMapper_Positions) do
			if not station_info[3] and APRService.Modules.SQLite3.Database.ExecuteNonQuery(sqlite3_db, string.format("INSERT OR IGNORE INTO gateways VALUES('%s', %f, %f); UPDATE gateways SET latitude = %f, longitude = %f WHERE station = '%s'", station, station_info[1], station_info[2], station_info[1], station_info[2], station)) then
				IGateMapper_Positions[station][3] = true;
			end
		end

		APRService.Modules.SQLite3.Database.Close(sqlite3_db);
	end

	APRService.Events.Schedule(service, database_config['UpdateInterval'], IGateMapper_WriteStationsToDisk);
end

local function IGateMapper_ExportStationsToKML(service)
	local file = io.open('IGateMapper.kml', "w");

	if file ~= nil then
		file:write('<?xml version="1.0" encoding="UTF-8"?>');
		file:write('<kml xmlns="http://www.opengis.net/kml/2.2">');
		file:write('<Document>');

		for station, station_info in pairs(IGateMapper_Positions) do
			file:write('<Placemark>');
			file:write(string.format('<name>%s</name>', station));
			file:write(string.format('<description>http://aprs.fi/#!call=%s</description>', station));
			file:write(string.format('<Point><coordinates>%f,%f,0</coordinates></Point>', station_info[2], station_info[1]));
			file:write('</Placemark>');
		end

		file:write('</Document>');
		file:write('</kml>');
		file:close();
	end
end

APRService.Config.Events.SetOnDisconnect(config, function(service, reason)
	if reason == APRService.APRS.DISCONNECT_REASON_USER_REQUESTED then
		APRService.Stop(service);
	elseif reason == APRService.APRS.DISCONNECT_REASON_AUTHENTICATION_FAILED then
		APRService.Stop(service);
	else
		APRService.Events.Schedule(service, 0, IGateMapper_Connect_APRS_IS);
	end
end);

APRService.Config.Events.SetOnReceivePacket(config, function(service, station, tocall, path, igate, content)
	if string.len(igate) ~= 0 then
		IGateMapper_Stations[igate] = true;
	end
end);

APRService.Config.Events.SetOnReceivePosition(config, function(service, station, path, igate, altitude, latitude, longitude, symbol_table, symbol_table_key, comment, flags)
	if IGateMapper_Stations[station] then
		local position = IGateMapper_Positions[station];

		if not position then
			IGateMapper_StationCount = IGateMapper_StationCount + 1;
			APRService.Console.WriteLine(string.format('Discovered IGate #%u [Station: %s, Location: %.6f, %.6f]', IGateMapper_StationCount, station, latitude, longitude));
		end

		IGateMapper_Positions[station] = { latitude, longitude, (position and (latitude == IGateMapper_Positions[station][1]) and (longitude == IGateMapper_Positions[station][2])) };
	end
end);

local service = APRService.Init(config);

IGateMapper_InitDisk(service);
IGateMapper_ReadStationsFromDisk(service);
IGateMapper_ExportStationsToKML(service);

APRService.Events.Schedule(service, 0,                                 IGateMapper_Connect_APRS_IS);
APRService.Events.Schedule(service, database_config['UpdateInterval'], IGateMapper_WriteStationsToDisk);
APRService.Run(service, 5, APRService.FLAG_NONE);

APRService.Deinit(service);
APRService.Config.Deinit(config);
