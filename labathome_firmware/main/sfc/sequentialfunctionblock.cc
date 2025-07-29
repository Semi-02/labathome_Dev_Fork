#include "sequentialfunctionblock.hh"

SequentialFunctionBlocks::SequentialFunctionBlocks(DeviceManager* deviceManager)
    : deviceManager(deviceManager) {
    ESP_LOGI(SFC_TAG, "SequentialFunctionBlocks constructor called");
    
    // Erstelle sofort einen neuen Adapter
    adapter = std::make_unique<SfcAdapter>(deviceManager);
    ESP_LOGI(SFC_TAG, "SfcAdapter created in SequentialFunctionBlocks constructor");
}

SequentialFunctionBlocks::~SequentialFunctionBlocks() {
    ESP_LOGI(SFC_TAG, "SequentialFunctionBlocks destructor called");
    
    initialized = false;

    if (adapter) {
        ESP_LOGI(SFC_TAG, "Releasing SFC adapter");
        adapter.reset(); // This will call SfcAdapter's destructor
    }
    
    ESP_LOGI(SFC_TAG, "SequentialFunctionBlocks destructor completed");
}

ErrorCode SequentialFunctionBlocks::LoadSfcFromFile(const char* filepath) {
    ESP_LOGI(SFC_TAG, "Loading SFC from file: %s", filepath);
    
    // JSON zur Application wandeln 
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