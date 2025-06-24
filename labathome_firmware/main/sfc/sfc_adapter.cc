// #define SFC_TAG "SFC_ADAPTER"

// #include "sfc_adapter.hh"
// #include "crgb.hh"
// #include "esp_log.h"
// #include <string.h>
// #include "microsfc/include/Application.h"
// #include "microsfc/include/Step.h"
// #include "microsfc/include/Action.h"
// #include "microsfc/include/Transition.h"
// #include "microsfc/include/sfctypes.h"
// #include "microsfc/include/StatefulObject.h"
// #include "microsfc/include/EventListener.h"
// #include "microsfc/include/StepContext.h"
// #include "microsfc/include/Timer.h"
// #include "microsfc/include/NonStoredAction.h"
// #include "microsfc/include/StoredAction.h"

// SfcAdapter::SfcAdapter(DeviceManager* deviceManager) : 
//     deviceManager(deviceManager),
//     hasLedMapping(false) {
//     application = nullptr;
//     redLightVar = "";
//     yellowLightVar = "";
//     greenLightVar = "";
// }

// SfcAdapter::~SfcAdapter() {
//     if (application) {
//         delete application;
//     }
//     for (auto action : actions) {
//         delete action;
//     }
// }

// ErrorCode SfcAdapter::LoadFromFile(const char* path) {
//     Reset();

//     FILE* file = fopen(path, "r");
//     if (!file) {
//         ESP_LOGE(SFC_TAG, "Failed to open SFC file: %s", path);
//         return ErrorCode::FILE_SYSTEM_ERROR;
//     }

//     fseek(file, 0, SEEK_END);
//     long size = ftell(file);
//     fseek(file, 0, SEEK_SET);

//     char* buffer = (char*)malloc(size + 1);
//     if (!buffer) {
//         fclose(file);
//         return ErrorCode::FILE_SYSTEM_ERROR;
//     }

//     size_t bytesRead = fread(buffer, 1, size, file);
//     fclose(file);
//     buffer[bytesRead] = '\0';

//     cJSON* root = cJSON_Parse(buffer);
//     free(buffer);

//     if (!root) {
//         ESP_LOGE(SFC_TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
//         return ErrorCode::INVALID_NEW_FBD;
//     }

//     ErrorCode result = ParseJson(root);
//     cJSON_Delete(root);

//     if (result != ErrorCode::OK) {
//         return result;
//     }

//     // Create application with the correct context type
//     application = new sfc::Application(context);
//     application->activate();

//     ESP_LOGI(SFC_TAG, "SFC loaded successfully");
//     return ErrorCode::OK;
// }

// ErrorCode SfcAdapter::ExecuteCycle() {
//     if (!application) {
//         return ErrorCode::NOT_YET_INITIALIZED;
//     }
    
//     // Update inputs from hardware
//     UpdateInputs();
    
//     // Execute one SFC cycle
//     application->onTick(1);  // 1ms tick
    
//     // Update hardware based on SFC state
//     UpdateHardware();
    
//     return ErrorCode::OK;
// }

// void SfcAdapter::Reset() {
//     if (application) {
//         delete application;
//         application = nullptr;
//     }
    
//     // Clean up actions
//     for (auto action : actions) {
//         delete action;
//     }
    
//     // Clear containers
//     steps.clear();
//     actions.clear();
//     transitions.clear();
//     boolVarMap.clear();
//     intVarMap.clear();
//     floatVarMap.clear();
    
//     // Reset context
//     context = { {NULL, 0}, {NULL, 0}, {NULL, 0} };
    
//     // Reset LED variables
//     redLightVar = "";
//     yellowLightVar = "";
//     greenLightVar = "";
//     hasLedMapping = false;
// }

// ErrorCode SfcAdapter::ParseJson(cJSON* root) {
//     // Parse steps
//     cJSON* stepsArray = cJSON_GetObjectItem(root, "steps");
//     if (!stepsArray || !cJSON_IsArray(stepsArray)) {
//         ESP_LOGE(SFC_TAG, "Steps not found or not an array");
//         return ErrorCode::INVALID_NEW_FBD;
//     }
    
