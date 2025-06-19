#include "sequentialfunctionblock.hh"
#include "devicemanager.hh"
#include "sfc_adapter.hh"

SequentialFunctionBlocks::SequentialFunctionBlocks(DeviceManager* deviceManager) :
    deviceManager(deviceManager),
    initialized(false) {
    // Don't create the adapter yet - wait until LoadSfcFromFile is called
}

SequentialFunctionBlocks::~SequentialFunctionBlocks() {
    // The unique_ptr will automatically clean up the adapter
}

ErrorCode SequentialFunctionBlocks::LoadSfcFromFile(const char* filepath) {
    // Create the adapter if it doesn't exist
    if (!adapter) {
        adapter = std::make_unique<SfcAdapter>(deviceManager);
    }
    
    // Load the SFC file
    ErrorCode result = adapter->LoadFromFile(filepath);
    if (result == ErrorCode::OK) {
        initialized = true;
        ESP_LOGI(SFC_TAG, "SFC loaded successfully from %s", filepath);
    } else {
        ESP_LOGE(SFC_TAG, "Failed to load SFC from %s, error: %d", filepath, static_cast<int>(result));
    }
    
    return result;
}

ErrorCode SequentialFunctionBlocks::ExecuteCycle() {
    if (!initialized || !adapter) {
        return ErrorCode::NOT_YET_INITIALIZED;
    }
    
    return adapter->ExecuteCycle();
}

void SequentialFunctionBlocks::Reset() {
    if (adapter) {
        adapter->Reset();
    }
    initialized = false;
}