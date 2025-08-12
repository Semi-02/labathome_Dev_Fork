#define SFC_TAG "SFC_ADAPTER"

#include "sfc_adapter.hh"
#include "sfcParser.cc"
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
//x N  Non-stored               The action is active as long as the step.
//x R  overriding Reset         The action is deactivated.
//x S  Set (Stored)             executes this action as soon as the step is active. The action execution is continued even when the step has been deactivated until it gets a reset.
//x L  time Limited             executes this action as soon as the step is active. The action is executed until the step is deactivated or the given time span has elapsed.
//x D  time Delayed             starts executing the action only after the given delay time has elapsed following step activation and the step is still active. The action is executed until the step is deactivated.
//x P  Pulse                    executes the action exactly two times: one time when the step is activated and one time when the step is deactivated.
//x SD Stored and time Delayed  starts executing the action only after the given delay time has elapsed following step activation. The action is executed until it gets a reset.
//x DS Delayed and Stored       starts executing the action only after the given delay time has elapsed following step activation and the step is still active. The action is executed until it gets a reset.
// SL Stored and time limited  executes this action as soon as the step is activated. It is executed until the specified time has elapsed or it gets a reset.


SfcAdapter::SfcAdapter(DeviceManager* deviceManager)
    : deviceManager(deviceManager),
      hasLedMapping(false) {
    application = nullptr;
    redLightVar = "Red_LED";
    yellowLightVar = "Yellow_LED";
    greenLightVar = "Green_LED";
    redButtonVar = "Red_Button";
    greenButtonVar = "Green_Button";
}

SfcAdapter::~SfcAdapter() {
    ESP_LOGI(SFC_TAG, "SfcAdapter destructor called - freeing all resources");

    initialized = false;
    
    if (application) {
        application->shutdown();
        delete application;
        application = nullptr;
    }
    
    for (auto& timer : timers) {
        if (timer) {
            timer->disable();
        }
    }
    timers.clear();
    

    for (auto* action : actions) {
        if (action) {
            delete action;
        }
    }
    actions.clear();
    
    // Clear remaining containers
    steps.clear();
    transitions.clear();
    
    // Clear nested arrays
    for (auto& handlerArray : allHandlerArrays) {
        handlerArray.clear();
    }
    allHandlerArrays.clear();
    
    for (auto& inputArray : allInputStepArrays) {
        inputArray.clear();
    }
    allInputStepArrays.clear();
    
    for (auto& outputArray : allOutputStepArrays) {
        outputArray.clear();
    }
    allOutputStepArrays.clear();
    
    // Clear variable maps
    boolVarMap.clear();
    intVarMap.clear();
    floatVarMap.clear();
    
    storedActionsByVar.clear();
    ESP_LOGI(SFC_TAG, "SfcAdapter destructor end");
}

ErrorCode SfcAdapter::LoadFromFile(const char* path) {
    cJSON* root = LoadFile(path);

    ErrorCode result = ParseJson(root);
        if (!root) {
        ESP_LOGE(SFC_TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return ErrorCode::INVALID_NEW_FBD;
    }
    cJSON_Delete(root);
    if (result != ErrorCode::OK) {
        return result;
    }
    
    result = InitializeApplication();
    if (result != ErrorCode::OK) {
        return result;
    }

    UpdateHardware();

    ESP_LOGI(SFC_TAG, "SFC loaded successfully");
    return ErrorCode::OK;
}

ErrorCode SfcAdapter::InitializeApplication() {
    context.steps = { steps.data(), steps.size() };
    context.actions = { actions.data(), actions.size() };
    context.transitions = { transitions.data(), transitions.size() };

    application = new sfc::Application(context);
    application->activate();
    initialized = true;
    
    return ErrorCode::OK;
}

ErrorCode SfcAdapter::ExecuteCycle(uint32_t ms)
{
    if (!application) {
        return ErrorCode::NOT_YET_INITIALIZED;
    }

    ReadHardware();

    for (auto& timer : timers) {
        timer->onTick(ms);
    } 
    application->onTick(ms);

    UpdateHardware();

    return ErrorCode::OK;
}

void SfcAdapter::ReadHardware() {
    auto hal = deviceManager->GetHAL();
    
    bool redButtonPressed = hal->GetButtonRedIsPressed();
    SetBoolVar(redButtonVar, redButtonPressed);
    
    bool greenButtonPressed = hal->GetButtonGreenIsPressed();
    SetBoolVar(greenButtonVar, greenButtonPressed);
}

void SfcAdapter::UpdateHardware() {
    auto hal = deviceManager->GetHAL();

 
    bool red = GetBoolVar(redLightVar);
    hal->ColorizeLed(0, red ? CRGB::DarkRed : CRGB::Black);


    bool yellow = GetBoolVar(yellowLightVar);
    hal->ColorizeLed(1, yellow ? CRGB::Yellow : CRGB::Black);


    bool green = GetBoolVar(greenLightVar);
    hal->ColorizeLed(2, green ? CRGB::DarkGreen : CRGB::Black);

}

void SfcAdapter::ResetStoredActionsFor(const std::string& name) {
    auto it = storedActionsByVar.find(name);
    if (it == storedActionsByVar.end()) return;

    for (auto* act : it->second) {
        if (!act) continue;
        
        act->shutdown();
        act->clear();
        ESP_LOGI(SFC_TAG, "Re-armed stored action for '%s'", name.c_str());
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
void SfcAdapter::setInitalized(bool initialized) {
    this->initialized = initialized;
}

bool SfcAdapter::IsInitialized() const {
    return initialized && application != nullptr;
}


cJSON* SfcAdapter::LoadFile(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        ESP_LOGE(SFC_TAG, "Failed to open SFC file: %s", path);
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return nullptr;
    }

    size_t bytesRead = fread(buffer, 1, size, file);
    fclose(file);
    buffer[bytesRead] = '\0';

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);

    return root;
}

bool SfcAdapter::IsStepActive(size_t stepIndex) const {
    if (!application) return false;
    
    // Get the step state from the application
    const sfc::stateful_state_t& stepState = application->getStepState(stepIndex);
    return stepState.active;
}