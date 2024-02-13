#include "aprservice.hpp"

#include <AL/Collections/Queue.hpp>
#include <AL/Collections/Dictionary.hpp>

struct aprservice_event_context
{
	void*                    param;
	aprservice_event_handler handler;
};

typedef AL::Collections::Queue<aprservice_event_context>                  aprservice_event_queue;
typedef AL::Collections::Dictionary<AL::TimeSpan, aprservice_event_queue> aprservice_event_collection;

struct aprservice_events
{
	aprservice*                 service;

	aprservice_event_collection events;
	AL::TimeSpan                events_time;
};

aprservice_events* aprservice_events_init(aprservice* service)
{
	auto events = new aprservice_events
	{
		.service = service
	};

	return events;
}
void               aprservice_events_deinit(aprservice_events* events)
{
	delete events;
}
AL::uint32         aprservice_events_get_count(aprservice_events* events)
{
	return static_cast<AL::uint32>(events->events.GetSize());
}
void               aprservice_events_clear(aprservice_events* events)
{
	events->events.Clear();
}
bool               aprservice_events_update(aprservice_events* events, AL::TimeSpan delta)
{
	events->events_time += delta;

	for (auto it = events->events.begin(); it != events->events.end(); )
	{
		if (it->Key <= events->events_time)
		{
			aprservice_event_context event;

			while (it->Value.Dequeue(event))
				event.handler(events->service, event.param);

			events->events.Erase(it++);

			continue;
		}

		++it;
	}

	return true;
}
void               aprservice_events_schedule(aprservice_events* events, AL::TimeSpan delay, aprservice_event_handler handler, void* param)
{
	events->events[events->events_time + delay].Enqueue({ .param = param, .handler = handler });
}
