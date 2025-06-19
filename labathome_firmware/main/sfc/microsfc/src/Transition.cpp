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
#include "Transition.h"

namespace sfc {

Transition::Transition(const array<int> &input_step_ids, 
		const array<int> &output_step_ids,
		predicate_fnc condition) {
	this->input_step_ids = input_step_ids;
	this->output_step_ids = output_step_ids;
	this->condition = condition;
}

Transition::~Transition() {
}

Transition::Transition() {
	this->input_step_ids = { NULL, 0 };
	this->output_step_ids = { NULL, 0 };
	this->condition = NULL;
}

void Transition::onActivationChanged(StepContext * const& context) {
	bool step_precondition = true;

	for (size_t i = 0; i < this->input_step_ids.size; i++) {
		stateful_state_t step_state = context->getStepState(
				*(this->input_step_ids.ptr + i));
		step_precondition = step_precondition & step_state.active;
	}

	if (step_precondition && this->condition()) {
		for (size_t i = 0; i < this->input_step_ids.size; i++) {
			context->toggleStepState(*(this->input_step_ids.ptr + i),
					false);
		}
		for (size_t i = 0; i < this->output_step_ids.size; i++) {
			context->toggleStepState(*(this->output_step_ids.ptr + i),
					true);
		}
	}
}

} /* namespace sfc */

