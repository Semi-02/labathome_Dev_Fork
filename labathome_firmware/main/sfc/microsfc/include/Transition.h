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
#ifndef TRANSITION_H_
#define TRANSITION_H_

#include "sfctypes.h"
#include "EventListener.h"

#include "StepContext.h"

namespace sfc {

class Transition {
private:
	array<int> input_step_ids;
	array<int> output_step_ids;
	predicate_fnc condition;
public:
	Transition();
	Transition(const array<int> &input_step_ids,
			const array<int> &output_step_ids, 
			predicate_fnc condition);
	virtual ~Transition();
	virtual void onActivationChanged(StepContext * const& context);
};

} /* namespace sfc */

#endif /* TRANSITION_H_ */
