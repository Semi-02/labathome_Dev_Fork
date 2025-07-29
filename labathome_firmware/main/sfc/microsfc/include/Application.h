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
#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "sfctypes.h"
#include "StatefulObject.h"
#include "Step.h"
#include "Action.h"
#include "Transition.h"
#include "StepContext.h"

namespace sfc {

typedef struct {
	array<Step> steps;
	array<Action*> actions;
	array<Transition> transitions;
} component_context_t;

class Application : public StatefulObject, public StepContext {
private:
	component_context_t container_context;
	bool evaluate;
	sfc::time_t component_delta;
protected:
	virtual void stateReported(const stateful_state_t &state);

	virtual void evaluateStates(const sfc::time_t &delta);
	virtual void performComponentTick(const sfc::time_t &delta);

	virtual void evaluateTransitions();
	virtual void evaluateActions();
public:
	Application();
	Application(const component_context_t & container_context);
	virtual ~Application();
	const virtual sfc::stateful_state_t& getStepState(const int &id);
	virtual size_t getStepCount();
	virtual bool isEntryPoint(const int &id);
	virtual void toggleStepState(const int &id, const bool &active);
	virtual void onTick(const sfc::time_t &delta);
	virtual void clear();
	
	virtual component_context_t * getContext();
	virtual void shutdown();
};

} /* namespace sfc */

#endif /* APPLICATION_H_ */
