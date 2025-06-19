/*
Copyright 2020 Jerson Leonardo Huerfano Romero

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "Clock.h"

namespace sfc {

Clock::Clock(const array<ClockListener*> &listeners) {
	this->listeners = listeners;
	this->current_time = 0;
}

const sfc::time_t Clock::getCurrentTime() const {
	return current_time;
}

void Clock::tick(const sfc::time_t &time) {
	sfc::time_t delta =
			(this->current_time < time) ?
					time - this->current_time :
					(ULONG_MAX - this->current_time + 1) + time;

	this->current_time = time;

	for (size_t i = 0; i < this->listeners.size; i++) {
		ClockListener* listener = *ARRAY_GET(this->listeners, i);
		listener->onTick(delta);
	}
}

Clock::Clock() {
	this->listeners = { NULL, 0 };
	this->current_time = 0;
}

const array<ClockListener*>& Clock::getListeners() const {
	return listeners;
}

Clock::~Clock() {
}

void Clock::offset(const int &value) {
	this->current_time += value;
}

} /* namespace sfc */
