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
#ifndef STATEFULOBJECT_H_
#define STATEFULOBJECT_H_

#include "ClockListener.h"
#include "EventListener.h"

namespace sfc {

void stateful_on_tick(stateful_state_t & state, const sfc::time_t & delta);

class StatefulObject: public ClockListener {
private:
	stateful_state_t state;
	array<state_handler_t> handlers;
	array<EventListener*> listeners;

	void reportState(const stateful_state_t &state);
	void reportState(const stateful_state_t &state, state_handler_t *const& handler);
	void reportState(const stateful_state_t &state, EventListener *const& listener);
protected:
	virtual void stateReported(const stateful_state_t &state);
public:
	StatefulObject();
	StatefulObject(array<state_handler_t> const& handlers);
	virtual ~StatefulObject();

	virtual void onTick(const sfc::time_t &delta);
	virtual void activate();
	virtual void shutdown();
	virtual void clear();

	const array<EventListener*>& getListeners() const;
	void setListeners(const array<EventListener*> &listeners);
	stateful_state_t* getState();
};

} /* namespace sfc */

#endif /* STATEFULOBJECT_H_ */
