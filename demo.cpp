#include <iostream>

#include <APRService.hpp>

#define APRS_PATH             { "WIDE1-1" }
#define APRS_STATION          "N0CALL"
#define APRS_SYMBOL_TABLE     '/'
#define APRS_SYMBOL_TABLE_KEY 'l'

#define APRS_BEACON_ENABLED   true
#define APRS_BEACON_COMMENT   "Test"
#define APRS_BEACON_INTERVAL  (5 * 60)
#define APRS_BEACON_ALTITUDE  0
#define APRS_BEACON_LATITUDE  0
#define APRS_BEACON_LONGITUDE 0

#define APRS_IS_HOST          "noam.aprs2.net"
#define APRS_IS_PORT          14580
#define APRS_IS_PASSCODE      -1

APRService::Service* demo_service;

// @return true to reschedule
bool demo_task_beacon(std::uint32_t& seconds)
{
	demo_service->SendPosition(0, 0, APRS_BEACON_ALTITUDE, APRS_BEACON_LATITUDE, APRS_BEACON_LONGITUDE, APRS_BEACON_COMMENT);

	return true;
}

void demo_dump_packet_fields(const APRService::Packet& packet)
{
	std::cout << "\tType: " << (int)packet.Type << std::endl;
	std::cout << "\tPath: " << packet.Path[0];
	for (std::size_t i = 1; i < packet.Path.size(); ++i)
	{
		if (!packet.Path[i].length())
			break;

		std::cout << ", " << packet.Path[i];
	}
	std::cout << std::endl;
	std::cout << "\tIGate: " << packet.IGate << std::endl;
	std::cout << "\tToCall: " << packet.ToCall << std::endl;
	std::cout << "\tSender: " << packet.Sender << std::endl;
	std::cout << "\tContent: " << packet.Content << std::endl;
	std::cout << "\tQConstruct: " << packet.QConstruct << std::endl;
}
void demo_dump_message_fields(const APRService::Message& message)
{
	demo_dump_packet_fields(message);

	std::cout << "\tID: " << message.ID << std::endl;
	std::cout << "\tBody: " << message.Body << std::endl;
	std::cout << "\tDestination: " << message.Destination << std::endl;
}
void demo_dump_weather_fields(const APRService::Weather& weather)
{
	demo_dump_packet_fields(weather);

	std::cout << "\tTime: " << mktime(const_cast<tm*>(&weather.Time)) << std::endl;
	std::cout << "\tWindSpeed: " << weather.WindSpeed << std::endl;
	std::cout << "\tWindSpeedGust: " << weather.WindSpeedGust << std::endl;
	std::cout << "\tWindDirection: " << weather.WindDirection << std::endl;
	std::cout << "\tRainfallLastHour: " << weather.RainfallLastHour << std::endl;
	std::cout << "\tRainfallLast24Hours: " << weather.RainfallLast24Hours << std::endl;
	std::cout << "\tRainfallSinceMidnight: " << weather.RainfallSinceMidnight << std::endl;
	std::cout << "\tHumidity: " << weather.Humidity << std::endl;
	std::cout << "\tTemperature: " << weather.Temperature << std::endl;
	std::cout << "\tBarometricPressure: " << weather.BarometricPressure << std::endl;
}
void demo_dump_position_fields(const APRService::Position& position)
{
	demo_dump_packet_fields(position);

	std::cout << "\tFlags: " << position.Flags << std::endl;
	std::cout << "\tSpeed: " << position.Speed << std::endl;
	std::cout << "\tCourse: " << position.Course << std::endl;
	std::cout << "\tAltitude: " << position.Altitude << std::endl;
	std::cout << "\tLatitude: " << position.Latitude << std::endl;
	std::cout << "\tLongitude: " << position.Longitude << std::endl;
	std::cout << "\tComment: " << position.Comment << std::endl;
	std::cout << "\tSymbolTable: " << position.SymbolTable << std::endl;
	std::cout << "\tSymbolTableKey: " << position.SymbolTableKey << std::endl;
	std::cout << "\tDistance: " << position.CalculateDistance(APRS_BEACON_LATITUDE, APRS_BEACON_LONGITUDE) << std::endl;
	std::cout << "\tDistance3D: " << position.CalculateDistance3D(APRS_BEACON_LATITUDE, APRS_BEACON_LONGITUDE, APRS_BEACON_ALTITUDE) << std::endl;
}
void demo_dump_telemetry_fields(const APRService::Telemetry& telemetry)
{
	demo_dump_packet_fields(telemetry);

	std::cout << "\tAnalog: " << telemetry.Analog[0];
	for (std::size_t i = 1; i < telemetry.Analog.size(); ++i)
		std::cout << ", " << telemetry.Analog[i];
	std::cout << std::endl;
	std::cout << "\tDigital: ";
	for (std::size_t i = 0; i < 8; ++i)
		std::cout << ((telemetry.Digital >> i) & 1);
	std::cout << std::endl;
	std::cout << "\tSequence: " << telemetry.Sequence << std::endl;
}

