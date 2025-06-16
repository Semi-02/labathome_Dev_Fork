#include "sfc_components.hh"

namespace sfc {

// SfcAction implementation
SfcAction::SfcAction(const std::string& name, ActionQualifier qualifier, uint32_t timeValue)
    : name(name), qualifier(qualifier), timeValue(timeValue), activationTime(-1), isActive(false) {
}

ErrorCode SfcAction::Execute(bool stepActive, bool stepActivated, bool stepDeactivated, int64_t currentTime) {
    switch (qualifier) {
        case ActionQualifier::N:  // Non-stored
            isActive = stepActive;
            break;
            
        case ActionQualifier::S:  // Set (Stored)
            if (stepActivated) {
                isActive = true;
            }
            break;
            
        case ActionQualifier::R:  // Reset
            if (stepActivated) {
                isActive = false;
            }
            break;
            
        case ActionQualifier::P:  // Pulse
            isActive = stepActivated || stepDeactivated;
            break;
            
        case ActionQualifier::L:  // Time Limited
            if (stepActivated) {
                isActive = true;
                activationTime = currentTime;
            } else if (stepActive) {
                // Check if time limit has been reached
                if (currentTime - activationTime >= timeValue) {
                    isActive = false;
                }
            } else {
                isActive = false;
            }
            break;
            
        case ActionQualifier::D:  // Time Delayed
            if (stepActivated) {
                activationTime = currentTime;
                isActive = false;
            } else if (stepActive) {
                // Check if delay time has elapsed
                if (currentTime - activationTime >= timeValue) {
                    isActive = true;
                }
            } else {
                isActive = false;
            }
            break;
            
        case ActionQualifier::SD: // Stored and time Delayed
            if (stepActivated) {
                activationTime = currentTime;
                isActive = false;
            } else if (activationTime > 0) {
                // Check if delay time has elapsed
                if (currentTime - activationTime >= timeValue) {
                    isActive = true;
                }
            }
            break;
            
        case ActionQualifier::DS: // Delayed and Stored
            if (stepActivated) {
                activationTime = currentTime;
                isActive = false;
            } else if (stepActive && activationTime > 0) {
                // Check if delay time has elapsed
                if (currentTime - activationTime >= timeValue) {
                    isActive = true;
                }
            }
            break;
            
        case ActionQualifier::SL: // Stored and time limited
            if (stepActivated) {
                isActive = true;
                activationTime = currentTime;
            } else if (isActive && activationTime > 0) {
                // Check if time limit has been reached
                if (currentTime - activationTime >= timeValue) {
                    isActive = false;
                }
            }
            break;
            
        default:
            ESP_LOGE(SFC_TAG, "Unknown action qualifier");
            return ErrorCode::INVALID_NEW_FBD;
    }
    
    return ErrorCode::OK;
}

void SfcAction::Reset() {
    isActive = false;
    activationTime = -1;
}

// SfcTransition implementation
SfcTransition::SfcTransition(const std::string& id, const std::string& condition)
    : id(id), condition(condition) {
}

bool SfcTransition::EvaluateCondition() const {
    // In a real implementation, you would parse and evaluate the condition
    // For now, just return true to make transitions always active
    ESP_LOGD(SFC_TAG, "Evaluating transition %s with condition %s", id.c_str(), condition.c_str());
    return true;
}

// SfcStep implementation
SfcStep::SfcStep(const std::string& id, bool isInitial)
    : id(id), isInitial(isInitial), isActive(false), wasActivated(false), wasDeactivated(false) {
}

void SfcStep::SetActive(bool active) {
    if (active && !isActive) {
        wasActivated = true;
    } else if (!active && isActive) {
        wasDeactivated = true;
    }
    isActive = active;
}

void SfcStep::AddAction(std::shared_ptr<SfcAction> action) {
    actions.push_back(action);
}

void SfcStep::AddIncomingTransition(SfcTransition* transition) {
    incomingTransitions.push_back(transition);
}

void SfcStep::AddOutgoingTransition(SfcTransition* transition) {
    outgoingTransitions.push_back(transition);
}

ErrorCode SfcStep::ExecuteActions(int64_t currentTime) {
    ErrorCode result = ErrorCode::OK;
    for (auto& action : actions) {
        ErrorCode actionResult = action->Execute(isActive, wasActivated, wasDeactivated, currentTime);
        if (actionResult != ErrorCode::OK) {
            result = actionResult;
        }
    }
    return result;
}

void SfcStep::ClearFlags() {
    wasActivated = false;
    wasDeactivated = false;
}

// SfcChart implementation
SfcChart::SfcChart(const std::string& name) : name(name) {
}

std::shared_ptr<SfcStep> SfcChart::CreateStep(const std::string& id, bool isInitial) {
    auto step = std::make_shared<SfcStep>(id, isInitial);
    steps[id] = step;
    if (isInitial) {
        initialSteps.push_back(step);
    }
    return step;
}

std::shared_ptr<SfcTransition> SfcChart::CreateTransition(const std::string& id, const std::string& condition) {
    auto transition = std::make_shared<SfcTransition>(id, condition);
    transitions[id] = transition;
    return transition;
}

ErrorCode SfcChart::ConnectTransition(const std::string& transitionId, 
                                     const std::vector<std::string>& sourceStepIds,
                                     const std::vector<std::string>& targetStepIds) {
    auto transition = GetTransition(transitionId);
    if (!transition) {
        ESP_LOGE(SFC_TAG, "Transition %s not found", transitionId.c_str());
        return ErrorCode::FILE_SYSTEM_ERROR;
    }
    
    for (const auto& sourceId : sourceStepIds) {
        auto sourceStep = GetStep(sourceId);
        if (!sourceStep) {
            ESP_LOGE(SFC_TAG, "Source step %s not found", sourceId.c_str());
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        transition->AddSource(sourceStep.get());
        sourceStep->AddOutgoingTransition(transition.get());
    }
    
    for (const auto& targetId : targetStepIds) {
        auto targetStep = GetStep(targetId);
        if (!targetStep) {
            ESP_LOGE(SFC_TAG, "Target step %s not found", targetId.c_str());
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        transition->AddTarget(targetStep.get());
        targetStep->AddIncomingTransition(transition.get());
    }
    
    return ErrorCode::OK;
}

std::shared_ptr<SfcStep> SfcChart::GetStep(const std::string& id) const {
    auto it = steps.find(id);
    if (it != steps.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<SfcTransition> SfcChart::GetTransition(const std::string& id) const {
    auto it = transitions.find(id);
    if (it != transitions.end()) {
        return it->second;
    }
    return nullptr;
}

void SfcChart::Reset() {
    for (const auto& [id, step] : steps) {
        step->SetActive(step->IsInitial());
    }
}

void SfcChart::Initialize() {
    for (const auto& step : initialSteps) {
        step->SetActive(true);
    }
}

} // namespace sfc