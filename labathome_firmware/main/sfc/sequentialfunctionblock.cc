#include "sequentialfunctionblock.hh"

SequentialFunctionBlocks::SequentialFunctionBlocks(DeviceManager* deviceManager)
    : deviceManager(deviceManager) {}

SequentialFunctionBlocks::~SequentialFunctionBlocks() {}

ErrorCode SequentialFunctionBlocks::LoadSfcFromFile(const char* filepath) {
    if (!adapter) adapter = std::make_unique<SfcAdapter>(deviceManager);
    ErrorCode result = adapter->LoadFromFile(filepath);
    initialized = (result == ErrorCode::OK);
    return result;
}

void SequentialFunctionBlocks::Tick(uint32_t ms) {
    if (initialized && adapter) adapter->ExecuteCycle(ms);
}

void SequentialFunctionBlocks::Reset() {
    if (adapter) adapter->Reset();
    initialized = false;
}