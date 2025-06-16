#pragma once

#include "sfc_components.hh"
#include "cJSON.h"
#include <memory>

namespace sfc {

class SfcParser {
public:
    SfcParser() = default;
    ~SfcParser() = default;
    
    // Parse SFC chart from JSON data
    ErrorCode ParseFromJson(const char* jsonData, std::shared_ptr<SfcChart>& chart);
    
    // Parse SFC chart from JSON object
    ErrorCode ParseFromJsonObject(cJSON* root, std::shared_ptr<SfcChart>& chart);
    
private:
    // Parse steps from JSON
    ErrorCode ParseSteps(cJSON* stepsArray, std::shared_ptr<SfcChart>& chart);
    
    // Parse transitions from JSON
    ErrorCode ParseTransitions(cJSON* transitionsArray, std::shared_ptr<SfcChart>& chart);
    
    // Parse connections between steps and transitions
    ErrorCode ParseConnections(cJSON* connectionsArray, std::shared_ptr<SfcChart>& chart);
    
    // Parse actions from a step
    ErrorCode ParseActions(cJSON* actionsArray, std::shared_ptr<SfcStep>& step);
    
    // Helper function to parse action qualifier
    ActionQualifier ParseQualifier(const char* qualifierStr);
};

} // namespace sfc