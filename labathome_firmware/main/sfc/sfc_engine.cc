#include "sfc_engine.hh"

namespace sfc {

SfcEngine::SfcEngine() : isInitialized(false) {
}

void SfcEngine::SetChart(std::shared_ptr<SfcChart> chart) {
    this->chart = chart;
    isInitialized = false;
}

void SfcEngine::SetContext(std::shared_ptr<SfcContext> context) {
    this->context = context;
}

ErrorCode SfcEngine::Initialize() {
    if (!chart) {
        ESP_LOGE(SFC_TAG, "No chart set for SFC engine");
        return ErrorCode::NOT_YET_INITIALIZED;
    }
    
    if (!context) {
        ESP_LOGE(SFC_TAG, "No context set for SFC engine");
        return ErrorCode::NOT_YET_INITIALIZED;
    }
    
    // Reset the chart and activate initial steps
    chart->Reset();
    chart->Initialize();
    
    isInitialized = true;
    ESP_LOGI(SFC_TAG, "SFC engine initialized with chart %s", chart->GetName().c_str());
    return ErrorCode::OK;
}

ErrorCode SfcEngine::ExecuteCycle() {
    if (!isInitialized) {
        ESP_LOGE(SFC_TAG, "SFC engine not initialized");
        return ErrorCode::NOT_YET_INITIALIZED;
    }
    
    // First, evaluate transitions and update active steps
    ErrorCode transResult = EvaluateTransitions();
    if (transResult != ErrorCode::OK) {
        return transResult;
    }
    
    // Then, execute actions for all steps
    ErrorCode actionResult = ExecuteActions();
    if (actionResult != ErrorCode::OK) {
        return actionResult;
    }
    
    // Finally, clear activation/deactivation flags
    for (const auto& [id, step] : chart->GetAllSteps()) {
        step->ClearFlags();
    }
    
    return ErrorCode::OK;
}

void SfcEngine::Reset() {
    if (chart) {
        chart->Reset();
    }
    isInitialized = false;
}

ErrorCode SfcEngine::EvaluateTransitions() {
    // Collect all active transitions
    std::vector<std::shared_ptr<SfcTransition>> activeTransitions;
    
    // For each transition, check if all source steps are active and the condition is true
    for (const auto& [id, transition] : chart->GetAllTransitions()) {
        bool allSourcesActive = true;
        
        for (const auto* sourceStep : transition->GetSources()) {
            if (!sourceStep->IsActive()) {
                allSourcesActive = false;
                break;
            }
        }
        
        if (allSourcesActive && transition->EvaluateCondition()) {
            activeTransitions.push_back(transition);
        }
    }
    
    // Activate target steps and deactivate source steps for each active transition
    for (const auto& transition : activeTransitions) {
        ESP_LOGD(SFC_TAG, "Transition %s is active", transition->GetId().c_str());
        
        // First, deactivate all source steps
        for (const auto* sourceStep : transition->GetSources()) {
            auto step = chart->GetStep(sourceStep->GetId());
            if (step) {
                step->SetActive(false);
                ESP_LOGD(SFC_TAG, "Deactivating step %s", step->GetId().c_str());
            }
        }
        
        // Then, activate all target steps
        for (const auto* targetStep : transition->GetTargets()) {
            auto step = chart->GetStep(targetStep->GetId());
            if (step) {
                step->SetActive(true);
                ESP_LOGD(SFC_TAG, "Activating step %s", step->GetId().c_str());
            }
        }
    }
    
    return ErrorCode::OK;
}

ErrorCode SfcEngine::ExecuteActions() {
    ErrorCode result = ErrorCode::OK;
    
    // Execute actions for all steps
    for (const auto& [id, step] : chart->GetAllSteps()) {
        ErrorCode stepResult = step->ExecuteActions(context->GetCurrentTime());
        if (stepResult != ErrorCode::OK) {
            result = stepResult;
        }
    }
    
    return result;
}

} // namespace sfc