//     // Create steps
//     int stepCount = cJSON_GetArraySize(stepsArray);
//     steps.reserve(stepCount);
    
//     for (int i = 0; i < stepCount; i++) {
//         cJSON* stepObj = cJSON_GetArrayItem(stepsArray, i);
        
//         cJSON* idObj = cJSON_GetObjectItem(stepObj, "id");
//         cJSON* initialObj = cJSON_GetObjectItem(stepObj, "initial");
        
//         bool isInitial = initialObj && cJSON_IsTrue(initialObj);
//         steps.emplace_back(isInitial);
        
//         // Add actions for this step
//         cJSON* actionsArray = cJSON_GetObjectItem(stepObj, "actions");
//         if (actionsArray && cJSON_IsArray(actionsArray)) {
//             int actionCount = cJSON_GetArraySize(actionsArray);
            
//             for (int j = 0; j < actionCount; j++) {
//                 cJSON* actionObj = cJSON_GetArrayItem(actionsArray, j);
                
//                 cJSON* nameObj = cJSON_GetObjectItem(actionObj, "name");
//                 cJSON* qualifierObj = cJSON_GetObjectItem(actionObj, "qualifier");
                
//                 if (!nameObj || !cJSON_IsString(nameObj)) {
//                     continue;
//                 }
                
//                 // Create appropriate action type based on qualifier
//                 sfc::Action* action = nullptr;
//                 const char* qualifier = qualifierObj && cJSON_IsString(qualifierObj) ? 
//                                       qualifierObj->valuestring : "N";
                
//                 if (strcmp(qualifier, "S") == 0) {
//                     // Stored action
//                     action = new sfc::StoredAction(i);
//                 } else {
//                     // Non-stored action
//                     action = new sfc::NonStoredAction(i);
//                 }
                
//                 actions.push_back(action);
//             }
//         }
//     }
    
//     // Parse transitions
//     cJSON* transitionsArray = cJSON_GetObjectItem(root, "transitions");
//     if (!transitionsArray || !cJSON_IsArray(transitionsArray)) {
//         ESP_LOGE(SFC_TAG, "Transitions not found or not an array");
//         return ErrorCode::INVALID_NEW_FBD;
//     }
    
//     // Create transitions
//     int transitionCount = cJSON_GetArraySize(transitionsArray);
//     transitions.reserve(transitionCount);
    
//     for (int i = 0; i < transitionCount; i++) {
//         cJSON* transObj = cJSON_GetArrayItem(transitionsArray, i);
        
//         cJSON* idObj = cJSON_GetObjectItem(transObj, "id");
//         cJSON* conditionObj = cJSON_GetObjectItem(transObj, "condition");
//         cJSON* sourcesObj = cJSON_GetObjectItem(transObj, "sources");
//         cJSON* targetsObj = cJSON_GetObjectItem(transObj, "targets");
        
//         if (!idObj || !cJSON_IsString(idObj) || 
//             !sourcesObj || !cJSON_IsArray(sourcesObj) ||
//             !targetsObj || !cJSON_IsArray(targetsObj)) {
//             continue;
//         }
        
//         // Get condition
//         const char* condition = conditionObj && cJSON_IsString(conditionObj) ? 
//                                conditionObj->valuestring : "1";
        
//         // Create predicate function
//         sfc::predicate_fnc predicate = CreatePredicate(condition);
        
//         // Get source step IDs
//         std::vector<int> sourceIds;
//         int sourceCount = cJSON_GetArraySize(sourcesObj);
//         for (int j = 0; j < sourceCount; j++) {
//             cJSON* sourceObj = cJSON_GetArrayItem(sourcesObj, j);
//             if (cJSON_IsNumber(sourceObj)) {
//                 sourceIds.push_back(sourceObj->valueint);
//             }
//         }
        
