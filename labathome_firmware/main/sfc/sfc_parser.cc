#include "sfc_parser.hh"
#include <string.h>

namespace sfc {

ErrorCode SfcParser::ParseFromJson(const char* jsonData, std::shared_ptr<SfcChart>& chart) {
    cJSON* root = cJSON_Parse(jsonData);
    if (!root) {
        ESP_LOGE(SFC_TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    
    ErrorCode result = ParseFromJsonObject(root, chart);
    cJSON_Delete(root);
    return result;
}

ErrorCode SfcParser::ParseFromJsonObject(cJSON* root, std::shared_ptr<SfcChart>& chart) {
    // Get chart name
    cJSON* nameObj = cJSON_GetObjectItem(root, "name");
    if (!nameObj || !cJSON_IsString(nameObj)) {
        ESP_LOGE(SFC_TAG, "Chart name not found or not a string");
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    
    // Create new chart
    chart = std::make_shared<SfcChart>(nameObj->valuestring);
    
    // Parse steps
    cJSON* stepsArray = cJSON_GetObjectItem(root, "steps");
    if (!stepsArray || !cJSON_IsArray(stepsArray)) {
        ESP_LOGE(SFC_TAG, "Steps not found or not an array");
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    
    ErrorCode stepResult = ParseSteps(stepsArray, chart);
    if (stepResult != ErrorCode::OK) {
        return stepResult;
    }
    
    // Parse transitions
    cJSON* transitionsArray = cJSON_GetObjectItem(root, "transitions");
    if (!transitionsArray || !cJSON_IsArray(transitionsArray)) {
        ESP_LOGE(SFC_TAG, "Transitions not found or not an array");
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    
    ErrorCode transResult = ParseTransitions(transitionsArray, chart);
    if (transResult != ErrorCode::OK) {
        return transResult;
    }
    
    // Parse connections
    cJSON* connectionsArray = cJSON_GetObjectItem(root, "connections");
    if (!connectionsArray || !cJSON_IsArray(connectionsArray)) {
        ESP_LOGE(SFC_TAG, "Connections not found or not an array");
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    
    ErrorCode connResult = ParseConnections(connectionsArray, chart);
    if (connResult != ErrorCode::OK) {
        return connResult;
    }
    
    ESP_LOGI(SFC_TAG, "Successfully parsed SFC chart: %s", chart->GetName().c_str());
    return ErrorCode::OK;
}

ErrorCode SfcParser::ParseSteps(cJSON* stepsArray, std::shared_ptr<SfcChart>& chart) {
    int numSteps = cJSON_GetArraySize(stepsArray);
    ESP_LOGI(SFC_TAG, "Parsing %d steps", numSteps);
    
    for (int i = 0; i < numSteps; i++) {
        cJSON* stepObj = cJSON_GetArrayItem(stepsArray, i);
        
        // Get step ID
        cJSON* idObj = cJSON_GetObjectItem(stepObj, "id");
        if (!idObj || !cJSON_IsString(idObj)) {
            ESP_LOGE(SFC_TAG, "Step ID not found or not a string");
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Check if step is initial
        cJSON* initialObj = cJSON_GetObjectItem(stepObj, "initial");
        bool isInitial = initialObj && cJSON_IsTrue(initialObj);
        
        // Create step
        auto step = chart->CreateStep(idObj->valuestring, isInitial);
        
        // Parse actions
        cJSON* actionsArray = cJSON_GetObjectItem(stepObj, "actions");
        if (actionsArray && cJSON_IsArray(actionsArray)) {
            ErrorCode actionResult = ParseActions(actionsArray, step);
            if (actionResult != ErrorCode::OK) {
                return actionResult;
            }
        }
    }
    
    return ErrorCode::OK;
}

ErrorCode SfcParser::ParseTransitions(cJSON* transitionsArray, std::shared_ptr<SfcChart>& chart) {
    int numTransitions = cJSON_GetArraySize(transitionsArray);
    ESP_LOGI(SFC_TAG, "Parsing %d transitions", numTransitions);
    
    for (int i = 0; i < numTransitions; i++) {
        cJSON* transObj = cJSON_GetArrayItem(transitionsArray, i);
        
        // Get transition ID
        cJSON* idObj = cJSON_GetObjectItem(transObj, "id");
        if (!idObj || !cJSON_IsString(idObj)) {
            ESP_LOGE(SFC_TAG, "Transition ID not found or not a string");
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Get condition
        cJSON* conditionObj = cJSON_GetObjectItem(transObj, "condition");
        const char* condition = "";
        if (conditionObj && cJSON_IsString(conditionObj)) {
            condition = conditionObj->valuestring;
        }
        
        // Create transition
        chart->CreateTransition(idObj->valuestring, condition);
    }
    
    return ErrorCode::OK;
}

ErrorCode SfcParser::ParseConnections(cJSON* connectionsArray, std::shared_ptr<SfcChart>& chart) {
    int numConnections = cJSON_GetArraySize(connectionsArray);
    ESP_LOGI(SFC_TAG, "Parsing %d connections", numConnections);
    
    for (int i = 0; i < numConnections; i++) {
        cJSON* connObj = cJSON_GetArrayItem(connectionsArray, i);
        
        // Get transition ID
        cJSON* transitionObj = cJSON_GetObjectItem(connObj, "transition");
        if (!transitionObj || !cJSON_IsString(transitionObj)) {
            ESP_LOGE(SFC_TAG, "Connection transition not found or not a string");
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Get source steps
        cJSON* sourcesArray = cJSON_GetObjectItem(connObj, "sources");
        if (!sourcesArray || !cJSON_IsArray(sourcesArray)) {
            ESP_LOGE(SFC_TAG, "Connection sources not found or not an array");
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Get target steps
        cJSON* targetsArray = cJSON_GetObjectItem(connObj, "targets");
        if (!targetsArray || !cJSON_IsArray(targetsArray)) {
            ESP_LOGE(SFC_TAG, "Connection targets not found or not an array");
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Collect source step IDs
        std::vector<std::string> sourceIds;
        int numSources = cJSON_GetArraySize(sourcesArray);
        for (int j = 0; j < numSources; j++) {
            cJSON* sourceObj = cJSON_GetArrayItem(sourcesArray, j);
            if (cJSON_IsString(sourceObj)) {
                sourceIds.push_back(sourceObj->valuestring);
            }
        }
        
        // Collect target step IDs
        std::vector<std::string> targetIds;
        int numTargets = cJSON_GetArraySize(targetsArray);
        for (int j = 0; j < numTargets; j++) {
            cJSON* targetObj = cJSON_GetArrayItem(targetsArray, j);
            if (cJSON_IsString(targetObj)) {
                targetIds.push_back(targetObj->valuestring);
            }
        }
        
        // Connect transition
        ErrorCode connResult = chart->ConnectTransition(
            transitionObj->valuestring, sourceIds, targetIds);
        if (connResult != ErrorCode::OK) {
            return connResult;
        }
    }
    
    return ErrorCode::OK;
}

ErrorCode SfcParser::ParseActions(cJSON* actionsArray, std::shared_ptr<SfcStep>& step) {
    int numActions = cJSON_GetArraySize(actionsArray);
    ESP_LOGI(SFC_TAG, "Parsing %d actions for step %s", numActions, step->GetId().c_str());
    
    for (int i = 0; i < numActions; i++) {
        cJSON* actionObj = cJSON_GetArrayItem(actionsArray, i);
        
        // Get action name
        cJSON* nameObj = cJSON_GetObjectItem(actionObj, "name");
        if (!nameObj || !cJSON_IsString(nameObj)) {
            ESP_LOGE(SFC_TAG, "Action name not found or not a string");
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Get qualifier
        cJSON* qualifierObj = cJSON_GetObjectItem(actionObj, "qualifier");
        if (!qualifierObj || !cJSON_IsString(qualifierObj)) {
            ESP_LOGE(SFC_TAG, "Action qualifier not found or not a string");
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Get time value for timed qualifiers
        uint32_t timeValue = 0;
        cJSON* timeObj = cJSON_GetObjectItem(actionObj, "time");
        if (timeObj && cJSON_IsNumber(timeObj)) {
            timeValue = timeObj->valueint;
        }
        
        // Parse qualifier
        ActionQualifier qualifier = ParseQualifier(qualifierObj->valuestring);
        
        // Create action
        auto action = std::make_shared<SfcAction>(nameObj->valuestring, qualifier, timeValue);
        step->AddAction(action);
    }
    
    return ErrorCode::OK;
}

ActionQualifier SfcParser::ParseQualifier(const char* qualifierStr) {
    if (strcmp(qualifierStr, "N") == 0) {
        return ActionQualifier::N;
    } else if (strcmp(qualifierStr, "R") == 0) {
        return ActionQualifier::R;
    } else if (strcmp(qualifierStr, "S") == 0) {
        return ActionQualifier::S;
    } else if (strcmp(qualifierStr, "L") == 0) {
        return ActionQualifier::L;
    } else if (strcmp(qualifierStr, "D") == 0) {
        return ActionQualifier::D;
    } else if (strcmp(qualifierStr, "P") == 0) {
        return ActionQualifier::P;
    } else if (strcmp(qualifierStr, "SD") == 0) {
        return ActionQualifier::SD;
    } else if (strcmp(qualifierStr, "DS") == 0) {
        return ActionQualifier::DS;
    } else if (strcmp(qualifierStr, "SL") == 0) {
        return ActionQualifier::SL;
    } else {
        ESP_LOGW(SFC_TAG, "Unknown qualifier: %s, defaulting to N", qualifierStr);
        return ActionQualifier::N;
    }
}

} // namespace sfc