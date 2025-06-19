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
#ifndef ACTION_H_
#define ACTION_H_

#include "sfctypes.h"
#include "StatefulObject.h"
#include "StepContext.h"

namespace sfc {

class Action: public StatefulObject {
private:
	size_t step_id;
	activation_predicate_fnc condition;
	stateful_state_t condition_state;
protected:
	virtual bool evaluateActivation(const sfc::predicate_state_t &state);
public:
	Action();
	Action(const size_t &step_id);
	Action(const size_t &step_id, activation_predicate_fnc condition);
	Action(const size_t &step_id, array<state_handler_t> handlers);
	Action(const size_t &step_id, activation_predicate_fnc condition, array<state_handler_t> handlers);
	virtual ~Action();

	virtual void evaluate(StepContext * const& context);
	virtual void onTick(const sfc::time_t &delta);
	virtual void clear();

	const stateful_state_t & getConditionState();
	size_t getStepId();
};

typedef Action * ActionPtr;

} /* namespace sfc */

#endif /* ACTION_H_ */
