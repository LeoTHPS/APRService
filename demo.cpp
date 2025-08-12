#include <iomanip>
#include <iostream>

#include <windows.h>

#include <APRService.hpp>

#define APRS_IS_HOST                  "noam.aprs2.net"
#define APRS_IS_PORT                  14580
#define APRS_IS_PASSCODE              0

#define APRS_STATION                  "N0CALL"
#define APRS_STATION_PATH             "WIDE1-1"
#define APRS_STATION_BEACON           (0 * 60) /* interval in seconds or 0 to disable */
#define APRS_STATION_COMMENT          "Test"
#define APRS_STATION_ALTITUDE         0
#define APRS_STATION_LATITUDE         0
#define APRS_STATION_LONGITUDE        0
#define APRS_STATION_SYMBOL_TABLE     '/'
#define APRS_STATION_SYMBOL_TABLE_KEY 'l'

struct demo
{
	aprs_path*  path;
	aprservice* service;
};

void  demo_on_authenticate(demo* d, bool success, bool verified, const char* message)
{
	if (!success)
		std::cout << "[Auth] Authentication failed: " << message << std::endl;
	else if (verified)
		std::cout << "[Auth] Authentication success: " << message << std::endl;
	else
		std::cout << "[Auth] Authentication success (unverified): " << message << std::endl;
}
void  demo_on_receive_packet(demo* d, aprs_packet* packet)
{
	auto sender = aprs_packet_get_sender(packet);

	switch (aprs_packet_get_type(packet))
	{
		case APRS_PACKET_TYPE_RAW:
		{
			auto content = aprs_packet_get_content(packet);

			std::cout << "[Packet] [From: " << sender << "] " << content << std::endl;
		}
		break;

		case APRS_PACKET_TYPE_OBJECT:
		{
			auto object_name      = aprs_packet_object_get_name(packet);
			auto object_comment   = aprs_packet_object_get_comment(packet);
			auto object_latitude  = aprs_packet_object_get_latitude(packet);
			auto object_longitude = aprs_packet_object_get_longitude(packet);

			std::cout << "[Packet] [Object] ";

			if (auto object_time = aprs_packet_object_get_time(packet))
			{
				uint8_t object_time_value[3] = {};

				if (aprs_time_get_dms(object_time, &object_time_value[0], &object_time_value[1], &object_time_value[2]))
				{
					std::cout << "[Time.DMS: " << (int)object_time_value[0] << ':';
					std::cout << std::setfill('0') << std::setw(2) << (int)object_time_value[1] << ':';
					std::cout << std::setfill('0') << std::setw(2) << (int)object_time_value[2] << "] ";
				}
				else if (aprs_time_get_hms(object_time, &object_time_value[0], &object_time_value[1], &object_time_value[2]))
				{
					std::cout << "[Time.HMS: " << (int)object_time_value[0] << ':';
					std::cout << std::setfill('0') << std::setw(2) << (int)object_time_value[1] << ':';
					std::cout << std::setfill('0') << std::setw(2) << (int)object_time_value[2] << "] ";
				}
			}

			std::cout << "[From: " << sender << "] [Name: " << object_name << "] [Lat: " << object_latitude << "] [Long: " << object_longitude << "] " << object_comment << std::endl;
		}
		break;

		case APRS_PACKET_TYPE_MESSAGE:
		{
			auto message_type        = aprs_packet_message_get_type(packet);
			auto message_content     = aprs_packet_message_get_content(packet);
			auto message_destination = aprs_packet_message_get_destination(packet);

			switch (message_type)
			{
				case APRS_MESSAGE_TYPE_MESSAGE:
					std::cout << "[Packet] [Message] [From: " << sender << "] [To: " << message_destination << "] " << message_content << std::endl;
					break;

				case APRS_MESSAGE_TYPE_BULLETIN:
					std::cout << "[Packet] [Bulletin] [From: " << sender << "] [To: " << message_destination << "] " << message_content << std::endl;
					break;
			}
		}
		break;

		case APRS_PACKET_TYPE_WEATHER:
		{
			auto weather_time                    = aprs_packet_weather_get_time(packet);
			auto weather_type                    = aprs_packet_weather_get_type(packet);
			auto weather_wind_speed              = aprs_packet_weather_get_wind_speed(packet);
			auto weather_wind_speed_gust         = aprs_packet_weather_get_wind_speed_gust(packet);
			auto weather_wind_direction          = aprs_packet_weather_get_wind_direction(packet);
			auto weather_rainfall_last_hour      = aprs_packet_weather_get_rainfall_last_hour(packet);
			auto weather_rainfall_last_24_hours  = aprs_packet_weather_get_rainfall_last_24_hours(packet);
			auto weather_rainfall_since_midnight = aprs_packet_weather_get_rainfall_since_midnight(packet);
			auto weather_humidity                = aprs_packet_weather_get_humidity(packet);
			auto weather_temperature             = aprs_packet_weather_get_temperature(packet);
			auto weather_barometric_pressure     = aprs_packet_weather_get_barometric_pressure(packet);

			std::cout << "[Packet] [Weather] ";

			if (auto weather_time = aprs_packet_object_get_time(packet))
			{
				uint8_t weather_time_value[4];

				if (aprs_time_get_mdhm(weather_time, &weather_time_value[0], &weather_time_value[1], &weather_time_value[2], &weather_time_value[3]))
				{
					std::cout << "[Date: ";
					std::cout << std::setfill('0') << std::setw(2) << (int)weather_time_value[0] << '/';
					std::cout << std::setfill('0') << std::setw(2) << (int)weather_time_value[1] << "] ";

					std::cout << "[Time: ";
					std::cout << std::setfill('0') << std::setw(2) << (int)weather_time_value[2] << ':';
					std::cout << std::setfill('0') << std::setw(2) << (int)weather_time_value[3] << "] ";
				}
			}

			std::cout << "[From: " << sender << "] " << std::endl;
		}
		break;

		case APRS_PACKET_TYPE_POSITION:
		{
			auto position_flags     = aprs_packet_position_get_flags(packet);
			auto position_comment   = aprs_packet_position_get_comment(packet);
			auto position_latitude  = aprs_packet_position_get_latitude(packet);
			auto position_longitude = aprs_packet_position_get_longitude(packet);

			std::cout << "[Packet] [Position] ";

			if (position_flags & APRS_POSITION_FLAG_TIME)
			{
				auto    position_time          = aprs_packet_position_get_time(packet);
				uint8_t position_time_value[3] = {};

				if (aprs_time_get_dms(position_time, &position_time_value[0], &position_time_value[1], &position_time_value[2]))
				{
					std::cout << "[Time.DMS: " << (int)position_time_value[0] << ':';
					std::cout << std::setfill('0') << std::setw(2) << (int)position_time_value[1] << ':';
					std::cout << std::setfill('0') << std::setw(2) << (int)position_time_value[2] << "] ";
				}
				else if (aprs_time_get_hms(position_time, &position_time_value[0], &position_time_value[1], &position_time_value[2]))
				{
					std::cout << "[Time.HMS: " << (int)position_time_value[0] << ':';
					std::cout << std::setfill('0') << std::setw(2) << (int)position_time_value[1] << ':';
					std::cout << std::setfill('0') << std::setw(2) << (int)position_time_value[2] << "] ";
				}
			}

			if (position_flags & APRS_POSITION_FLAG_MIC_E)
				std::cout << "[Mic-E] ";

			std::cout << "[From: " << sender << "] [Lat: " << position_latitude << "] [Long: " << position_longitude << "] " << position_comment << std::endl;
		}
		break;

		case APRS_PACKET_TYPE_TELEMETRY:
		{
			auto telemetry_digital  = aprs_packet_telemetry_get_digital(packet);
			auto telemetry_sequence = aprs_packet_telemetry_get_sequence(packet);

			std::cout << "[Packet] [Telemetry] [From: " << sender << "] [Seq: " << telemetry_sequence << "] ";

			switch (aprs_packet_telemetry_get_type(packet))
			{
				case APRS_TELEMETRY_TYPE_U8:
				{
					uint8_t telemetry_analog[] =
					{
						aprs_packet_telemetry_get_analog(packet, 0),
						aprs_packet_telemetry_get_analog(packet, 1),
						aprs_packet_telemetry_get_analog(packet, 2),
						aprs_packet_telemetry_get_analog(packet, 3),
						aprs_packet_telemetry_get_analog(packet, 4)
					};

					std::cout << "[Analog: " << (int)telemetry_analog[0] << ", " << (int)telemetry_analog[1] << ", " << (int)telemetry_analog[2] << ", " << (int)telemetry_analog[3] << ", " << (int)telemetry_analog[4] << "] ";
				}
				break;

				case APRS_TELEMETRY_TYPE_FLOAT:
				{
					float telemetry_analog[] =
					{
						aprs_packet_telemetry_get_analog_float(packet, 0),
						aprs_packet_telemetry_get_analog_float(packet, 1),
						aprs_packet_telemetry_get_analog_float(packet, 2),
						aprs_packet_telemetry_get_analog_float(packet, 3),
						aprs_packet_telemetry_get_analog_float(packet, 4)
					};

					std::cout << "[Analog: " << telemetry_analog[0] << ", " << telemetry_analog[1] << ", " << telemetry_analog[2] << ", " << telemetry_analog[3] << ", " << telemetry_analog[4] << "] ";
				}
				break;
			}

			std::cout << "[Digital: ";
			for (uint8_t i = 0; i < 8; ++i)
				std::cout << (((telemetry_digital & (1 << i)) == (1 << i)) ? 1 : 0);
			std::cout << ']' << std::endl;
		}
		break;
	}
}
void  demo_on_receive_message(demo* d, const char* sender, const char* message)
{
	std::cout << "[Message] [From: " << sender << "] " << message << std::endl;
}
void  demo_on_receive_server_message(demo* d, const char* message)
{
	std::cout << "[Server] [Message] " << message << std::endl;
}

