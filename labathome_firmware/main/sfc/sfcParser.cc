#include "sfc_adapter.hh"
#include "esp_log.h"

// Benötigte Include-Dateien
#include "microsfc/include/Action.h"
#include "microsfc/include/NonStoredAction.h"
#include "microsfc/include/StoredAction.h"
#include "microsfc/include/Transition.h"
#include "microsfc/include/Timer.h"

// Der Tag muss hier erneut definiert werden
#define SFC_TAG "SFC_PARSING"


sfc::predicate_fnc SfcAdapter::CreatePredicate(const char* condition) {
    std::string condStr(condition);
    
    
    if (condStr.find("==") != std::string::npos) {
        // Format: "variable == value"
        std::string varName = condStr.substr(0, condStr.find("=="));
        std::string valueStr = condStr.substr(condStr.find("==") + 2);
        
        varName.erase(0, varName.find_first_not_of(" \t"));
        varName.erase(varName.find_last_not_of(" \t") + 1);
        valueStr.erase(0, valueStr.find_first_not_of(" \t"));
        valueStr.erase(valueStr.find_last_not_of(" \t") + 1);
        
        bool expectedValue = (valueStr == "true");
        
        return [this, varName, expectedValue]() -> bool {
            auto it = boolVarMap.find(varName);
            if (it == boolVarMap.end()) {
                ESP_LOGE(SFC_TAG, "Variable not found in boolean map");
                return false;
            }
            
            return it->second == expectedValue;
        };
    } 
    else if (condStr.find("!=") != std::string::npos) {
        // Format: "variable != value"
        std::string varName = condStr.substr(0, condStr.find("!="));
        std::string valueStr = condStr.substr(condStr.find("!=") + 2);
        
        // Whitespace entfernen
        varName.erase(0, varName.find_first_not_of(" \t"));
        varName.erase(varName.find_last_not_of(" \t") + 1);
        valueStr.erase(0, valueStr.find_first_not_of(" \t"));
        valueStr.erase(valueStr.find_last_not_of(" \t") + 1);
        
        bool expectedValue = (valueStr == "true");
        
        return [this, varName, expectedValue]() -> bool {
            auto it = boolVarMap.find(varName);
            if (it == boolVarMap.end()) {
                ESP_LOGE(SFC_TAG, "Variable  not found in boolean map");
                return false;
            }
            
            return it->second != expectedValue;
        };
    }
    
    ESP_LOGE(SFC_TAG, "Unsupported condition format");
    return []() -> bool { return false; };
}