//         // Get target step IDs
//         std::vector<int> targetIds;
//         int targetCount = cJSON_GetArraySize(targetsObj);
//         for (int j = 0; j < targetCount; j++) {
//             cJSON* targetObj = cJSON_GetArrayItem(targetsObj, j);
//             if (cJSON_IsNumber(targetObj)) {
//                 targetIds.push_back(targetObj->valueint);
//             }
//         }
        
//         // Create transition
//         sfc::array<int> sourceIdArray = { sourceIds.data(), sourceIds.size() };
//         sfc::array<int> targetIdArray = { targetIds.data(), targetIds.size() };
//         transitions.emplace_back(sourceIdArray, targetIdArray, predicate);
//     }
    
//     // Parse boolean variables from the JSON
//     cJSON* booleansObj = cJSON_GetObjectItem(root, "booleans");
//     if (booleansObj && cJSON_IsObject(booleansObj)) {
//         // Create boolean variables and map them
//         int boolIndex = 0;
//         cJSON* item;
//         cJSON_ArrayForEach(item, booleansObj) {
//             if (item->string) {
//                 std::string varName(item->string);
//                 boolVarMap[varName] = boolIndex++;
                
//                 // Log the boolean variable mapping
//                 ESP_LOGI(SFC_TAG, "Mapped boolean variable '%s' to index %d", varName.c_str(), boolIndex-1);
//             }
//         }
//     }
    
//     // Look for action target booleans (these control the LEDs)
//     hasLedMapping = false;
//     for (int i = 0; i < stepCount; i++) {
//         cJSON* stepObj = cJSON_GetArrayItem(stepsArray, i);
//         cJSON* actionsArray = cJSON_GetObjectItem(stepObj, "actions");
        
//         if (actionsArray && cJSON_IsArray(actionsArray)) {
//             int actionCount = cJSON_GetArraySize(actionsArray);
            
//             for (int j = 0; j < actionCount; j++) {
//                 cJSON* actionObj = cJSON_GetArrayItem(actionsArray, j);
//                 cJSON* targetObj = cJSON_GetObjectItem(actionObj, "targetBoolean");
                
//                 if (targetObj && cJSON_IsString(targetObj)) {
//                     std::string targetName = targetObj->valuestring;
                    
//                     // Check if this is a light control action
//                     if (targetName == "redLight") {
//                         redLightVar = targetName;
//                         hasLedMapping = true;
//                     } else if (targetName == "yellowLight") {
//                         yellowLightVar = targetName;
//                         hasLedMapping = true;
//                     } else if (targetName == "greenLight") {
//                         greenLightVar = targetName;
//                         hasLedMapping = true;
//                     }
//                 }
//             }
//         }
//     }
    
//     if (hasLedMapping) {
//         ESP_LOGI(SFC_TAG, "Found LED mapping variables: red=%s, yellow=%s, green=%s", 
//                  redLightVar.c_str(), yellowLightVar.c_str(), greenLightVar.c_str());
//     } else {
//         ESP_LOGW(SFC_TAG, "No LED mapping variables found in SFC program");
//     }
    
//     // Set up context
//     context.steps = { steps.data(), steps.size() };
//     context.actions = { actions.data(), actions.size() };
//     context.transitions = { transitions.data(), transitions.size() };
    
//     return ErrorCode::OK;
// }

// sfc::predicate_fnc SfcAdapter::CreatePredicate(const char* condition) {
//     // For simple implementation, create a lambda that evaluates the condition
//     std::string condStr(condition);
    
//     return [this, condStr]() -> bool {
//         // Implement condition evaluation based on your variables
//         // Example: check if a binary variable with this name is true
//         auto it = boolVarMap.find(condStr);
//         if (it != boolVarMap.end()) {
//             return deviceManager->GetBinary(it->second);
//         }
        
//         // For numeric conditions, parse and evaluate
//         // This is a simple implementation - you'd want more robust parsing
        