void  demo_task_beacon(aprservice* service, aprservice_task_information* task, void* param)
{
	auto d = (demo*)param;

	task->seconds    = 10;
	task->reschedule = true;

	if (aprservice_is_authenticated(service) && !aprservice_send_position(service))
		task->seconds = APRS_STATION_BEACON;
}

void  demo_event_handler(aprservice* service, aprservice_event_information* event, void* param)
{
	auto d = (demo*)param;

	switch (event->type)
	{
		case APRSERVICE_EVENT_AUTHENTICATE:
			if (auto auth = (aprservice_event_information_authenticate*)event)
				demo_on_authenticate(d, auth->success, auth->verified, auth->message);
			break;

		case APRSERVICE_EVENT_SNIFF_MESSAGE:
			if (auto message = (aprservice_event_information_sniff_message*)event)
				;
			break;

		case APRSERVICE_EVENT_RECEIVE_PACKET:
			if (auto packet = ((aprservice_event_information_receive_packet*)event)->packet)
				demo_on_receive_packet(d, packet);
			break;

		case APRSERVICE_EVENT_RECEIVE_MESSAGE:
			if (auto message = (aprservice_event_information_receive_message*)event)
				demo_on_receive_message(d, message->sender, message->content);
			break;

		case APRSERVICE_EVENT_RECEIVE_SERVER_MESSAGE:
			if (auto message = ((aprservice_event_information_receive_server_message*)event)->message)
				demo_on_receive_server_message(d, message);
			break;
	}
}

