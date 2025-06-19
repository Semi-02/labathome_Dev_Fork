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
#ifndef TIME_CLOCKLISTENER_H_
#define TIME_CLOCKLISTENER_H_

#include "sfctypes.h"

namespace sfc {

class ClockListener {
public:
	ClockListener() { }
	virtual ~ClockListener() { }
	/**
	 * Invoked when clock raises a new time event.
	 * @param delta Time elapsed since last time event.
	 */
	virtual void onTick(const sfc::time_t &delta) { }
};

} /* namespace sfc */

#endif /* TIME_CLOCKLISTENER_H_ */
