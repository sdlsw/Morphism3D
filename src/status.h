#pragma once

#include "event.h"

namespace g3d {
class StatusMessageEvent {
private:
	const std::string* _message;

public:
	// Creates a special event that clears the message.
	static StatusMessageEvent clear();

	// Override time <0 indicates that the default time should be used, as
	// determined by the reciever of this event.
	float overrideTime = -1.0f;

	StatusMessageEvent(const std::string& m, float overrideTime)
	: _message { &m }, overrideTime { overrideTime } {}

	StatusMessageEvent(const std::string& m) : _message { &m } {}

	const auto& message() const { return *_message; }
};
}
