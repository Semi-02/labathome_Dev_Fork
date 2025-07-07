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
#include <memory> // Für std::unique_ptr

class SfcAdapter {
private:
    DeviceManager* deviceManager;
    sfc::Application* application = nullptr;

    std::string redLightVar, yellowLightVar, greenLightVar; 
    bool hasLedMapping = false;

    // These vectors own the memory for the context
    std::vector<sfc::Action*> actions;
    std::vector<sfc::Step> steps;
    std::vector<sfc::Transition> transitions;
    
    sfc::component_context_t context;

    ErrorCode ParseJson(cJSON* root);
    sfc::predicate_fnc CreatePredicate(const char* condition);
    void UpdateHardware();

    // Diese Vektoren speichern die Arrays für die Transition-Objekte
    std::vector<std::vector<int>> allInputStepArrays;
    std::vector<std::vector<int>> allOutputStepArrays;
    // Optional: Für Handler-Arrays, falls du sie dynamisch erzeugst
    std::vector<std::vector<sfc::state_handler_t>> allHandlerArrays;

    // Timer-Vektor für zeitgesteuerte Actions:
    std::vector<std::unique_ptr<sfc::Timer>> timers;

public:
    // Moved to public section for access from handlers
    std::map<std::string, bool> boolVarMap;
    std::map<std::string, int> intVarMap;
    std::map<std::string, float> floatVarMap;

    SfcAdapter(DeviceManager* manager);
    ~SfcAdapter();

    ErrorCode LoadFromFile(const char* filename);
    ErrorCode ExecuteCycle(uint32_t ms);
    void Reset();
    bool IsInitialized() const { return application != nullptr; }
    
    // Helper methods for the action handlers
    void SetBoolVar(const std::string& name, bool value);
    bool GetBoolVar(const std::string& name) const;
};
