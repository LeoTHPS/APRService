#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

#include <APRService.hpp>

#define APRS_PATH             { "WIDE1-1" }
#define APRS_STATION          "N0CALL"
#define APRS_SYMBOL_TABLE     '/'
#define APRS_SYMBOL_TABLE_KEY 'l'

#define APRS_BEACON_ENABLED   false
#define APRS_BEACON_COMMENT   "Test"
#define APRS_BEACON_INTERVAL  (5 * 60)
#define APRS_BEACON_ALTITUDE  0
#define APRS_BEACON_LATITUDE  0
#define APRS_BEACON_LONGITUDE 0

#define APRS_IS_HOST          "noam.aprs2.net"
#define APRS_IS_PORT          14580
#define APRS_IS_PASSCODE      -1

#define DECODE_ERROR_LOG      "decode_errors.log"

struct demo_filesystem
{
	std::ofstream decode_error;
};

bool demo_filesystem_init(demo_filesystem* fs)
{
	try
	{
		fs->decode_error.open(DECODE_ERROR_LOG, std::ios::out | std::ios::trunc);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Error opening " DECODE_ERROR_LOG << std::endl;
		std::cerr << exception.what() << std::endl;

		return false;
	}

	return true;
}
bool demo_filesystem_write_decode_error(demo_filesystem* fs, const std::string& raw, const APRService::Exception* exception)
{
	if (!fs->decode_error.is_open())
		return false;

	try
	{
		fs->decode_error << raw << std::endl;

		if (exception)
			fs->decode_error << '\t' << exception->what() << std::endl;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Error writing " DECODE_ERROR_LOG << std::endl;

		return false;
	}

	return true;
}

// @return true to reschedule
bool demo_task_beacon(APRService::Service* service, std::uint32_t& seconds)
{
	service->SendPosition(0, 0, APRS_BEACON_ALTITUDE, APRS_BEACON_LATITUDE, APRS_BEACON_LONGITUDE, APRS_BEACON_COMMENT);

	return true;
}

