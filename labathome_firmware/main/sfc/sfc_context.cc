#include "sfc_context.hh"

namespace sfc {

DeviceManagerContext::DeviceManagerContext(DeviceManager* deviceManager) 
    : deviceManager(deviceManager) {
    // Register variables and actions here if needed
}

int64_t DeviceManagerContext::GetCurrentTime() const {
    return deviceManager->GetMicroseconds() / 1000; // Convert to milliseconds
}

ErrorCode DeviceManagerContext::SetBool(const std::string& name, bool value) {
    auto it = boolVarMap.find(name);
    if (it == boolVarMap.end()) {
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    return deviceManager->SetBinary(it->second, value);
}

ErrorCode DeviceManagerContext::GetBool(const std::string& name, bool& value) {
    auto it = boolVarMap.find(name);
    if (it == boolVarMap.end()) {
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    value = deviceManager->GetBinary(it->second);
    return ErrorCode::OK;
}

ErrorCode DeviceManagerContext::SetInt(const std::string& name, int value) {
    auto it = intVarMap.find(name);
    if (it == intVarMap.end()) {
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    return deviceManager->SetInteger(it->second, value);
}

ErrorCode DeviceManagerContext::GetInt(const std::string& name, int& value) {
    auto it = intVarMap.find(name);
    if (it == intVarMap.end()) {
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    value = deviceManager->GetInteger(it->second);
    return ErrorCode::OK;
}

ErrorCode DeviceManagerContext::SetFloat(const std::string& name, float value) {
    auto it = floatVarMap.find(name);
    if (it == floatVarMap.end()) {
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    return deviceManager->SetFloat(it->second, value);
}

ErrorCode DeviceManagerContext::GetFloat(const std::string& name, float& value) {
    auto it = floatVarMap.find(name);
    if (it == floatVarMap.end()) {
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    value = deviceManager->GetFloat(it->second);
    return ErrorCode::OK;
}

ErrorCode DeviceManagerContext::ExecuteAction(const std::string& name, bool active) {
    auto it = actionMap.find(name);
    if (it == actionMap.end()) {
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    return it->second(active);
}

ErrorCode DeviceManagerContext::EvaluateCondition(const std::string& condition, bool& result) {
    // Simple implementation for basic conditions
    // For a real implementation, you would need a proper expression parser
    if (condition.empty()) {
        result = true;
        return ErrorCode::OK;
    }
    
    // Very basic implementation - just check if the named boolean is true
    bool value = false;
    ErrorCode ret = GetBool(condition, value);
    if (ret == ErrorCode::OK) {
        result = value;
        return ErrorCode::OK;
    }
    
    // Default to true for now (in a real implementation, you'd parse expressions)
    result = true;
    return ErrorCode::OK;
}

void DeviceManagerContext::RegisterBoolVar(const std::string& name, size_t index) {
    boolVarMap[name] = index;
}

void DeviceManagerContext::RegisterIntVar(const std::string& name, size_t index) {
    intVarMap[name] = index;
}

void DeviceManagerContext::RegisterFloatVar(const std::string& name, size_t index) {
    floatVarMap[name] = index;
}

void DeviceManagerContext::RegisterAction(const std::string& name, std::function<ErrorCode(bool)> handler) {
    actionMap[name] = handler;
}

} // namespace sfc