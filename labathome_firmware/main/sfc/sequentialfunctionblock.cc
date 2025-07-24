#include "sequentialfunctionblock.hh"

SequentialFunctionBlocks::SequentialFunctionBlocks(DeviceManager* deviceManager)
    : deviceManager(deviceManager) {}

SequentialFunctionBlocks::~SequentialFunctionBlocks() {}

ErrorCode SequentialFunctionBlocks::LoadSfcFromFile(const char* filepath) {
    ESP_LOGI(SFC_TAG, "Loading SFC from file: %s", filepath);
    
    // First reset if we have an existing adapter
    if (adapter) {
        ESP_LOGI(SFC_TAG, "Resetting existing SFC adapter");
        adapter->Reset();
    } else {
        // Create a new adapter if none exists
        adapter = std::make_unique<SfcAdapter>(deviceManager);
    }
    
    // Give a small delay to ensure resources are cleaned up
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // Now load the new configuration
    ErrorCode result = adapter->LoadFromFile(filepath);
    initialized = (result == ErrorCode::OK);
    
    if (initialized) {
        ESP_LOGI(SFC_TAG, "SFC loaded successfully");
    } else {
        ESP_LOGE(SFC_TAG, "Failed to load SFC, error code: %d", static_cast<int>(result));
    }
    
    return result;
}

void SequentialFunctionBlocks::Tick(uint32_t ms) {
    if (initialized && adapter) adapter->ExecuteCycle(ms);
}

void SequentialFunctionBlocks::Reset() {
    if (adapter) adapter->Reset();
    initialized = false;
}