void demo_dump_packet_fields(APRService::Service* service, const APRService::Packet& packet)
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
void demo_dump_object_fields(APRService::Service* service, const APRService::Object& object)
{
	demo_dump_packet_fields(service, object);

	std::cout << "\tFlags: " << object.Flags << std::endl;
	std::cout << "\tComment: " << object.Comment << std::endl;
	std::cout << "\tSpeed: " << object.Speed << std::endl;
	std::cout << "\tCourse: " << object.Course << std::endl;
	std::cout << "\tLatitude: " << object.Latitude << std::endl;
	std::cout << "\tLongitude: " << object.Longitude << std::endl;
	std::cout << "\tSymbolTable: " << object.SymbolTable << std::endl;
	std::cout << "\tSymbolTableKey: " << object.SymbolTableKey << std::endl;
}
void demo_dump_message_fields(APRService::Service* service, const APRService::Message& message)
{
	demo_dump_packet_fields(service, message);

	std::cout << "\tID: " << message.ID << std::endl;
	std::cout << "\tBody: " << message.Body << std::endl;
	std::cout << "\tDestination: " << message.Destination << std::endl;
}
void demo_dump_weather_fields(APRService::Service* service, const APRService::Weather& weather)
{
	demo_dump_packet_fields(service, weather);

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
void demo_dump_position_fields(APRService::Service* service, const APRService::Position& position)
{
	demo_dump_packet_fields(service, position);

	std::cout << "\tFlags: " << position.Flags << std::endl;
	std::cout << "\tSpeed: " << position.Speed << std::endl;
	std::cout << "\tCourse: " << position.Course << std::endl;
	std::cout << "\tAltitude: " << position.Altitude << std::endl;
	std::cout << "\tLatitude: " << position.Latitude << std::endl;
	std::cout << "\tLongitude: " << position.Longitude << std::endl;
	std::cout << "\tComment: " << position.Comment << std::endl;
	std::cout << "\tSymbolTable: " << position.SymbolTable << std::endl;
	std::cout << "\tSymbolTableKey: " << position.SymbolTableKey << std::endl;
	std::cout << "\tDistance: " << position.CalculateDistance(APRS_BEACON_LATITUDE, APRS_BEACON_LONGITUDE, APRService::DISTANCE_MILES) << " miles" << std::endl;
	std::cout << "\tDistance3D: " << position.CalculateDistance3D(APRS_BEACON_LATITUDE, APRS_BEACON_LONGITUDE, APRS_BEACON_ALTITUDE, APRService::DISTANCE_MILES) << " miles" << std::endl;
}
void demo_dump_telemetry_fields(APRService::Service* service, const APRService::Telemetry& telemetry)
{
	demo_dump_packet_fields(service, telemetry);

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

void demo_service_on_connect(APRService::Service* service)
{
	std::cout << "APRService::OnConnect" << std::endl;
}
void demo_service_on_disconnect(APRService::Service* service)
{
	std::cout << "APRService::OnDisconnect" << std::endl;
}
void demo_service_on_authenticate(APRService::Service* service, const std::string& message)
{
	std::cout << "APRService::OnAuthenticate" << std::endl;
	std::cout << "\tMessage: " << message << std::endl;

#if APRS_BEACON_ENABLED
	if (!service->IsReadOnly())
		if (std::uint32_t seconds = APRS_BEACON_INTERVAL; demo_task_beacon(service, seconds))
			service->ScheduleTask(seconds, std::bind(&demo_task_beacon, service, std::placeholders::_1));
#endif
}
void demo_service_on_decode_error(demo_filesystem* fs, APRService::Service* service, const std::string& raw, const APRService::Exception* exception)
{
	std::cout << "APRService::OnDecodeError" << std::endl;
	std::cout << "\tRaw: " << raw << std::endl;
	std::cout << "\tException: " << (exception ? exception->what() : "") << std::endl;

	demo_filesystem_write_decode_error(fs, raw, exception);
}
void demo_service_on_receive_packet(APRService::Service* service, const APRService::Packet& packet)
{
	std::cout << "APRService::OnReceivePacket" << std::endl;
	demo_dump_packet_fields(service, packet);
}
void demo_service_on_receive_object(APRService::Service* service, const APRService::Object& object)
{
	std::cout << "APRService::OnReceiveObject" << std::endl;
	demo_dump_object_fields(service, object);
}
void demo_service_on_receive_message(APRService::Service* service, const APRService::Message& message)
{
	std::cout << "APRService::OnReceiveMessage" << std::endl;
	demo_dump_message_fields(service, message);
}
void demo_service_on_receive_weather(APRService::Service* service, const APRService::Weather& weather)
{
	std::cout << "APRService::OnReceiveWeather" << std::endl;
	demo_dump_weather_fields(service, weather);
}
void demo_service_on_receive_position(APRService::Service* service, const APRService::Position& position)
{
	std::cout << "APRService::OnReceivePosition" << std::endl;
	demo_dump_position_fields(service, position);
}
void demo_service_on_receive_telemetry(APRService::Service* service, const APRService::Telemetry& telemetry)
{
	std::cout << "APRService::OnReceiveTelemetry" << std::endl;
	demo_dump_telemetry_fields(service, telemetry);
}

int main(int argc, char* argv[])
{
	demo_filesystem fs;
	demo_filesystem_init(&fs);

	try
	{
		APRService::Service service(APRS_STATION, APRS_PATH, APRS_SYMBOL_TABLE, APRS_SYMBOL_TABLE_KEY);

		service.EnableMonitorMode();
		service.EnableCompression();

		service.OnConnect.Register(std::bind(&demo_service_on_connect, &service));
		service.OnDisconnect.Register(std::bind(&demo_service_on_disconnect, &service));
		service.OnAuthenticate.Register(std::bind(&demo_service_on_authenticate, &service, std::placeholders::_1));
		service.OnDecodeError.Register(std::bind(&demo_service_on_decode_error, &fs, &service, std::placeholders::_1, std::placeholders::_2));
		// service.OnReceivePacket.Register(std::bind(&demo_service_on_receive_packet, &service, std::placeholders::_1));
		service.OnReceiveObject.Register(std::bind(&demo_service_on_receive_object, &service, std::placeholders::_1));
		service.OnReceiveMessage.Register(std::bind(&demo_service_on_receive_message, &service, std::placeholders::_1));
		service.OnReceiveWeather.Register(std::bind(&demo_service_on_receive_weather, &service, std::placeholders::_1));
		service.OnReceivePosition.Register(std::bind(&demo_service_on_receive_position, &service, std::placeholders::_1));
		service.OnReceiveTelemetry.Register(std::bind(&demo_service_on_receive_telemetry, &service, std::placeholders::_1));

		service.Connect(APRS_IS_HOST, APRS_IS_PORT, APRS_IS_PASSCODE);
		service.SendObject("KK7EDG", "Test", 0, 0, 43.942917, -122.769840, '/', 'l', true);

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
