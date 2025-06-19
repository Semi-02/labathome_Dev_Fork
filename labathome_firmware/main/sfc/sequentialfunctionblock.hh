#pragma once

#include "devicemanager.hh"
#include "sfc_adapter.hh"
#include <memory>

// Define SFC_TAG for logging
#define SFC_TAG "SFC"

class SequentialFunctionBlocks {
private:
    DeviceManager* deviceManager;
    std::unique_ptr<SfcAdapter> adapter;
    bool initialized;

public:
    SequentialFunctionBlocks(DeviceManager* deviceManager);
    ~SequentialFunctionBlocks();
        
    void Reset();
    ErrorCode LoadSfcFromFile(const char* filepath);
    ErrorCode ExecuteCycle();
    bool IsInitialized() const { return initialized; }
};