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
#include "StatefulObject.h"

namespace sfc {

void stateful_on_tick(stateful_state_t & state, const sfc::time_t & delta) {
	state.transiting = (state.activated) ^ (state.active);
	state.active = state.activated;
	state.active_time = state.active && state.transiting ? 0 : state.active_time;
	state.active_time += state.active ? delta : 0;
}

StatefulObject::~StatefulObject() {
}

StatefulObject::StatefulObject() {
	this->state = { 0, false, false };
	this->handlers = { NULL, 0 };
	this->listeners = { NULL, 0 };
}
StatefulObject::StatefulObject(array<state_handler_t> const& handlers) {
	this->state = { 0, false, false };
	this->handlers = handlers;
	this->listeners = { NULL, 0 };
}

void StatefulObject::reportState(const stateful_state_t &state) {
	this->stateReported(this->state);

	for(size_t i = 0; i < this->handlers.size; i++) {
		this->reportState(state, ARRAY_GET(this->handlers, i));
	}
	for(size_t i = 0; i < this->listeners.size; i++) {
		this->reportState(state, *ARRAY_GET(this->listeners, i));
	}
}

void StatefulObject::reportState(const stateful_state_t &state, state_handler_t *const& handler) {
	switch (handler->state)
	{
	case STATE_ACTIVATING:
		if(PTR_ACTIVATING(&state)) {
			handler->handler_fnc(state);
		}
		break;
	case STATE_ACTIVE:
		if(state.active) {
			handler->handler_fnc(state);
		}
		break;
	case STATE_DEACTIVATING:
		if(PTR_DEACTIVATING(&state)) {
			handler->handler_fnc(state);
		}
		break;
	case STATE_INACTIVE:
		if(!state.active) {
			handler->handler_fnc(state);
		}
		break;
	case STATE_ALL:
		handler->handler_fnc(state);
		break;
	default: break;
	}
}

void StatefulObject::reportState(const stateful_state_t &state, EventListener *const&listener) {
	listener->onActivationChanged(state);
}

void StatefulObject::onTick(const sfc::time_t &delta) {
	stateful_on_tick(this->state, delta);
	this->reportState(this->state);
}

void StatefulObject::activate() {
	if (!(this->state.activated)) {
		this->state.activated = true;
		this->state.active_time = 0;
	}
}

void StatefulObject::shutdown() {
	this->state.activated = false;
}

void StatefulObject::clear() {
	this->state = { 0, false, false, false };
}

void StatefulObject::stateReported(const stateful_state_t &state) {
}

const array<EventListener*>& StatefulObject::getListeners() const {
	return listeners;
}

void StatefulObject::setListeners(const array<EventListener*> &listeners) {
	this->listeners = listeners;
}

stateful_state_t* StatefulObject::getState() {
	return &(this->state);
}

} /* namespace sfc */