void demo_service_on_connect()
{
	std::cout << "APRService::OnConnect" << std::endl;
}
void demo_service_on_disconnect()
{
	std::cout << "APRService::OnDisconnect" << std::endl;
}
void demo_service_on_authenticate(const std::string& message)
{
	std::cout << "APRService::OnAuthenticate" << std::endl;
	std::cout << "\tMessage: " << message << std::endl;

#if APRS_BEACON_ENABLED
	if (!demo_service->IsReadOnly())
		if (std::uint32_t seconds = APRS_BEACON_INTERVAL; demo_task_beacon(seconds))
			demo_service->ScheduleTask(seconds, &demo_task_beacon);
#endif
}
void demo_service_on_decode_error(const std::string& raw, const APRService::Exception& exception)
{
	std::cout << "APRService::OnDecodeError" << std::endl;
	std::cout << "\tRaw: " << raw << std::endl;
	std::cout << "\tException: " << exception.what() << std::endl;
}
void demo_service_on_receive_packet(const APRService::Packet& packet)
{
	std::cout << "APRService::OnReceivePacket" << std::endl;
	demo_dump_packet_fields(packet);
}
void demo_service_on_receive_message(const APRService::Message& message)
{
	std::cout << "APRService::OnReceiveMessage" << std::endl;
	demo_dump_message_fields(message);
}
void demo_service_on_receive_weather(const APRService::Weather& weather)
{
	std::cout << "APRService::OnReceiveWeather" << std::endl;
	demo_dump_weather_fields(weather);
}
void demo_service_on_receive_position(const APRService::Position& position)
{
	std::cout << "APRService::OnReceivePosition" << std::endl;
	demo_dump_position_fields(position);
}
void demo_service_on_receive_telemetry(const APRService::Telemetry& telemetry)
{
	std::cout << "APRService::OnReceiveTelemetry" << std::endl;
	demo_dump_telemetry_fields(telemetry);
}

int main(int argc, char* argv[])
{
	try
	{
		APRService::Service service(APRS_STATION, APRS_PATH, APRS_SYMBOL_TABLE, APRS_SYMBOL_TABLE_KEY);
		demo_service = &service;

		service.EnableMonitorMode();
		service.EnableCompression();

		service.OnConnect.Register(&demo_service_on_connect);
		service.OnDisconnect.Register(&demo_service_on_disconnect);
		service.OnAuthenticate.Register(&demo_service_on_authenticate);
		service.OnDecodeError.Register(&demo_service_on_decode_error);
		// service.OnReceivePacket.Register(&demo_service_on_receive_packet);
		service.OnReceiveMessage.Register(&demo_service_on_receive_message);
		service.OnReceiveWeather.Register(&demo_service_on_receive_weather);
		service.OnReceivePosition.Register(&demo_service_on_receive_position);
		service.OnReceiveTelemetry.Register(&demo_service_on_receive_telemetry);

		service.Connect(APRS_IS_HOST, APRS_IS_PORT, APRS_IS_PASSCODE);

		while (service.Update())
#if defined(APRSERVICE_UNIX)
			sleep(10);
#elif defined(APRSERVICE_WIN32)
			Sleep(10);
#endif
	}
	catch (const APRService::Exception& exception)
	{
		std::cerr << exception.what() << std::endl;
	}

	return 0;
}
