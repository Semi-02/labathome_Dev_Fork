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
#ifndef SFCTYPES_H_
#define SFCTYPES_H_

#include <stddef.h>
#include <cstdint>
#include <climits>
#include <functional>

#define PTR(a) ((sfc::pointer_t)(&a))
#define SFC_BIT(b,n) ((b >> n) & 1)
#define BIT_SET(b,n) ((1 << n) | b)
#define BIT_RESET(b,n) ((~(1 << n)) & b)
#define ACTIVATING(state) (((state).active) && ((state).transiting))
#define DEACTIVATING(state) (!((state).active) && ((state).transiting))
#define PTR_ACTIVATING(state) (((state)->active) && ((state)->transiting))
#define PTR_ACTIVATED(state) ((state)->activated)
#define PTR_DEACTIVATING(state) (!((state)->active) && ((state)->transiting))
#define PTR_DEACTIVATED(state) ((state)->active)

#define ARRAY(t, n, ...) {(t[]){ __VA_ARGS__ }, n}
#define ARRAY_FOREACH(index_type, index_name, array) for(index_type index_name = 0; index_name < (array).size; index_name++)
#define ARRAY_GET(a,i) ((a).ptr + (i)) 

#define ACTION_STATE_ACTIVATING 'A'
#define ACTION_STATE_ACTIVE 'S'
#define ACTION_STATE_DEACTIVATING 'D'
#define ACTION_STATE_INACTIVE 'I'
#define ACTION_STATE_ALL 'O'

#define STATE_ACTIVATING 'A'
#define STATE_ACTIVE 'S'
#define STATE_DEACTIVATING 'D'
#define STATE_INACTIVE 'I'
#define STATE_ALL 'O'

namespace sfc {

template<typename T>
struct array {
	T *ptr;
	size_t size;
};

template<typename T>
inline array<T> arrayof(T * ptr, const size_t& size) {
	return {ptr, size};
}

typedef unsigned long time_t;

typedef void * pointer_t;
typedef struct {
	sfc::time_t current_time;
	bool enabled :1;
	bool interrupted :1;
} timer_state_t;
typedef struct {
	sfc::time_t active_time;
	bool activated :1;
	bool active :1;
	bool transiting :1;
} stateful_state_t;
typedef void (*state_handler_fnc)(const stateful_state_t & state);

typedef struct {
	char state;
	state_handler_fnc handler_fnc;
} state_handler_t;

typedef struct {
	stateful_state_t action_state;
	stateful_state_t step_state;
} predicate_state_t;

typedef bool (*activation_predicate_fnc)(const predicate_state_t &state);
typedef std::function<bool(void)> predicate_fnc;
}
;

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif

#endif /* SFCTYPES_H_ */
