#pragma once

#include "sfc_adapter.hh"
#include <memory>

#define SFC_TAG "SFC"

class DeviceManager; // Forward declaration

class SequentialFunctionBlocks {
private:
    DeviceManager* deviceManager;
    std::unique_ptr<SfcAdapter> adapter;
    bool initialized = false;

public:
    SequentialFunctionBlocks(DeviceManager* deviceManager);
    ~SequentialFunctionBlocks();

    ErrorCode LoadSfcFromFile(const char* filepath);
    void Tick(uint32_t ms);
    bool IsInitialized() const { 
        return initialized && adapter && adapter->IsInitialized(); 
    }
};