//#define HTTP
#define HTTPS

//c++ lib incudes
#include <cstdio>
#include <cstring>
#include <vector>

//FreeRTOS & Lwip includes
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

//opc ua include
#include <open62541.h> // Include für die kompilierten Header

//esp idf includes
#include <esp_system.h>
#include <esp_log.h>
#include <esp_littlefs.h>
#include <spi_flash_mmap.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#ifdef HTTPS
#include <esp_https_server.h>
#ifndef CONFIG_ESP_HTTPS_SERVER_ENABLE
#error "Enable HTTPS_SERVER in menuconfig!"
#endif
#endif
#ifdef HTTP
#include <esp_http_server.h>
#endif

//klaus-liebler component components
#include <common-esp32.hh>
#include <webmanager.hh>
#include <runtimeconfig_cpp/runtimeconfig.hh>

constexpr TickType_t xFrequency {pdMS_TO_TICKS(50)};

//board specific includes
#include "hal_impl.hh"
static iHAL * hal = new HAL_Impl();

static const char *TAG = "main";
#include "devicemanager.hh"
#include "webmanager_plugins/heaterexperiment_plugin.hh"
#include "webmanager_plugins/functionblock_plugin.hh"
#include "webmanager_plugins/systeminfo_plugin.hh"
#include "webmanager_plugins/usersettings_plugin.hh"

DeviceManager *devicemanager{nullptr};
httpd_handle_t http_server{nullptr};


constexpr const char* NVS_PARTITION_NAME{NVS_DEFAULT_PART_NAME};

FLASH_FILE(esp32_pem_crt);
FLASH_FILE(esp32_pem_key);

// Funktion für den OPC UA-Server
void StartOPCUAServer() {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    // Debug-Logs aktivieren
    config->logger = *UA_Log_Stdout;
    //config->logLevel = UA_LOGLEVEL_TRACE;

    // Hostname explizit setzen
    config->customHostname = UA_STRING_ALLOC("opc.tcp://192.168.4.1:4840");

    // Eine Variable (Node) hinzufügen
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Double myValue = 42.0; // Beispielwert
    UA_Variant_setScalar(&attr.value, &myValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"ist in 78 Stunden.");
    attr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)"Schützenfest");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId myNodeId = UA_NODEID_STRING(1, (char*)"schuetzenfest");
    UA_QualifiedName myName = UA_QUALIFIEDNAME(1, (char*)"schuetzenfest");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_Server_addVariableNode(server, myNodeId, parentNodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              myName, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, nullptr, nullptr);

    // Server starten
    ESP_LOGI("OPCUA", "Starting OPC UA Server...");
    UA_Boolean running = true;
    UA_StatusCode retval = UA_Server_run(server, &running);
    if (retval != UA_STATUSCODE_GOOD) {
        ESP_LOGE("OPCUA", "Failed to start OPC UA Server: %s", UA_StatusCode_name(retval));
    }

    UA_Server_delete(server);
}

#include "open62541.h"

#define OPC_UA_TAG "OPCUA"
#define OPCUA_TASK_STACK_SIZE 16384  // 16 KB Stack für OPC UA Task
#define OPCUA_TASK_PRIORITY   5      // Priorität im normalen Bereich

static void opcua_server_task(void *pvParameters) {
    ESP_LOGI(OPC_UA_TAG, "Starte OPC UA Task…");

    UA_Server *server = UA_Server_new();
    if(!server) {
        ESP_LOGE(OPC_UA_TAG, "Fehler beim Erstellen des OPC UA Servers");
        vTaskDelete(NULL);
        return;
    }

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    // Beispiel: Füge eine Variable im Address Space hinzu
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "myInteger");
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "MyInteger");

    UA_StatusCode addStatus = UA_Server_addVariableNode(
        server,
        myIntegerNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), // Parent Node: Objects folder
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),     // ReferenceType
        UA_QUALIFIEDNAME(1, "MyInteger"),             // BrowseName
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr,
        NULL,
        NULL);

    if(addStatus != UA_STATUSCODE_GOOD) {
        ESP_LOGE(OPC_UA_TAG, "Variable konnte nicht hinzugefügt werden: 0x%08x", (unsigned int)addStatus);
    }

    UA_StatusCode rc = UA_Server_run_startup(server);
    if(rc != UA_STATUSCODE_GOOD) {
        ESP_LOGE(OPC_UA_TAG, "Startup-Fehler: 0x%08x", (unsigned int)rc);
        UA_Server_delete(server);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(OPC_UA_TAG, "OPC UA Server läuft auf opc.tcp://<esp32-ip>:4840");

    while(true) {
        // True = blockierend, besser bei FreeRTOS aber nicht für längere Blocking calls
        // Besser: false + kurze Delays (oder kein Delay)
        UA_Server_run_iterate(server, false);
        vTaskDelay(pdMS_TO_TICKS(10)); // 10 ms Delay zum Yield
    }

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    vTaskDelete(NULL);
}