ErrorCode SfcAdapter::ParseJson(cJSON* root) {
    // Reset all containers 
    //TO-DO: Clearing of Containers should be in Reset Method only remove them here 
    steps.clear();
    for (auto action : actions) {
        delete action;
    }
    actions.clear();
    transitions.clear();
    boolVarMap.clear();
    intVarMap.clear();
    floatVarMap.clear();

    // 1. Parse boolean variables
    cJSON* booleans = cJSON_GetObjectItem(root, "booleans");
    if (!booleans) {
        return ErrorCode::INVALID_NEW_FBD;
    }

    const char* categories[] = {"hardware", "custom"};
    for (const char* category : categories) {
        cJSON* section = cJSON_GetObjectItem(booleans, category);
        if (section) {
            cJSON* var;
            cJSON_ArrayForEach(var, section) {
                std::string name = var->string;
                bool value = cJSON_IsTrue(var);
                boolVarMap[name] = value;
            }
        }
    }

    // 2. Parse steps
    cJSON* stepsArray = cJSON_GetObjectItem(root, "steps");
    if (!stepsArray || !cJSON_IsArray(stepsArray)) {
        return ErrorCode::INVALID_NEW_FBD;
    }

    // Find entry point
    cJSON* startStep = cJSON_GetObjectItem(root, "start");
    std::string startStepId = startStep ? startStep->valuestring : "";
    if (startStepId.empty()) {
        return ErrorCode::INVALID_NEW_FBD;
    }

    // Create steps and index mapping
    std::map<std::string, size_t> stepUidToIndex;
    cJSON* step;
    int stepIndex = 0;
    
    cJSON_ArrayForEach(step, stepsArray) {
        cJSON* uid = cJSON_GetObjectItem(step, "uid");
        if (!uid || !cJSON_IsString(uid)) {
            continue;
        }
        
        std::string stepUid = uid->valuestring;
        bool isEntryPoint = (stepUid == startStepId);
        
        steps.push_back(sfc::Step(isEntryPoint));
        stepUidToIndex[stepUid] = stepIndex++;
    }

    // 3. Parse actions
    stepIndex = 0;
    cJSON_ArrayForEach(step, stepsArray) {
        cJSON* actionsArray = cJSON_GetObjectItem(step, "actions");
        if (!actionsArray || !cJSON_IsArray(actionsArray)) {
            stepIndex++;
            continue;
        }
        
        cJSON* actionItem;
        cJSON_ArrayForEach(actionItem, actionsArray) {
            cJSON* targetBool = cJSON_GetObjectItem(actionItem, "targetBoolean");
            cJSON* qualifier = cJSON_GetObjectItem(actionItem, "qualifier");
            cJSON* ms_time = cJSON_GetObjectItem(actionItem, "ms_time");
            int msTime = (ms_time && cJSON_IsNumber(ms_time)) ? ms_time->valueint : 0;

            if (!targetBool || !cJSON_IsString(targetBool) || 
                !qualifier || !cJSON_IsString(qualifier)) {
                continue;
            }

            std::string targetBoolName = targetBool->valuestring;
            std::string qualifierStr = qualifier->valuestring;
            sfc::Action* action = nullptr;
            // TODO : Check if generating Actions would be better with a factory method
            if (qualifierStr == "N") {
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [this, targetBoolName](const sfc::stateful_state_t&) {
                        this->SetBoolVar(targetBoolName, true);
                    }},
                    { ACTION_STATE_DEACTIVATING, [this, targetBoolName](const sfc::stateful_state_t&) {
                        this->SetBoolVar(targetBoolName, false);
                    }}
                });
                action = new sfc::NonStoredAction(
                    stepIndex,
                    sfc::arrayof(allHandlerArrays.back().data(), allHandlerArrays.back().size())
                );
            }
            else if (qualifierStr == "R") {
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [this, targetBoolName](const sfc::stateful_state_t&) {
                        this->SetBoolVar(targetBoolName, false);
                    }}
                });
                action = new sfc::StoredAction(
                    stepIndex,
                    sfc::arrayof(allHandlerArrays.back().data(), allHandlerArrays.back().size())
                );
            }
            else if (qualifierStr == "S") {
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [this, targetBoolName](const sfc::stateful_state_t&) {
                        this->SetBoolVar(targetBoolName, true);
                    }}
                });
                action = new sfc::StoredAction(
                    stepIndex,
                    sfc::arrayof(allHandlerArrays.back().data(), allHandlerArrays.back().size())
                );
            }
            else if (qualifierStr == "L" && msTime > 0) {
                timers.push_back(std::make_unique<sfc::Timer>(msTime, false));
                sfc::Timer* timerPtr = timers.back().get();
                
                // Create a safer handler that checks timer validity
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [this, targetBoolName, timerPtr](const sfc::stateful_state_t&) {
                        if (timerPtr && timerPtr->getState()) {  // Add safety check
                            this->SetBoolVar(targetBoolName, true);
                            timerPtr->enable();
                        }
                    }},
                    { ACTION_STATE_DEACTIVATING, [this, targetBoolName, timerPtr](const sfc::stateful_state_t&) {
                        if (timerPtr && timerPtr->getState()) {  // Add safety check
                            this->SetBoolVar(targetBoolName, false);
                            timerPtr->disable();
                        }
                    }}
                });
                action = new sfc::NonStoredAction(
                    stepIndex,
                    sfc::arrayof(allHandlerArrays.back().data(), allHandlerArrays.back().size())
                );
            }
            else if (qualifierStr == "D" && msTime > 0) {
                timers.push_back(std::make_unique<sfc::Timer>(msTime, false));
                sfc::Timer* timerPtr = timers.back().get();
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [timerPtr](const sfc::stateful_state_t&) {
                        timerPtr->enable();
                    }},
                    { ACTION_STATE_ACTIVE, [this, targetBoolName, timerPtr](const sfc::stateful_state_t&) {
                        if (timerPtr->getState()->interrupted)
                            this->SetBoolVar(targetBoolName, true);
                    }},
                    { ACTION_STATE_DEACTIVATING, [this, targetBoolName, timerPtr](const sfc::stateful_state_t&) {
                        this->SetBoolVar(targetBoolName, false);
                        timerPtr->disable();
                    }}
                });
                action = new sfc::NonStoredAction(
                    stepIndex,
                    sfc::arrayof(allHandlerArrays.back().data(), allHandlerArrays.back().size())
                );
            }
            else if (qualifierStr == "P") {
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [this, targetBoolName](const sfc::stateful_state_t&) {
                        bool current = this->GetBoolVar(targetBoolName);
                        this->SetBoolVar(targetBoolName, !current);
                    }},
                    { ACTION_STATE_DEACTIVATING, [this, targetBoolName](const sfc::stateful_state_t&) {
                        bool current = this->GetBoolVar(targetBoolName);
                        this->SetBoolVar(targetBoolName, !current);
                    }}
                });
                action = new sfc::NonStoredAction(
                    stepIndex,
                    sfc::arrayof(allHandlerArrays.back().data(), allHandlerArrays.back().size())
                );
            }
            else if (qualifierStr == "SD" && msTime > 0) {
                timers.push_back(std::make_unique<sfc::Timer>(msTime, false));
                sfc::Timer* timerPtr = timers.back().get();
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [timerPtr](const sfc::stateful_state_t&) {
                        timerPtr->enable();
                    }},
                    { ACTION_STATE_ACTIVE, [this, targetBoolName, timerPtr](const sfc::stateful_state_t&) {
                        if (timerPtr->getState()->interrupted)
                            this->SetBoolVar(targetBoolName, true);
                    }}
                });
                action = new sfc::StoredAction(
                    stepIndex,
                    sfc::arrayof(allHandlerArrays.back().data(), allHandlerArrays.back().size())
                );
            }
            else if (qualifierStr == "DS" && msTime > 0) {
                timers.push_back(std::make_unique<sfc::Timer>(msTime, false));
                sfc::Timer* timerPtr = timers.back().get();
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [timerPtr](const sfc::stateful_state_t&) {
                        timerPtr->enable();
                    }},
                    { ACTION_STATE_ACTIVE, [this, targetBoolName, timerPtr](const sfc::stateful_state_t&) {
                        if (timerPtr->getState()->interrupted)
                            this->SetBoolVar(targetBoolName, true);
                    }}
                });
                action = new sfc::StoredAction(
                    stepIndex,
                    sfc::arrayof(allHandlerArrays.back().data(), allHandlerArrays.back().size())
                );
            }
            else if (qualifierStr == "SL" && msTime > 0) {
                timers.push_back(std::make_unique<sfc::Timer>(msTime, false));
                sfc::Timer* timerPtr = timers.back().get();
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [this, targetBoolName, timerPtr](const sfc::stateful_state_t&) {
                        this->SetBoolVar(targetBoolName, true);
                        timerPtr->enable();
                    }},
                    { ACTION_STATE_ACTIVE, [this, targetBoolName, timerPtr](const sfc::stateful_state_t&) {
                        if (timerPtr->getState()->interrupted)
                            this->SetBoolVar(targetBoolName, false);
                    }}
                });
                action = new sfc::StoredAction(
                    stepIndex,
                    sfc::arrayof(allHandlerArrays.back().data(), allHandlerArrays.back().size())
                );
            }

            if (action) {
                actions.push_back(action);
            }
        }
        
        stepIndex++;
    }

    // 4. Parse transitions
    stepIndex = 0;
    cJSON_ArrayForEach(step, stepsArray) {
        cJSON* uid = cJSON_GetObjectItem(step, "uid");
        if (!uid || !cJSON_IsString(uid)) {
            stepIndex++;
            continue;
        }
        std::string sourceStepId = uid->valuestring;
        
        cJSON* transitionsArray = cJSON_GetObjectItem(step, "outgoingTransitions");
        if (!transitionsArray || !cJSON_IsArray(transitionsArray)) {
            stepIndex++;
            continue;
        }
        
        cJSON* transition;
        cJSON_ArrayForEach(transition, transitionsArray) {
            cJSON* condition = cJSON_GetObjectItem(transition, "condition");
            cJSON* target = cJSON_GetObjectItem(transition, "target");
            
            std::string conditionStr;
            if (condition && cJSON_IsArray(condition) && cJSON_GetArraySize(condition) > 0) {
                cJSON* firstCond = cJSON_GetArrayItem(condition, 0);
                if (firstCond && cJSON_IsString(firstCond)) {
                    conditionStr = firstCond->valuestring;
                }
            } else if (condition && cJSON_IsString(condition)) {
                conditionStr = condition->valuestring;
            }
            if (conditionStr.empty()) continue;
            
            std::string targetStepId;
            if (target && cJSON_IsArray(target) && cJSON_GetArraySize(target) > 0) {
                cJSON* firstTarget = cJSON_GetArrayItem(target, 0);
                if (firstTarget && cJSON_IsString(firstTarget)) {
                    targetStepId = firstTarget->valuestring;
                }
            } else if (target && cJSON_IsString(target)) {
                targetStepId = target->valuestring;
            }
            if (targetStepId.empty()) continue;
            
            // Validate step indices
            if (stepUidToIndex.find(sourceStepId) == stepUidToIndex.end() ||
                stepUidToIndex.find(targetStepId) == stepUidToIndex.end()) {
                continue;
            }
            
            int sourceIndex = stepUidToIndex[sourceStepId];
            int targetIndex = stepUidToIndex[targetStepId];
            
            // Create predicate function
            auto predicateFn = CreatePredicate(conditionStr.c_str());
            
            // Create input and output arrays
            allInputStepArrays.push_back({sourceIndex});
            allOutputStepArrays.push_back({targetIndex});
            // Create and add transition
            sfc::Transition newTransition(
                sfc::arrayof(allInputStepArrays.back().data(), allInputStepArrays.back().size()),
                sfc::arrayof(allOutputStepArrays.back().data(), allOutputStepArrays.back().size()),
                predicateFn
            );
            
            transitions.push_back(newTransition);
        }
        
        stepIndex++;
    }

    // Validate we have at least one step
    if (steps.empty()) {
        return ErrorCode::INVALID_NEW_FBD;
    }

    return ErrorCode::OK;
}