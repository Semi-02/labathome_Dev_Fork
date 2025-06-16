#pragma once

#include "sfc_components.hh"
#include "sfc_context.hh"
#include <memory>

namespace sfc {

class SfcEngine {
private:
    std::shared_ptr<SfcChart> chart;
    std::shared_ptr<SfcContext> context;
    bool isInitialized;
    
public:
    SfcEngine();
    ~SfcEngine() = default;
    
    // Set the chart to be executed
    void SetChart(std::shared_ptr<SfcChart> chart);
    
    // Set execution context
    void SetContext(std::shared_ptr<SfcContext> context);
    
    // Initialize the engine
    ErrorCode Initialize();
    
    // Execute one cycle of the SFC
    ErrorCode ExecuteCycle();
    
    // Reset the engine
    void Reset();
    
    // Check if engine is initialized
    bool IsInitialized() const { return isInitialized; }
    
private:
    // Evaluate transitions and update active steps
    ErrorCode EvaluateTransitions();
    
    // Execute actions for all steps
    ErrorCode ExecuteActions();
};

} // namespace sfc