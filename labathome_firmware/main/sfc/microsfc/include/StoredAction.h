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
#ifndef STOREDACTION_H_
#define STOREDACTION_H_

#include "Action.h"

namespace sfc {

class StoredAction: public Action {
private:
	bool should_activate = false;
public:
	StoredAction();
	StoredAction(const size_t &step_id);
	StoredAction(const size_t &step_id, activation_predicate_fnc condition);
	StoredAction(const size_t &step_id, array<state_handler_t> handlers);
	StoredAction(const size_t &step_id, activation_predicate_fnc condition, array<state_handler_t> handlers);
	virtual ~StoredAction();

	virtual void clear();

protected:
	virtual bool evaluateActivation(const sfc::predicate_state_t &state) override;
};

} /* namespace sfc */

#endif /* STOREDACTION_H_ */
