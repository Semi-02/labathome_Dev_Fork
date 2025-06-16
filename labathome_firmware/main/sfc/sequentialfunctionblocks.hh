#pragma once

#include "esp_log.h"
#include "errorcodes.hh"
#include <vector>
#include <cstring>
#include "devicemanager.hh"
#include "common.hh"
#include "common-esp32.hh"
#include "cJSON.h"
#include "esp_vfs.h"
#include "sfc/sfc_components.hh"
#include "sfc/sfc_engine.hh"
#include "sfc/sfc_parser.hh"
#include "sfc/sfc_context.hh"
#include <memory>

#define TAG "SFC"

class SequentialFunctionBlocks {
private:
    DeviceManager* deviceManager;
    cJSON* currentSfcData;
    bool isInitialized;
    
    // SFC components
    std::shared_ptr<sfc::SfcChart> chart;
    std::shared_ptr<sfc::SfcEngine> engine;
    std::shared_ptr<sfc::DeviceManagerContext> context;
    sfc::SfcParser parser;

public:
    SequentialFunctionBlocks(DeviceManager* deviceManager) : 
        deviceManager(deviceManager), 
        currentSfcData(nullptr),
        isInitialized(false) {
        
        // Create SFC engine and context
        engine = std::make_shared<sfc::SfcEngine>();
        context = std::make_shared<sfc::DeviceManagerContext>(deviceManager);
        
    }

    ~SequentialFunctionBlocks() {
        if (currentSfcData) {
            cJSON_Delete(currentSfcData);
        }
    }

    /**
     * @brief Loads and parses an SFC JSON file uploaded by the client
     * @param path Path to the SFC JSON file
     * @return ErrorCode with the operation status
     */
    ErrorCode LoadSfcFromFile(const char* path) {
        FILE *fd = NULL;
        struct stat file_stat;
        
        if (stat(path, &file_stat) == -1) {
            ESP_LOGI(TAG, "SFC file %s does not exist.", path);
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        fd = fopen(path, "r");
        if (!fd) {
            ESP_LOGE(TAG, "Failed to read existing file: %s", path);
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        ESP_LOGI(TAG, "Opening SFC file %s was successful. File Size is %ld bytes", path, file_stat.st_size);
        
        // Allocate memory for JSON string
        char* jsonBuffer = (char*)malloc(file_stat.st_size + 1);
        if (!jsonBuffer) {
            ESP_LOGE(TAG, "Memory allocation failed for JSON buffer");
            fclose(fd);
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Read the entire file
        size_t bytesRead = fread(jsonBuffer, 1, file_stat.st_size, fd);
        fclose(fd);
        
        if (bytesRead != file_stat.st_size) {
            ESP_LOGE(TAG, "Failed to read complete file content. Read %d of %ld bytes", bytesRead, file_stat.st_size);
            free(jsonBuffer);
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Ensure string is null-terminated
        jsonBuffer[bytesRead] = '\0';
        
        // Parse JSON
        cJSON* newSfcData = cJSON_Parse(jsonBuffer);
        free(jsonBuffer);
        
        if (!newSfcData) {
            ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Delete old data if present
        if (currentSfcData) {
            cJSON_Delete(currentSfcData);
        }
        
        // Store the new data
        currentSfcData = newSfcData;
        
        // Create SFC chart from JSON
        std::shared_ptr<sfc::SfcChart> newChart;
        ErrorCode parseResult = parser.ParseFromJsonObject(currentSfcData, newChart);
        if (parseResult != ErrorCode::OK) {
            ESP_LOGE(TAG, "Failed to parse SFC chart from JSON");
            return parseResult;
        }
        
        // Store the chart and initialize the engine
        chart = newChart;
        engine->SetChart(chart);
        engine->SetContext(context);
        ErrorCode initResult = engine->Initialize();
        if (initResult != ErrorCode::OK) {
            ESP_LOGE(TAG, "Failed to initialize SFC engine");
            return initResult;
        }
        
        isInitialized = true;
        ESP_LOGI(TAG, "SFC data successfully loaded and parsed");
        
        // Log the content (first level)
        LogSfcData();
        
        return ErrorCode::OK;
    }
    
    /**
     * @brief Execute one cycle of the SFC program
     * @return ErrorCode with the operation status
     */
    ErrorCode ExecuteCycle() {
        if (!isInitialized) {
            ESP_LOGW(TAG, "SFC not initialized, cannot execute cycle");
            return ErrorCode::NOT_YET_INITIALIZED;
        }
        
        return engine->ExecuteCycle();
    }
    
    
    /**
     * @brief Log the structure of the loaded SFC in the console
     */
    void LogSfcData() {
        if (!currentSfcData) {
            ESP_LOGW(TAG, "No SFC data available for logging");
            return;
        }
        
        char* jsonStr = cJSON_Print(currentSfcData);
        if (jsonStr) {
            ESP_LOGI(TAG, "SFC Data: %s", jsonStr);
            free(jsonStr);
        }
    }
    
    /**
     * @brief Check if SFC is initialized
     */
    bool IsInitialized() const {
        return isInitialized;
    }
    
    /**
     * @brief Reset the SFC engine
     */
    void Reset() {
        if (engine) {
            engine->Reset();
            isInitialized = false;
        }
    }
};

#undef TAG