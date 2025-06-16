#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include "errorcodes.hh"
#include "esp_log.h"

namespace sfc {

#define SFC_TAG "SFC"

// Forward declarations
class SfcStep;
class SfcTransition;
class SfcAction;
class SfcChart;

// Action qualifiers as defined in IEC 61131-3
enum class ActionQualifier {
    N,  // Non-stored (active as long as the step)
    R,  // Overriding Reset
    S,  // Set (Stored)
    L,  // Time Limited
    D,  // Time Delayed
    P,  // Pulse
    SD, // Stored and time Delayed
    DS, // Delayed and Stored
    SL  // Stored and time Limited
};

// Class representing an action within a step
class SfcAction {
private:
    std::string name;                // Action name/identifier
    ActionQualifier qualifier;       // Action qualifier
    uint32_t timeValue;              // Time value for timed qualifiers (in ms)
    int64_t activationTime;          // When the action was activated
    bool isActive;                   // Current action state
    
public:
    SfcAction(const std::string& name, ActionQualifier qualifier, uint32_t timeValue = 0);
    ~SfcAction() = default;
    
    const std::string& GetName() const { return name; }
    ActionQualifier GetQualifier() const { return qualifier; }
    uint32_t GetTimeValue() const { return timeValue; }
    bool IsActive() const { return isActive; }
    
    // Execute the action based on its qualifier and step state
    ErrorCode Execute(bool stepActive, bool stepActivated, bool stepDeactivated, int64_t currentTime);
    void Reset();
};

// Class representing a transition between steps
class SfcTransition {
private:
    std::string id;                  // Transition identifier
    std::string condition;           // Condition expression (to be evaluated)
    std::vector<SfcStep*> sources;   // Source steps (steps above)
    std::vector<SfcStep*> targets;   // Target steps (steps below)
    
public:
    SfcTransition(const std::string& id, const std::string& condition);
    ~SfcTransition() = default;
    
    const std::string& GetId() const { return id; }
    const std::string& GetCondition() const { return condition; }
    
    void AddSource(SfcStep* step) { sources.push_back(step); }
    void AddTarget(SfcStep* step) { targets.push_back(step); }
    
    const std::vector<SfcStep*>& GetSources() const { return sources; }
    const std::vector<SfcStep*>& GetTargets() const { return targets; }
    
    bool EvaluateCondition() const;  // Evaluate transition condition
};

// Class representing a step in the SFC
class SfcStep {
private:
    std::string id;                  // Step identifier
    bool isInitial;                  // Is this an initial step?
    bool isActive;                   // Current state of the step
    bool wasActivated;               // Activated in current cycle
    bool wasDeactivated;             // Deactivated in current cycle
    std::vector<std::shared_ptr<SfcAction>> actions;  // Actions associated with the step
    std::vector<SfcTransition*> incomingTransitions;  // Transitions leading to this step
    std::vector<SfcTransition*> outgoingTransitions;  // Transitions leaving this step
    
public:
    SfcStep(const std::string& id, bool isInitial = false);
    ~SfcStep() = default;
    
    const std::string& GetId() const { return id; }
    bool IsInitial() const { return isInitial; }
    bool IsActive() const { return isActive; }
    bool WasActivated() const { return wasActivated; }
    bool WasDeactivated() const { return wasDeactivated; }
    
    void SetActive(bool active);
    void AddAction(std::shared_ptr<SfcAction> action);
    void AddIncomingTransition(SfcTransition* transition);
    void AddOutgoingTransition(SfcTransition* transition);
    
    const std::vector<std::shared_ptr<SfcAction>>& GetActions() const { return actions; }
    const std::vector<SfcTransition*>& GetIncomingTransitions() const { return incomingTransitions; }
    const std::vector<SfcTransition*>& GetOutgoingTransitions() const { return outgoingTransitions; }
    
    // Execute all actions associated with this step
    ErrorCode ExecuteActions(int64_t currentTime);
    
    // Clear activation/deactivation flags at the end of cycle
    void ClearFlags();
};

// Class representing an entire SFC chart/program
class SfcChart {
private:
    std::string name;
    std::unordered_map<std::string, std::shared_ptr<SfcStep>> steps;
    std::unordered_map<std::string, std::shared_ptr<SfcTransition>> transitions;
    std::vector<std::shared_ptr<SfcStep>> initialSteps;
    
public:
    SfcChart(const std::string& name);
    ~SfcChart() = default;
    
    const std::string& GetName() const { return name; }
    
    // Create and add a step to the chart
    std::shared_ptr<SfcStep> CreateStep(const std::string& id, bool isInitial = false);
    
    // Create and add a transition to the chart
    std::shared_ptr<SfcTransition> CreateTransition(const std::string& id, const std::string& condition);
    
    // Connect a transition between source and target steps
    ErrorCode ConnectTransition(const std::string& transitionId, 
                                const std::vector<std::string>& sourceStepIds,
                                const std::vector<std::string>& targetStepIds);
    
    // Get step by ID
    std::shared_ptr<SfcStep> GetStep(const std::string& id) const;
    
    // Get transition by ID
    std::shared_ptr<SfcTransition> GetTransition(const std::string& id) const;
    
    // Get all initial steps
    const std::vector<std::shared_ptr<SfcStep>>& GetInitialSteps() const { return initialSteps; }
    
    // Get all steps
    const std::unordered_map<std::string, std::shared_ptr<SfcStep>>& GetAllSteps() const { return steps; }
    
    // Get all transitions
    const std::unordered_map<std::string, std::shared_ptr<SfcTransition>>& GetAllTransitions() const { return transitions; }
    
    // Reset the chart (deactivate all steps except initial ones)
    void Reset();
    
    // Initialize the chart (activate initial steps)
    void Initialize();
};

} // namespace sfc