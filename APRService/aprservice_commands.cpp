#include "aprservice.hpp"

#include <AL/Collections/LinkedList.hpp>

struct aprservice_command_context
{
	AL::String                 name;
	aprservice_command_handler handler;
	void*                      handler_param;
};

typedef AL::Collections::LinkedList<aprservice_command_context> aprservice_command_list;

struct aprservice_commands
{
	aprservice*             service;

	aprservice_command_list commands;
};

aprservice_commands* aprservice_commands_init(aprservice* service)
{
	auto commands = new aprservice_commands
	{
		.service = service
	};

	return commands;
}
void                 aprservice_commands_deinit(aprservice_commands* commands)
{
	delete commands;
}
bool                 aprservice_commands_execute(aprservice_commands* commands, const AL::String& sender, const AL::String& message)
{
	if (message.GetLength() == 0)
		return false;

	auto command_offset = message.IndexOf(' ');
	auto command_name   = message.SubString(0, command_offset); 
	auto command_params = message.SubString(command_offset + 1);

	for (auto& command_context : commands->commands)
	{
		if (command_context.name.Compare(command_name, AL::True))
		{
			command_context.handler(commands->service, sender, command_name, command_params, command_context.handler_param);

			return true;
		}
	}

	return false;
}
void                 aprservice_commands_register(aprservice_commands* commands, const AL::String& name, aprservice_command_handler handler, void* param)
{
	aprservice_command_context command =
	{
		.name          = name,
		.handler       = handler,
		.handler_param = param
	};

	commands->commands.PushBack(AL::Move(command));
}