extern "C" void app_main()
{
    // Configure Logging
    //esp_log_level_set(TAG, ESP_LOG_INFO);
    //esp_log_level_set("esp_https_server", ESP_LOG_WARN);
    ESP_LOGI(TAG, "\n%s", cfg::BANNER);
    ESP_LOGI(TAG, "%s is booting up. Firmware build at %s on Git %s", cfg::BOARD_NAME, cfg::CREATION_DT_STR, cfg::GIT_SHORT_HASH);

    // Configure NVS and SPIFFS
    size_t total = 0, used = 0;
    esp_vfs_littlefs_conf_t conf = {"/spiffs", "storage", nullptr, 0,0,0,0};
    ESP_ERROR_CHECK(esp_vfs_littlefs_register(&conf));
    ESP_ERROR_CHECK(esp_littlefs_info(conf.partition_label, &total, &used));
    ESP_LOGI(TAG, "LittleFS Partition successfully mounted: total: %dbyte, used: %dbyte", total, used);
    ESP_ERROR_CHECK(nvs_flash_init_and_erase_lazily(NVS_PARTITION_NAME));

    //Install Temperature sensor
    //Temperature Sensor is used in generic hal for generic use and is used in the SystemPlugin
    temperature_sensor_handle_t tempHandle;
    temperature_sensor_config_t temp_sensor_config = {-10, 80, TEMPERATURE_SENSOR_CLK_SRC_DEFAULT, {0}};
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &tempHandle));
    ESP_ERROR_CHECK(temperature_sensor_enable(tempHandle));

    //Generating deviceManager
    devicemanager = new DeviceManager(hal);
    
    std::vector<webmanager::iWebmanagerPlugin*> plugins;
    plugins.push_back(new HeaterExperimentPlugin(devicemanager));
    plugins.push_back(new FunctionblockPlugin(devicemanager));
    plugins.push_back(new SystemInfoPlugin(tempHandle));
    plugins.push_back(new UsersettingsPlugin("nvs"));
    
    
    //Configure Network
    webmanager::M* wm = webmanager::M::GetSingleton();
    ESP_ERROR_CHECK(wm->Begin(cfg::HOSTNAME, "labathome", cfg::HOSTNAME, false, &plugins, true));

    const char *hostname = wm->GetHostname();
#ifdef HTTPS
    httpd_ssl_config_t httpd_conf = HTTPD_SSL_CONFIG_DEFAULT();
    httpd_conf.servercert = esp32_pem_crt_start;
    httpd_conf.servercert_len = esp32_pem_crt_end-esp32_pem_crt_start;
    httpd_conf.prvtkey_pem = esp32_pem_key_start;
    httpd_conf.prvtkey_len = esp32_pem_key_end-esp32_pem_key_start;
    httpd_conf.httpd.uri_match_fn = httpd_uri_match_wildcard;
    httpd_conf.httpd.max_uri_handlers = 15;
    ESP_ERROR_CHECK(httpd_ssl_start(&http_server, &httpd_conf));
    ESP_LOGI(TAG, "HTTPS Server listening on https://%s:%d", hostname, httpd_conf.port_secure);
#elif defined(HTTP)
    httpd_config_t httpd_conf = HTTPD_DEFAULT_CONFIG();
    httpd_conf.uri_match_fn = httpd_uri_match_wildcard;
    httpd_conf.max_uri_handlers = 15;
    ESP_ERROR_CHECK(httpd_start(&http_server, &httpd_conf));
    ESP_LOGI(TAG, "HTTP Server (not secure!) listening on http://%s:%d", hostname, httpd_conf.server_port);
#else
    #error "Either define HTTP or HTTPS"
#endif

    // Start all managers
    
    hal->InitAndRun();
    devicemanager->InitAndRun();
    ESP_LOGI(TAG, "RED %d YEL %d GRN %d", hal->GetButtonRedIsPressed(), hal->GetButtonEncoderIsPressed(), hal->GetButtonGreenIsPressed());

    // Allow Browser Access
    //the "sensor" endpoint is used to get the sensor data as JSON
    //register this before the webmanager, because the webmanager has a wildcard handler
    httpd_uri_t sensors_get = {
        "/sensors", 
        HTTP_GET, 
        [](httpd_req_t *req){
            size_t l=2048;
            char *buf= new char[l];
            devicemanager->GetHAL()->GetSensorsAsJSON(buf, l);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, buf, l);
            delete[] buf;
            return ESP_OK;
        }, 
        nullptr, false, false, nullptr
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &sensors_get));

    wm->RegisterHTTPDHandlers(http_server);

    wm->CallMeAfterInitializationToMarkCurrentPartitionAsValid();

<<<<<<< Updated upstream
    // Start OPC UA Server
    StartOPCUAServer();
=======
    xTaskCreate(opcua_server_task, "opcua_task", OPCUA_TASK_STACK_SIZE, NULL, OPCUA_TASK_PRIORITY, NULL);

>>>>>>> Stashed changes

    // Start eternal supervisor loop
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true)
    {
        xTaskDelayUntil(&xLastWakeTime, xFrequency);
        hal->DoMonitoring();
        
    }
}
