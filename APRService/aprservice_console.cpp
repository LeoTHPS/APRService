#include "aprservice.hpp"

#include <AL/OS/Console.hpp>

bool aprservice_console_set_title(const AL::String& value)
{
	return AL::OS::Console::SetTitle(value);
}
bool aprservice_console_read(char& value)
{
	return AL::OS::Console::Read(value);
}
bool aprservice_console_read_line(AL::String& value)
{
	return AL::OS::Console::ReadLine(value);
}
bool aprservice_console_write(const AL::String& value)
{
	return AL::OS::Console::Write(value);
}
bool aprservice_console_write_line(const AL::String& value)
{
	return AL::OS::Console::WriteLine(value);
}
bool aprservice_console_write_exception(const AL::Exception& value)
{
	return AL::OS::Console::WriteException(value);
}