//         // Default to true if condition can't be evaluated
//         return true;
//     };
// }

// void SfcAdapter::UpdateInputs() {
//     // Implement timers for the traffic light
//     static uint32_t lastTimerUpdate = 0;
//     uint32_t now = deviceManager->GetHAL()->GetMillis();
    
//     // Update timers every 1000ms (1 second)
//     if (now - lastTimerUpdate >= 1000) {
//         lastTimerUpdate = now;
        
//         // Simple timer implementation for demonstration
//         // In a real implementation, these would be more sophisticated timers
        
//         // Check for timer variables and toggle them if they exist
//         static int timerCounter = 0;
//         timerCounter++;
        
//         // Red phase timer (about 5 seconds)
//         auto redTimerIt = boolVarMap.find("redTimer");
//         if (redTimerIt != boolVarMap.end()) {
//             // Set to true after 5 seconds
//             bool timerDone = (timerCounter % 5 == 0);
//             deviceManager->SetBinary(redTimerIt->second, timerDone);
//         }
        
//         // Red-Yellow phase timer (about 2 seconds)
//         auto redYellowTimerIt = boolVarMap.find("redYellowTimer");
//         if (redYellowTimerIt != boolVarMap.end()) {
//             // Set to true after 2 seconds
//             bool timerDone = (timerCounter % 2 == 0);
//             deviceManager->SetBinary(redYellowTimerIt->second, timerDone);
//         }
        
//         // Green phase timer (about 5 seconds)
//         auto greenTimerIt = boolVarMap.find("greenTimer");
//         if (greenTimerIt != boolVarMap.end()) {
//             // Set to true after 5 seconds
//             bool timerDone = (timerCounter % 5 == 0);
//             deviceManager->SetBinary(greenTimerIt->second, timerDone);
//         }
        
//         // Yellow phase timer (about 2 seconds)
//         auto yellowTimerIt = boolVarMap.find("yellowTimer");
//         if (yellowTimerIt != boolVarMap.end()) {
//             // Set to true after 2 seconds
//             bool timerDone = (timerCounter % 2 == 0);
//             deviceManager->SetBinary(yellowTimerIt->second, timerDone);
//         }
//     }
// }

// void SfcAdapter::UpdateHardware() {
//     // Check if we have LED variable mappings
//     if (!hasLedMapping) {
//         return;
//     }
    
//     // Control LEDs based on SFC boolean variable states
//     bool redOn = false;
//     bool yellowOn = false;
//     bool greenOn = false;
    
//     // Get the current state of each LED from the DeviceManager
//     if (!redLightVar.empty()) {
//         auto it = boolVarMap.find(redLightVar);
//         if (it != boolVarMap.end()) {
//             redOn = deviceManager->GetBinary(it->second);
//         }
//     }
    
//     if (!yellowLightVar.empty()) {
//         auto it = boolVarMap.find(yellowLightVar);
//         if (it != boolVarMap.end()) {
//             yellowOn = deviceManager->GetBinary(it->second);
//         }
//     }
    
//     if (!greenLightVar.empty()) {
//         auto it = boolVarMap.find(greenLightVar);
//         if (it != boolVarMap.end()) {
//             greenOn = deviceManager->GetBinary(it->second);
//         }
//     }
    
//     // Control the physical LEDs using the HAL
//     // LED index 0 = red, 1 = yellow, 2 = green as per FB_RedLED, FB_YellowLED, FB_GreenLED
//     deviceManager->GetHAL()->ColorizeLed(0, redOn ? CRGB::DarkRed : CRGB::Black);
//     deviceManager->GetHAL()->ColorizeLed(1, yellowOn ? CRGB::Yellow : CRGB::Black);
//     deviceManager->GetHAL()->ColorizeLed(2, greenOn ? CRGB::DarkGreen : CRGB::Black);
    
//     ESP_LOGD(SFC_TAG, "Updated LED states: R=%d Y=%d G=%d", redOn, yellowOn, greenOn);
// }