#pragma once

#include "webmanager_interfaces.hh"
#include "flatbuffers/flatbuffers.h"
#include "../generated/flatbuffers_cpp/ns03functionblock_generated.h"
#include "cJSON.h"
#include "esp_err.h"
#define TAG "SFC_PLUGIN"

// Define a namespace value for SFC messages - must match client-side value
#define SFC_NAMESPACE_VALUE 999 

using namespace webmanager;

class SequentialFunctionBlockPlugin : public webmanager::iWebmanagerPlugin
{
private:
    DeviceManager *devicemanager;

    esp_err_t ProcessSfcData(const char* jsonData, flatbuffers::FlatBufferBuilder& responseBuilder) {
        // Parse the JSON data
        cJSON *json = cJSON_Parse(jsonData);
        if (json == NULL) {
            responseBuilder.Clear();
            return ESP_ERR_INVALID_ARG; 
        }
        
        // Validate the SFC structure
        cJSON *startNode = cJSON_GetObjectItem(json, "start");
        cJSON *steps = cJSON_GetObjectItem(json, "steps");
        cJSON *booleans = cJSON_GetObjectItem(json, "booleans");
        
        if (!startNode || !steps || !booleans) {
            // Create error response for invalid structure
            responseBuilder.Clear();
            cJSON_Delete(json);
            return ESP_ERR_INVALID_ARG;
        }
        
        // Process the SFC data
        int stepCount = cJSON_GetArraySize(steps);
        ESP_LOGI(TAG, "SFC structure with %d steps received", stepCount);
        

        // devicemanager->ProcessSfcData(json);
        
        responseBuilder.Clear();
        cJSON_Delete(json);
        return ESP_OK;
    }

public:
    SequentialFunctionBlockPlugin(DeviceManager *devicemanager) : devicemanager(devicemanager) {}

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
            ESP_LOGI(TAG, "Got Requests_RequestSFCRun");
            const auto *request = rw->request_as_RequestSFCRun();
            const char *sfcData = request->sfc_data()->c_str();
            ESP_LOGI(TAG, "SFC Data: %s", sfcData);
    
            // Verarbeiten Sie die SFC-Daten hier
            flatbuffers::FlatBufferBuilder b(256);
            b.Finish(
                functionblock::CreateResponseWrapper(
                    b,
                    functionblock::Responses::Responses_ResponseSFCRun,
                    functionblock::CreateResponseSFCRun(b).Union()
                )
            );
            callback->WrapAndSendAsync(SFC_NAMESPACE_VALUE, b);
            ESP_LOGI(TAG, "SFC Data process");
            return webmanager::eMessageReceiverResult::OK;
        }
        default:
            return webmanager::eMessageReceiverResult::FOR_ME_BUT_FAILED;
        }
    }
};
#undef TAG