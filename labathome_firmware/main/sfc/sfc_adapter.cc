#define SFC_TAG "SFC_ADAPTER"

#include "sfc_adapter.hh"
#include "crgb.hh"
#include "esp_log.h"
#include <string.h>
#include <sstream>
#include <map>
#include "microsfc/include/Application.h"
#include "microsfc/include/Step.h"
#include "microsfc/include/Action.h"
#include "microsfc/include/Transition.h"
#include "microsfc/include/sfctypes.h"
#include "microsfc/include/StatefulObject.h"
#include "microsfc/include/EventListener.h"
#include "microsfc/include/StepContext.h"
#include "microsfc/include/Timer.h"
#include "microsfc/include/NonStoredAction.h"
#include "microsfc/include/StoredAction.h"

//Qualifier:
// N  Non-stored               The action is active as long as the step.
// R  overriding Reset         The action is deactivated.
// S  Set (Stored)             executes this action as soon as the step is active. The action execution is continued even when the step has been deactivated until it gets a reset.
// L  time Limited             executes this action as soon as the step is active. The action is executed until the step is deactivated or the given time span has elapsed.
// D  time Delayed             starts executing the action only after the given delay time has elapsed following step activation and the step is still active. The action is executed until the step is deactivated.
// P  Pulse                    executes the action exactly two times: one time when the step is activated and one time when the step is deactivated.
// SD Stored and time Delayed  starts executing the action only after the given delay time has elapsed following step activation. The action is executed until it gets a reset.
// DS Delayed and Stored       starts executing the action only after the given delay time has elapsed following step activation and the step is still active. The action is executed until it gets a reset.
// SL Stored and time limited  executes this action as soon as the step is activated. It is executed until the specified time has elapsed or it gets a reset.</p></td></tr>

SfcAdapter::SfcAdapter(DeviceManager* deviceManager)
    : deviceManager(deviceManager),
      hasLedMapping(false) {
    application = nullptr;
    redLightVar = "Red_LED";
    yellowLightVar = "Yellow_LED";
    greenLightVar = "Green_LED";
}

SfcAdapter::~SfcAdapter() {
    if (application) {
        delete application;
    }
    for (auto action : actions) {
        delete action;
    }
}

ErrorCode SfcAdapter::LoadFromFile(const char* path) {
    Reset();

    FILE* file = fopen(path, "r");
    if (!file) {
        ESP_LOGE(SFC_TAG, "Failed to open SFC file: %s", path);
        return ErrorCode::FILE_SYSTEM_ERROR;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return ErrorCode::FILE_SYSTEM_ERROR;
    }

    size_t bytesRead = fread(buffer, 1, size, file);
    fclose(file);
    buffer[bytesRead] = '\0';

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);

    if (!root) {
        ESP_LOGE(SFC_TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return ErrorCode::INVALID_NEW_FBD;
    }

    ErrorCode result = ParseJson(root);
    cJSON_Delete(root);

    if (result != ErrorCode::OK) {
        return result;
    }

    // Set up context arrays using the vectors we've populated
    context.steps = { steps.data(), steps.size() };
    context.actions = { actions.data(), actions.size() };
    context.transitions = { transitions.data(), transitions.size() };

    // Create application with the correct context type
    application = new sfc::Application(context);
    application->activate();

    // Initial hardware update to set the starting state
    UpdateHardware();

    ESP_LOGI(SFC_TAG, "SFC loaded successfully");
    return ErrorCode::OK;
}

ErrorCode SfcAdapter::ExecuteCycle(uint32_t ms)
{
    if (!application) {
        return ErrorCode::NOT_YET_INITIALIZED;
    }
    
      // Timer tick hinzufügen:
    for (auto& timer : timers) {
        timer->onTick(ms);
    }
    
    ESP_LOGI(SFC_TAG, "SFC Tick");  
    
    application->onTick(ms);
    
    // Update hardware based on SFC state
    UpdateHardware();
    
    return ErrorCode::OK;
}
void SfcAdapter::Reset() {
    if (application) {
        delete application;
        application = nullptr;
    }

    // Clean up actions
    for (auto action : actions) {
        delete action;
    }

    // Clear containers
    steps.clear();
    actions.clear();
    transitions.clear();
    boolVarMap.clear();
    intVarMap.clear();
    floatVarMap.clear();

    // Reset context
    context = { {NULL, 0}, {NULL, 0}, {NULL, 0} };

    // Reset LED variables
    hasLedMapping = false;

    // Clear handler context vectors
    allInputStepArrays.clear();
    allOutputStepArrays.clear();
    allHandlerArrays.clear();
}

ErrorCode SfcAdapter::ParseJson(cJSON* root) {
    // Reset all containers
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
                allHandlerArrays.push_back({
                    { ACTION_STATE_ACTIVATING, [this, targetBoolName, timerPtr](const sfc::stateful_state_t&) {
                        this->SetBoolVar(targetBoolName, true);
                        timerPtr->enable();
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

sfc::predicate_fnc SfcAdapter::CreatePredicate(const char* condition) {
    std::string condStr(condition);
    
    // Erkennen von komplexeren Bedingungen
    if (condStr.find("==") != std::string::npos) {
        // Format: "variable == value"
        std::string varName = condStr.substr(0, condStr.find("=="));
        std::string valueStr = condStr.substr(condStr.find("==") + 2);
        
        // Whitespace entfernen
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

void SfcAdapter::UpdateHardware() {
    auto hal = deviceManager->GetHAL();

    // Red LED
    bool red = GetBoolVar(redLightVar);
    hal->ColorizeLed(0, red ? CRGB::DarkRed : CRGB::Black);

    // Yellow LED
    bool yellow = GetBoolVar(yellowLightVar);
    hal->ColorizeLed(1, yellow ? CRGB::Yellow : CRGB::Black);

    // Green LED
    bool green = GetBoolVar(greenLightVar);
    hal->ColorizeLed(2, green ? CRGB::DarkGreen : CRGB::Black);

    // Debug output
    ESP_LOGI(SFC_TAG, "LED states:");
    ESP_LOGI(SFC_TAG, "Red LED state: %s", red ? "ON" : "OFF");
    ESP_LOGI(SFC_TAG, "Yellow LED state: %s", yellow ? "ON" : "OFF");
    ESP_LOGI(SFC_TAG, "Green LED state: %s", green ? "ON" : "OFF");

    // Booleans
    ESP_LOGI(SFC_TAG, "Boolean states:");
    for (const auto &pair : boolVarMap)
    {
        ESP_LOGI(SFC_TAG, "%s: %s", pair.first.c_str(), pair.second ? "true" : "false");
    }

    // Timers
    ESP_LOGI(SFC_TAG, "Timer states:");
    int timerIdx = 0;
    for (const auto &timer : timers)
    {
        auto *state = timer->getState();
        ESP_LOGI(SFC_TAG, "Timer %d: enabled: %s, interrupted: %s, elapsed: %lu ms / %lu ms",
                 timerIdx,
                 state->enabled ? "true" : "false",
                 state->interrupted ? "true" : "false",
                 static_cast<unsigned long>(state->current_time),
                 static_cast<unsigned long>(timer->getPeriod() ? *timer->getPeriod() : 0));
        timerIdx++;
    }
}

void SfcAdapter::SetBoolVar(const std::string& name, bool value) {
    auto it = boolVarMap.find(name);
    if (it != boolVarMap.end()) {
        it->second = value;
    }
}

bool SfcAdapter::GetBoolVar(const std::string& name) const {
    auto it = boolVarMap.find(name);
    if (it != boolVarMap.end()) {
        return it->second;
    }
    return false;
}