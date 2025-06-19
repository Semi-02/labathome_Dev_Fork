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
#include "Timer.h"

namespace sfc {

Timer::Timer() {
	this->period = period;
	this->continous = continous;
	this->timer_state = { 0, false, false };
	this->interrupt_callback = 0;
}

Timer::~Timer() {
}

Timer::Timer(const time_t &period, const bool &continous, timer_interrupt_callback_t interrput_callback) {
	this->period = period;
	this->continous = continous;
	this->timer_state = {0, false, false};
	this->interrupt_callback = interrupt_callback;
}

Timer::Timer(const time_t &period, const bool &continous) {
	this->period = period;
	this->continous = continous;
	this->timer_state = {0, false, false};
	this->interrupt_callback = 0;
}

Timer::Timer(const time_t &period) {
	this->period = period;
	this->continous = false;
	this->timer_state = {0, false, false};
	this->interrupt_callback = 0;
}

void Timer::onTick(const sfc::time_t &delta) {
	time_t elapsed_time = this->timer_state.current_time + delta;
	
	if (this->timer_state.enabled) {
		this->timer_state.current_time = elapsed_time % (this->period);
		this->timer_state.interrupted = elapsed_time >= this->period;
		this->timer_state.enabled = this->timer_state.enabled
				&& (this->continous || (!(this->timer_state.interrupted)));
	}
	if(this->timer_state.interrupted) {
		this->timer_state.interrupted = true;
		if(this->interrupt_callback != 0) {
			this->interrupt_callback(elapsed_time);
		}
	}
}

void Timer::enable() {
	if (!(this->timer_state.enabled = true)) {
		this->reset();
		this->timer_state.enabled = true;
	}
}
void Timer::reset() {
	this->timer_state.current_time = 0l;
	this->timer_state.interrupted = false;
}

void Timer::disable() {
	this->reset();
	this->timer_state.enabled = false;
}

timer_state_t * Timer::getState() {
	return &(this->timer_state);
}

sfc::time_t * Timer::getPeriod() {
	return &(this->period);
}

void Timer::setInterruptCallback(timer_interrupt_callback_t interrupt_callback) {
	this->interrupt_callback = interrupt_callback;
}

} /* namespace sfc */