demo* demo_init()
{
	auto d = new demo
	{
	};

	if (!(d->path = aprs_path_init_from_string(APRS_STATION_PATH)))
	{
		std::cerr << "aprs_path_init_from_string returned nullptr" << std::endl;

		return nullptr;
	}

	if (auto error = aprservice_init(&d->service, APRS_STATION, d->path, APRS_STATION_SYMBOL_TABLE, APRS_STATION_SYMBOL_TABLE_KEY))
	{
		std::cerr << "aprservice_init returned " << error << ": " << aprservice_error_to_string(error) << std::endl;

		aprs_path_deinit(d->path);

		delete d;

		return nullptr;
	}

	if (auto error = aprservice_set_comment(d->service, APRS_STATION_COMMENT))
	{
		std::cerr << "aprservice_set_comment returned " << error << ": " << aprservice_error_to_string(error) << std::endl;

		aprservice_deinit(d->service);
		aprs_path_deinit(d->path);

		delete d;

		return nullptr;
	}

	if (auto error = aprservice_set_position(d->service, APRS_STATION_LATITUDE, APRS_STATION_LONGITUDE, APRS_STATION_ALTITUDE, 0, 0))
	{
		std::cerr << "aprservice_set_position returned " << error << ": " << aprservice_error_to_string(error) << std::endl;

		aprservice_deinit(d->service);
		aprs_path_deinit(d->path);

		delete d;

		return nullptr;
	}

	aprservice_set_default_event_handler(d->service, &demo_event_handler, d);

#if APRS_STATION_BEACON
	aprservice_task_schedule(d->service, 10, &demo_task_beacon, d);
#endif

	return d;
}
void  demo_deinit(demo* d)
{
	aprservice_deinit(d->service);
	aprs_path_deinit(d->path);

	delete d;
}
bool  demo_update(demo* d)
{
	if (auto error = aprservice_poll(d->service))
	{
		std::cerr << "aprservice_poll returned " << error << ": " << aprservice_error_to_string(error) << std::endl;

		if (error != APRSERVICE_ERROR_DISCONNECTED)
			return false;

		if (error = aprservice_connect(d->service, APRS_IS_HOST, APRS_IS_PORT, APRS_IS_PASSCODE))
		{
			std::cerr << "aprservice_connect returned " << error << ": " << aprservice_error_to_string(error) << std::endl;

			return false;
		}
	}

	return true;
}

int main(int argc, char* argv[])
{
	if (auto demo = demo_init())
	{
		while (demo_update(demo))
			Sleep(10);

		demo_deinit(demo);
	}

	return 0;
}
