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
#include "StoredAction.h"

namespace sfc {

StoredAction::StoredAction() :
		Action() {
}

StoredAction::StoredAction(const size_t &step_id) :
		Action(step_id) {
}

StoredAction::StoredAction(const size_t &step_id, activation_predicate_fnc condition) :
		Action(step_id, condition) {
}

StoredAction::StoredAction(const size_t &step_id, array<state_handler_t> handlers) :
		Action(step_id, handlers) {
}

StoredAction::StoredAction(const size_t &step_id, activation_predicate_fnc condition, array<state_handler_t> handlers) :
		Action(step_id, condition, handlers) {
}

StoredAction::~StoredAction() {
}

void StoredAction::clear() {
	Action::clear();
	this->should_activate = false;
}

bool StoredAction::evaluateActivation(const sfc::predicate_state_t &state) {
	if(state.action_state.activated) {
		this->should_activate = false;
	} else if (state.step_state.activated && !state.step_state.active) {
		this->should_activate = true;
	}
	return state.action_state.activated || this->should_activate;
}

} /* namespace sfc */
