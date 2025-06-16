#pragma once

#include "devicemanager.hh"
#include <unordered_map>
#include <string>
#include <functional>

namespace sfc {

// Interface for SFC execution context
class SfcContext {
public:
    virtual ~SfcContext() = default;
    
    // Get current time in milliseconds
    virtual int64_t GetCurrentTime() const = 0;
    
    // Set a boolean variable
    virtual ErrorCode SetBool(const std::string& name, bool value) = 0;
    
    // Get a boolean variable
    virtual ErrorCode GetBool(const std::string& name, bool& value) = 0;
    
    // Set an integer variable
    virtual ErrorCode SetInt(const std::string& name, int value) = 0;
    
    // Get an integer variable
    virtual ErrorCode GetInt(const std::string& name, int& value) = 0;
    
    // Set a float variable
    virtual ErrorCode SetFloat(const std::string& name, float value) = 0;
    
    // Get a float variable
    virtual ErrorCode GetFloat(const std::string& name, float& value) = 0;
    
    // Execute an action by name
    virtual ErrorCode ExecuteAction(const std::string& name, bool active) = 0;
    
    // Evaluate a condition expression
    virtual ErrorCode EvaluateCondition(const std::string& condition, bool& result) = 0;
};

// Implementation of SFC context using DeviceManager
class DeviceManagerContext : public SfcContext {
private:
    DeviceManager* deviceManager;
    std::unordered_map<std::string, size_t> boolVarMap;
    std::unordered_map<std::string, size_t> intVarMap;
    std::unordered_map<std::string, size_t> floatVarMap;
    std::unordered_map<std::string, std::function<ErrorCode(bool)>> actionMap;
    
public:
    DeviceManagerContext(DeviceManager* deviceManager);
    ~DeviceManagerContext() = default;
    
    // Register a boolean variable with its index
    void RegisterBoolVar(const std::string& name, size_t index);
    
    // Register an integer variable with its index
    void RegisterIntVar(const std::string& name, size_t index);
    
    // Register a float variable with its index
    void RegisterFloatVar(const std::string& name, size_t index);
    
    // Register an action with its handler
    void RegisterAction(const std::string& name, std::function<ErrorCode(bool)> handler);
    
    // SfcContext interface implementation
    int64_t GetCurrentTime() const override;
    ErrorCode SetBool(const std::string& name, bool value) override;
    ErrorCode GetBool(const std::string& name, bool& value) override;
    ErrorCode SetInt(const std::string& name, int value) override;
    ErrorCode GetInt(const std::string& name, int& value) override;
    ErrorCode SetFloat(const std::string& name, float value) override;
    ErrorCode GetFloat(const std::string& name, float& value) override;
    ErrorCode ExecuteAction(const std::string& name, bool active) override;
    ErrorCode EvaluateCondition(const std::string& condition, bool& result) override;
};

} // namespace sfc