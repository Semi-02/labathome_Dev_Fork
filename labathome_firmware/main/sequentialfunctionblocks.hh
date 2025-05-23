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

#define TAG "SFC"

class SequentialFunctionBlocks {
private:
    DeviceManager* deviceManager;
    cJSON* currentSfcData;
    bool isInitialized;

public:
    SequentialFunctionBlocks(DeviceManager* deviceManager) : 
        deviceManager(deviceManager), 
        currentSfcData(nullptr),
        isInitialized(false) {}

    ~SequentialFunctionBlocks() {
        if (currentSfcData) {
            cJSON_Delete(currentSfcData);
        }
    }

    /**
     * @brief Lädt und parst eine SFC-JSON-Datei, die vom Client hochgeladen wurde
     * @param path Pfad zur SFC-JSON-Datei
     * @return ErrorCode mit dem Status der Operation
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
        
        // Allokiere Speicher für den JSON-String
        char* jsonBuffer = (char*)malloc(file_stat.st_size + 1);
        if (!jsonBuffer) {
            ESP_LOGE(TAG, "Memory allocation failed for JSON buffer");
            fclose(fd);
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Lese die Datei komplett ein
        size_t bytesRead = fread(jsonBuffer, 1, file_stat.st_size, fd);
        fclose(fd);
        
        if (bytesRead != file_stat.st_size) {
            ESP_LOGE(TAG, "Failed to read complete file content. Read %d of %ld bytes", bytesRead, file_stat.st_size);
            free(jsonBuffer);
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Stelle sicher, dass der String null-terminiert ist
        jsonBuffer[bytesRead] = '\0';
        
        // Parse JSON
        cJSON* newSfcData = cJSON_Parse(jsonBuffer);
        free(jsonBuffer);
        
        if (!newSfcData) {
            ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
            return ErrorCode::FILE_SYSTEM_ERROR;
        }
        
        // Lösche alte Daten, falls vorhanden
        if (currentSfcData) {
            cJSON_Delete(currentSfcData);
        }
        
        // Speichere die neuen Daten
        currentSfcData = newSfcData;
   
        isInitialized = true;
        ESP_LOGI(TAG, "SFC data successfully loaded and parsed");
        
        // Gib den Inhalt in der Konsole aus (erster Level)
        LogSfcData();
        
        return ErrorCode::OK;
    }
    
    /**
     * @brief Gibt die Struktur des geladenen SFC in der Konsole aus
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

};