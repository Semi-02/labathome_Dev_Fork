#pragma once

#include "webmanager_interfaces.hh"
#include "flatbuffers/flatbuffers.h"
#include "../generated/flatbuffers_cpp/ns03functionblock_generated.h"
#include "cJSON.h"
#include "esp_err.h"
// #include "../sfc/sequentialfunctionblock.hh"  // Fix: changed from sequentialfunctionblocks.hh
#define TAG "SFC_PLUGIN"

// Define a namespace value for SFC messages - must match client-side value
#define SFC_NAMESPACE_VALUE 999 

using namespace webmanager;

class SequentialFunctionBlockPlugin : public webmanager::iWebmanagerPlugin
{
private:
    DeviceManager *devicemanager;
    
public:
    SequentialFunctionBlockPlugin(DeviceManager *devicemanager) : devicemanager(devicemanager) {}
    ~SequentialFunctionBlockPlugin() {
    }

    void OnBegin(webmanager::iWebmanagerCallback *callback) override {
        ESP_LOGI(TAG, "Sequential Function Block Plugin initialized");
    }

    void OnWifiConnect(webmanager::iWebmanagerCallback *callback) override { (void)(callback); }
    void OnWifiDisconnect(webmanager::iWebmanagerCallback *callback) override { (void)(callback); }
    void OnTimeUpdate(webmanager::iWebmanagerCallback *callback) override { (void)(callback); }
    
        webmanager::eMessageReceiverResult ProvideWebsocketMessage(webmanager::iWebmanagerCallback *callback, httpd_req_t *req, httpd_ws_frame_t *ws_pkt, uint32_t ns, uint8_t *buf) override
    {
        if (ns != SFC_NAMESPACE_VALUE) { return eMessageReceiverResult::NOT_FOR_ME; }
        auto rw = flatbuffers::GetRoot<functionblock::RequestWrapper>(buf);
        auto reqType = rw->request_type();
    
        switch (reqType) {
        case functionblock::Requests::Requests_RequestSFCRun: {
            // Use a static flag to prevent multiple requests in flight
            static bool loading_in_progress = false;
            
            if (loading_in_progress) {
                ESP_LOGW(TAG, "SFC loading already in progress, ignoring request");
                return webmanager::eMessageReceiverResult::OK;
            }
            
            loading_in_progress = true;
            ESP_LOGI(TAG, "Got Requests_RequestSFCRun");

            // First send response to client
            flatbuffers::FlatBufferBuilder b(256);
            b.Finish(
                functionblock::CreateResponseWrapper(
                    b,
                    functionblock::Responses::Responses_ResponseSFCRun,
                    functionblock::CreateResponseSFCRun(b).Union()
                )
            );
            callback->WrapAndSendAsync(SFC_NAMESPACE_VALUE, b);

            // Create a context structure to pass to the task
            struct LoadSfcTaskContext {
                DeviceManager* devicemanager;
                char filepath[128];
            };
            
            LoadSfcTaskContext* context = new LoadSfcTaskContext();
            context->devicemanager = devicemanager;
            strlcpy(context->filepath, TEMPSFC_FILEPATH, sizeof(context->filepath));
            
            // Then load SFC from file in a separate task
            ESP_LOGI(TAG, "Starting SFC loading task");
            TaskHandle_t task_handle = NULL;
            BaseType_t result = xTaskCreate(
                [](void* arg) {
                    LoadSfcTaskContext* ctx = static_cast<LoadSfcTaskContext*>(arg);
                    ESP_LOGI(TAG, "SFC Data process in task");
                    ErrorCode result = ctx->devicemanager->LoadSfcFromFile(ctx->filepath);
                    
                    if (result != ErrorCode::OK) {
                        ESP_LOGE(TAG, "Failed to load SFC, error code: %d", static_cast<int>(result));
                    }
                    
                    // Reset the static flag
                    loading_in_progress = false;
                    
                    // Clean up
                    delete ctx;
                    vTaskDelete(NULL);
                },
                "sfc_load",
                4096,  // Stack size
                context,
                5,     // Priority
                &task_handle
            );
            
            if (result != pdPASS) {
                ESP_LOGE(TAG, "Failed to create SFC loading task");
                delete context;
                loading_in_progress = false;
            }

            return webmanager::eMessageReceiverResult::OK;
        }

        case functionblock::Requests::Requests_RequestSFCStop: {
            ESP_LOGI(TAG, "Got Requests_RequestSFCStop - stopping SFC engine");
            
            // Create a task to stop the SFC in the background
            TaskHandle_t task_handle = NULL;
            BaseType_t result = xTaskCreate(
                [](void* arg) {
                    DeviceManager* dm = static_cast<DeviceManager*>(arg);
                    
                    // Set the experiment mode to function block to stop using the SFC
                    ESP_LOGI(TAG, "Setting experiment mode to function block");
                    
                    // Delete the SFC instance to clean up resources
                    if (dm->IsSfcLoaded()) {
                        ESP_LOGI(TAG, "Stopping and deleting SFC engine");
                        dm->UnloadSfc();  // We'll need to implement this method
                    } else {
                        ESP_LOGW(TAG, "No SFC was loaded, nothing to stop");
                    }
                    
                    vTaskDelete(NULL);
                },
                "sfc_stop",
                4096,  // Stack size
                devicemanager,
                5,     // Priority
                &task_handle
            );
            
            if (result != pdPASS) {
                ESP_LOGE(TAG, "Failed to create SFC stopping task");
            }
            
            // Send response to client
            flatbuffers::FlatBufferBuilder b(256);
            b.Finish(
                functionblock::CreateResponseWrapper(
                    b,
                    functionblock::Responses::Responses_ResponseSFCStop,
                    functionblock::CreateResponseSFCStop(b).Union()
                )
            );
            callback->WrapAndSendAsync(SFC_NAMESPACE_VALUE, b);
            
            return webmanager::eMessageReceiverResult::OK;
        }

        default:
            return webmanager::eMessageReceiverResult::FOR_ME_BUT_FAILED;
        }
    }
};
#undef TAG