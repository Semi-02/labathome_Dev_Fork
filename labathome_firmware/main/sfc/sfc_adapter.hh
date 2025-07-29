#pragma once

#include "esp_system.h"
#include "esp_log.h"
#include "devicemanager.hh"
#include <vector>
#include <string>
#include <map>
#include "cJSON.h"
#include "microsfc/include/Application.h"
#include "microsfc/include/Timer.h"
#include <memory>

class SfcAdapter {
private:
    DeviceManager* deviceManager;
    sfc::Application* application;

    std::string redLightVar;
    std::string yellowLightVar;
    std::string greenLightVar;
    bool hasLedMapping;
    bool initialized;
    std::vector<sfc::Action*> actions;
    std::vector<sfc::Step> steps;
    std::vector<sfc::Transition> transitions;
    sfc::component_context_t context;
    std::vector<std::vector<int>> allInputStepArrays;
    std::vector<std::vector<int>> allOutputStepArrays;
    std::vector<std::vector<sfc::state_handler_t>> allHandlerArrays;
    std::vector<std::unique_ptr<sfc::Timer>> timers;

    sfc::predicate_fnc CreatePredicate(const char* condition);
    ErrorCode ParseJson(cJSON* root);
    cJSON* LoadFile(const char* path);

    void UpdateHardware();
    ErrorCode InitializeApplication();
  
public:
    std::map<std::string, bool> boolVarMap;
    std::map<std::string, int> intVarMap;
    std::map<std::string, float> floatVarMap;

    SfcAdapter(DeviceManager* manager);
    ~SfcAdapter();
    ErrorCode LoadFromFile(const char* filename);
    ErrorCode ExecuteCycle(uint32_t ms);
    bool IsInitialized() const;
    void setInitalized(bool initialized);
    void SetBoolVar(const std::string& name, bool value);
    bool GetBoolVar(const std::string& name) const;
 
};