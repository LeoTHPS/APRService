#include <stdio.h>

#include <APRService.h>

#if defined(APRSERVICE_UNIX)
	#include <time.h>
#elif defined(APRSERVICE_WIN32)
	#include <Windows.h>
#endif

struct demo
{
	FILE*              file;
	struct aprs_path*  path;
	struct aprservice* service;
};

void    demo_task_beacon(struct aprservice* service, struct aprservice_task_information* task, void* param)
{
	struct demo* d = (struct demo*)param;

	task->seconds    = 10;
	task->reschedule = true;

	if (aprservice_is_authenticated(service) && aprservice_send_position(service))
		task->seconds = APRS_STATION_BEACON;
}

void    demo_event_handler(struct aprservice* service, struct aprservice_event_information* event, void* param)
{
	struct demo* d = (struct demo*)param;

	switch (event->type)
	{
		case APRSERVICE_EVENT_CONNECT:
		{
			struct aprservice_event_information_connect* connect = (struct aprservice_event_information_connect*)event;

			printf("Connected\n");
		}
		break;

		case APRSERVICE_EVENT_DISCONNECT:
		{
			struct aprservice_event_information_disconnect* disconnect = (struct aprservice_event_information_disconnect*)event;

			printf("Disconnected\n");
		}
		break;

		case APRSERVICE_EVENT_AUTHENTICATE:
		{
			struct aprservice_event_information_authenticate* authenticate = (struct aprservice_event_information_authenticate*)event;

			printf("Authentication complete [Success: %s, Verified: %s] %s\n", authenticate->success ? "True" : "False", authenticate->verified ? "True" : "False", authenticate->message);
		}
		break;

		case APRSERVICE_EVENT_RECEIVE_PACKET:
		{
			struct aprs_packet* packet = ((struct aprservice_event_information_receive_packet*)event)->packet;

			const char* sender = aprs_packet_get_sender(packet);

			switch (aprs_packet_get_type(packet))
			{
				case APRS_PACKET_TYPE_GPS:
				{
					const char* nmea    = aprs_packet_gps_get_nmea(packet);
					const char* comment = aprs_packet_gps_get_comment(packet);

					printf("[Packet] [GPS] [From: %s] %s %s\n", sender, nmea, comment);
				}
				break;

				case APRS_PACKET_TYPE_RAW:
				{
					const char* string  = aprs_packet_to_string(packet);
					const char* content = aprs_packet_get_content(packet);

					printf("[Packet] [From: %s] %s\n", sender, content);

					fwrite(string, 1, strlen(string), d->file);
					fwrite("\n", 1, 1, d->file);
					fflush(d->file);
				}
				break;

				case APRS_PACKET_TYPE_ITEM:
				{
					const char* item_name      = aprs_packet_item_get_name(packet);
					const char* item_comment   = aprs_packet_item_get_comment(packet);
					float       item_latitude  = aprs_packet_item_get_latitude(packet);
					float       item_longitude = aprs_packet_item_get_longitude(packet);

					printf("[Packet] [Item] [From: %s] [Name: %s] [Lat: %f] [Long: %f] %s\n", sender, item_name, item_latitude, item_longitude, item_comment);
				}
				break;

				case APRS_PACKET_TYPE_TEST:
				{
					const char* content = aprs_packet_get_content(packet);

					printf("[Packet] [Test] [From: %s] %s\n", sender, content);
				}
				break;

				case APRS_PACKET_TYPE_QUERY:
				{
					const char* content = aprs_packet_get_content(packet);

					printf("[Packet] [Query] [From: %s] %s\n", sender, content);
				}
				break;

				case APRS_PACKET_TYPE_OBJECT:
				{
					const char*             object_name      = aprs_packet_object_get_name(packet);
					const struct aprs_time* object_time      = aprs_packet_object_get_time(packet);
					const char*             object_comment   = aprs_packet_object_get_comment(packet);
					float                   object_latitude  = aprs_packet_object_get_latitude(packet);
					float                   object_longitude = aprs_packet_object_get_longitude(packet);

					printf("[Packet] [Object] ");

					if (object_time)
					{
						uint8_t object_time_value[3];
						memset(object_time_value, 0, 3);

						if (aprs_time_get_dhm(object_time, &object_time_value[0], &object_time_value[1], &object_time_value[2]))
							printf("[Time.DMS: %u:%02u:%02u] ", object_time_value[0], object_time_value[1], object_time_value[2]);
						else if (aprs_time_get_hms(object_time, &object_time_value[0], &object_time_value[1], &object_time_value[2]))
							printf("[Time.HMS: %u:%02u:%02u] ", object_time_value[0], object_time_value[1], object_time_value[2]);
					}

					printf("[From: %s] [Name: %s] [Lat: %f] [Long: %f] %s\n", sender, object_name, object_latitude, object_longitude, object_comment);
				}
				break;

				case APRS_PACKET_TYPE_STATUS:
				{
					const struct aprs_time* status_time    = aprs_packet_status_get_time(packet);
					const char*             status_message = aprs_packet_status_get_message(packet);

					printf("[Packet] [Status] ");

					if (status_time)
					{
						uint8_t status_time_value[3];

						if (aprs_time_get_dhm(status_time, &status_time_value[0], &status_time_value[1], &status_time_value[2]))
							printf("[Time.DMS: %u:%02u:%02u] ", status_time_value[0], status_time_value[1], status_time_value[2]);
						else if (aprs_time_get_hms(status_time, &status_time_value[0], &status_time_value[1], &status_time_value[2]))
							printf("[Time.HMS: %u:%02u:%02u] ", status_time_value[0], status_time_value[1], status_time_value[2]);
					}

					printf("[From: %s] %s\n", sender, status_message);
				}
				break;

				case APRS_PACKET_TYPE_MESSAGE:
				{
					enum APRS_MESSAGE_TYPES message_type        = aprs_packet_message_get_type(packet);
					const char*             message_content     = aprs_packet_message_get_content(packet);
					const char*             message_destination = aprs_packet_message_get_destination(packet);

					switch (message_type)
					{
						case APRS_MESSAGE_TYPE_MESSAGE:
							printf("[Packet] [Message] [From: %s] [To: %s] %s\n", sender, message_destination, message_content);
							break;

						case APRS_MESSAGE_TYPE_BULLETIN:
							printf("[Packet] [Bulletin] [From: %s] [To: %s] %s\n", sender, message_destination, message_content);
							break;
					}
				}
				break;

				case APRS_PACKET_TYPE_WEATHER:
				{
					const struct aprs_time* weather_time                    = aprs_packet_weather_get_time(packet);
					const char*             weather_type                    = aprs_packet_weather_get_type(packet);
					char                    weather_software                = aprs_packet_weather_get_software(packet);
					uint16_t                weather_wind_speed              = aprs_packet_weather_get_wind_speed(packet);
					uint16_t                weather_wind_speed_gust         = aprs_packet_weather_get_wind_speed_gust(packet);
					uint16_t                weather_wind_direction          = aprs_packet_weather_get_wind_direction(packet);
					uint16_t                weather_rainfall_last_hour      = aprs_packet_weather_get_rainfall_last_hour(packet);
					uint16_t                weather_rainfall_last_24_hours  = aprs_packet_weather_get_rainfall_last_24_hours(packet);
					uint16_t                weather_rainfall_since_midnight = aprs_packet_weather_get_rainfall_since_midnight(packet);
					uint8_t                 weather_humidity                = aprs_packet_weather_get_humidity(packet);
					int16_t                 weather_temperature             = aprs_packet_weather_get_temperature(packet);
					uint32_t                weather_barometric_pressure     = aprs_packet_weather_get_barometric_pressure(packet);

					printf("[Packet] [Weather] ");

					if (weather_time)
					{
						uint8_t weather_time_value[4];

						if (aprs_time_get_mdhm(weather_time, &weather_time_value[0], &weather_time_value[1], &weather_time_value[2], &weather_time_value[3]))
						{
							printf("[Date: %02u:%02u] ", weather_time_value[0], weather_time_value[1]);
							printf("[Time: %02u:%02u] ", weather_time_value[2], weather_time_value[3]);
						}
					}

					printf("[From: %s] [Wind: %u/%u/%u] [Rainfall: %u/%u/%u] [Humidity: %u] [Temperature: %i] [Barometric Pressure: %lu]\n",
							sender,
							weather_wind_speed, weather_wind_speed_gust, weather_wind_direction,
							weather_rainfall_last_hour, weather_rainfall_last_24_hours, weather_rainfall_since_midnight,
							weather_humidity,
							weather_temperature,
							weather_rainfall_last_24_hours);
				}
				break;

				case APRS_PACKET_TYPE_POSITION:
				{
					int         position_flags         = aprs_packet_position_get_flags(packet);
					const char* position_comment       = aprs_packet_position_get_comment(packet);
					float       position_latitude      = aprs_packet_position_get_latitude(packet);
					float       position_longitude     = aprs_packet_position_get_longitude(packet);
					int         position_mic_e_message = aprs_packet_position_get_mic_e_message(packet);

					printf("[Packet] [Position] ");

					if (position_flags & APRS_POSITION_FLAG_TIME)
					{
						const struct aprs_time* position_time = aprs_packet_position_get_time(packet);
						uint8_t                 position_time_value[3];

						memset(position_time_value, 0, 3);

						if (aprs_time_get_dhm(position_time, &position_time_value[0], &position_time_value[1], &position_time_value[2]))
							printf("[Time.DMS: %u:%02u:%02u] ", position_time_value[0], position_time_value[1], position_time_value[2]);
						else if (aprs_time_get_hms(position_time, &position_time_value[0], &position_time_value[1], &position_time_value[2]))
							printf("[Time.HMS: %u:%02u:%02u] ", position_time_value[0], position_time_value[1], position_time_value[2]);
					}

					if (position_flags & APRS_POSITION_FLAG_MIC_E)
						printf("[Mic-E: %s] ", aprs_mic_e_message_to_string((enum APRS_MIC_E_MESSAGES)position_mic_e_message));

					printf("[From: %s] [Lat: %f] [Long: %f] %s\n", sender, position_latitude, position_longitude, position_comment);
				}
				break;

				case APRS_PACKET_TYPE_TELEMETRY:
				{
					printf("[Packet] [Telemetry] [From: %s] ", sender);

					switch (aprs_packet_telemetry_get_type(packet))
					{
						case APRS_TELEMETRY_TYPE_U8:
						case APRS_TELEMETRY_TYPE_FLOAT:
							printf("[Seq: %u] ", aprs_packet_telemetry_get_sequence(packet));
							break;
					}

					switch (aprs_packet_telemetry_get_type(packet))
					{
						case APRS_TELEMETRY_TYPE_U8:
						{
							const uint8_t** telementry_analog = aprs_packet_telemetry_get_analog(packet);

							printf("[Analog: ");

							if (*telementry_analog)
							{
								printf("%u", **telementry_analog);

								while (*(++telementry_analog))
									printf(", %u", **telementry_analog);
							}

							printf("] ");
						}
						break;

						case APRS_TELEMETRY_TYPE_FLOAT:
						{
							const float** telementry_analog = aprs_packet_telemetry_get_analog_float(packet);

							printf("[Analog: ");

							if (*telementry_analog)
							{
								printf("%f", **telementry_analog);

								while (*(++telementry_analog))
									printf(", %f", **telementry_analog);
							}

							printf("] ");
						}
						break;

						case APRS_TELEMETRY_TYPE_PARAMS:
						{
							const char** telementry_params = aprs_packet_telemetry_get_params(packet);

							printf("[Params: ");

							if (*telementry_params)
							{
								printf(*telementry_params);

								while (*(++telementry_params))
									printf(", %s", *telementry_params);
							}

							printf("] ");
						}
						break;

						case APRS_TELEMETRY_TYPE_UNITS:
						{
							const char** telementry_units = aprs_packet_telemetry_get_units(packet);

							printf("[Units: ");

							if (*telementry_units)
							{
								printf(*telementry_units);

								while (*(++telementry_units))
									printf(", %s", *telementry_units);
							}

							printf("] ");
						}
						break;

						case APRS_TELEMETRY_TYPE_EQNS:
						{
							const struct aprs_telemetry_eqn** telementry_equations = aprs_packet_telemetry_get_eqns(packet);

							printf("[Equations: ");

							if (*telementry_equations)
							{
								printf("[%f, %f, %f]", (*telementry_equations)->a, (*telementry_equations)->b, (*telementry_equations)->c);

								while (*(++telementry_equations))
									printf(", [%f, %f, %f]", (*telementry_equations)->a, (*telementry_equations)->b, (*telementry_equations)->c);
							}

							printf("] ");
						}
						break;

						case APRS_TELEMETRY_TYPE_BITS:
							printf("[Bits: ");
							for (uint8_t i = 0, b = aprs_packet_telemetry_get_bits(packet); i < 8; ++i)
								printf("%u", ((b & (1 << i)) == (1 << i)) ? 1 : 0);
							printf("] ");
							break;
					}

					switch (aprs_packet_telemetry_get_type(packet))
					{
						case APRS_TELEMETRY_TYPE_U8:
						case APRS_TELEMETRY_TYPE_FLOAT:
							printf("[Digital: ");
							for (uint8_t i = 0, d = aprs_packet_telemetry_get_digital(packet); i < 8; ++i)
								printf("%u", ((d & (1 << i)) == (1 << i)) ? 1 : 0);
							printf("] ");
							break;
					}

					const char* telemetry_comment = aprs_packet_telemetry_get_comment(packet);

					if (telemetry_comment)
						printf(telemetry_comment);
					printf("\n");
				}
				break;

				case APRS_PACKET_TYPE_MAP_FEATURE:
				{
					const char* content = aprs_packet_get_content(packet);

					printf("[Packet] [Map Feature] [From: %s] %s\n", sender, content);
				}
				break;

				case APRS_PACKET_TYPE_GRID_BEACON:
				{
					const char* content = aprs_packet_get_content(packet);

					printf("[Packet] [Grid Beacon] [From: %s] %s\n", sender, content);
				}
				break;

				case APRS_PACKET_TYPE_THIRD_PARTY:
				{
					const char* third_party_content = aprs_packet_third_party_get_content(packet);

					printf("[Packet] [Third Party] [From: %s] %s\n", sender, third_party_content);
				}
				break;

				case APRS_PACKET_TYPE_MICROFINDER:
				{
					const char* content = aprs_packet_get_content(packet);

					printf("[Packet] [Microfinder] [From: %s] %s\n", sender, content);
				}
				break;

				case APRS_PACKET_TYPE_USER_DEFINED:
				{
					char        user_defined_id   = aprs_packet_user_defined_get_id(packet);
					char        user_defined_type = aprs_packet_user_defined_get_type(packet);
					const char* user_defined_data = aprs_packet_user_defined_get_data(packet);

					printf("[Packet] [User Defined] [From: %s] [ID: %c] [Type: %c] %s\n", sender, user_defined_id, user_defined_type, user_defined_data);
				}
				break;

				case APRS_PACKET_TYPE_SHELTER_TIME:
				{
					const char* content = aprs_packet_get_content(packet);

					printf("[Packet] [Shelter Time] [From: %s] %s\n", sender, content);
				}
				break;

				case APRS_PACKET_TYPE_STATION_CAPABILITIES:
				{
					const char* content = aprs_packet_get_content(packet);

					printf("[Packet] [Station Capabilities] [From: %s] %s\n", sender, content);
				}
				break;

				case APRS_PACKET_TYPE_MAIDENHEAD_GRID_BEACON:
				{
					const char* content = aprs_packet_get_content(packet);

					printf("[Packet] [Maidenhead Grid Beacon] [From: %s] %s\n", sender, content);
				}
				break;
			}
		}
		break;

		case APRSERVICE_EVENT_RECEIVE_MESSAGE:
		{
			struct aprservice_event_information_receive_message* message = (struct aprservice_event_information_receive_message*)event;

			printf("[Message] [From: %s] [To: %s] %s\n", message->sender, message->destination, message->content);
		}
		break;

		case APRSERVICE_EVENT_RECEIVE_SERVER_MESSAGE:
		{
			struct aprservice_event_information_receive_server_message* server_message = (struct aprservice_event_information_receive_server_message*)event;

			printf("[Server] [Message] %s\n", server_message->content);
		}
		break;
	}
}

