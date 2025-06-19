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
#ifndef TIME_CLOCK_H_
#define TIME_CLOCK_H_

#include "sfctypes.h"
#include "ClockListener.h"

namespace sfc {

/**
 * Handles time events.
 */
class Clock {
private:
	sfc::time_t current_time;
	array<ClockListener*> listeners;
public:
	/**
	 *
	 */
	Clock();
	/**
	 * Setup a new clock with given listeners.
	 * @param listeners array of time event listeners.
	 */
	Clock(const array<ClockListener*> &listeners);
	virtual ~Clock();

	void tick(const sfc::time_t &time);

	const sfc::time_t getCurrentTime() const;
	const array<ClockListener*>& getListeners() const;
	void offset(const int & value);
};

} /* namespace sfc */

#endif /* TIME_CLOCK_H_ */
