#pragma once

#include "esp_system.h"
#include "esp_log.h"
#include "devicemanager.hh"
#include <vector>
#include <string>
#include <map>
#include "cJSON.h"
#include "microsfc/include/Application.h"


class SfcAdapter {
private:
    DeviceManager* deviceManager;
    sfc::Application* application = nullptr;

    bool hasLedMapping = false;
    std::string redLightVar, yellowLightVar, greenLightVar;

    // These vectors own the memory for the context
    std::vector<sfc::Action*> actions;
    std::vector<sfc::Step> steps;
    std::vector<sfc::Transition> transitions;

    std::map<std::string, bool> boolVarMap;
    std::map<std::string, int> intVarMap;
    std::map<std::string, float> floatVarMap;

    sfc::component_context_t context; // Use the correct type

    ErrorCode ParseJson(cJSON* root);
    sfc::predicate_fnc CreatePredicate(const char* condition);
    void UpdateInputs();
    void UpdateHardware();

public:
    SfcAdapter(DeviceManager* manager);
    ~SfcAdapter();

    bool IsInitialized() const { return application != nullptr; }
    ErrorCode LoadFromFile(const char* filename);
    ErrorCode ExecuteCycle();
    void Reset();
};