bool    demo_init(struct demo* d)
{
	if ((d->file = fopen("demo.log", "w+")) == NULL)
	{
		printf("Error opening demo.log\n");

		return false;
	}

	if (!(d->path = aprs_path_init_from_string(APRS_STATION_PATH)))
	{
		printf("Error initializing path \"" APRS_STATION_PATH "\"\n");

		fclose(d->file);

		return false;
	}

	if (!(d->service = aprservice_init(APRS_STATION, d->path, APRS_STATION_SYMBOL_TABLE, APRS_STATION_SYMBOL_TABLE_KEY)))
	{
		printf("Error initializing service\n");

		aprs_path_deinit(d->path);
		fclose(d->file);

		return false;
	}

	if (!aprservice_set_comment(d->service, APRS_STATION_COMMENT))
	{
		printf("Error setting station comment\n");

		aprservice_deinit(d->service);
		aprs_path_deinit(d->path);
		fclose(d->file);

		return false;
	}

	if (!aprservice_set_position_type(d->service, APRS_STATION_POSITION_TYPE))
	{
		printf("Error setting station position type\n");

		aprservice_deinit(d->service);
		aprs_path_deinit(d->path);
		fclose(d->file);

		return false;
	}

	if (!aprservice_set_position(d->service, APRS_STATION_LATITUDE, APRS_STATION_LONGITUDE, APRS_STATION_ALTITUDE, APRS_STATION_SPEED, APRS_STATION_COURSE))
	{
		printf("Error setting station position\n");

		aprservice_deinit(d->service);
		aprs_path_deinit(d->path);
		fclose(d->file);

		return false;
	}

	aprservice_enable_monitoring(d->service, true);

	aprservice_set_connection_timeout(d->service, 2 * 60);
	aprservice_set_default_event_handler(d->service, &demo_event_handler, d);

#if APRS_STATION_BEACON
	aprservice_task_schedule(d->service, 10, &demo_task_beacon, d);
#endif

	return true;
}
void    demo_deinit(struct demo* d)
{
	aprservice_deinit(d->service);
	aprs_path_deinit(d->path);
	fclose(d->file);
}

#define demo_connect                       demo_connect_aprs_is
#define demo_connect_aprs_is(demo)         aprservice_connect_aprs_is(demo->service, APRS_IS_HOST, APRS_IS_PORT, APRS_IS_PASSCODE)
#define demo_connect_kiss_tnc_tcp(demo)    aprservice_connect_kiss_tnc_tcp(demo->service, APRS_KISS_TNC_TCP_HOST, APRS_KISS_TNC_TCP_PORT)
#define demo_connect_kiss_tnc_serial(demo) aprservice_connect_kiss_tnc_serial(demo->service, APRS_KISS_TNC_SERIAL_DEVICE, APRS_KISS_TNC_SERIAL_SPEED)

void    demo_run(struct demo* d)
{
	while (aprservice_poll(d->service))
		if (aprservice_is_connected(d->service) || demo_connect(d))
			aprservice_wait_for_io(d->service);
}

int main(int argc, char* argv[])
{
	struct demo d;

	if (demo_init(&d))
	{
		demo_run(&d);
		demo_deinit(&d);
	}

	return 0;
}
