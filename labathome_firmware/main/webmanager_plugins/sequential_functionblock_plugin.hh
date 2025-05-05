#pragma once

#include "webmanager_interfaces.hh"
#include "esp_http_server.h"
#include "cJSON.h"
#define TAG "SFC_PLUGIN"

using namespace webmanager;

class SequentialFunctionBlockPlugin : public webmanager::iWebmanagerPlugin
{
private:
    DeviceManager *devicemanager;

      static esp_err_t handle_sfc_data(httpd_req_t *req) {
        // Setze CORS-Header
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "http://localhost");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
        // OPTIONS-Anfragen direkt beantworten
        if (req->method == HTTP_OPTIONS) {
            httpd_resp_sendstr(req, "CORS Preflight");
            return ESP_OK;
        }
    
        // Puffer für die empfangenen JSON-Daten
        char buffer[1024];
        int received = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
    
        buffer[received] = '\0'; // Null-terminiere den Puffer
    
        // Parse die JSON-Daten
        cJSON *json = cJSON_Parse(buffer);
        if (json == NULL) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Ungültiges JSON");
            return ESP_FAIL;
        }
    
        // Debug-Ausgabe der empfangenen Daten
        ESP_LOGI(TAG, "Empfangene SfcData: %s", buffer);
    
        // JSON weiterverarbeiten (z. B. speichern oder analysieren)
        cJSON_Delete(json);
    
        // Erfolgsantwort senden
        httpd_resp_sendstr(req, "SfcData erfolgreich empfangen");
        return ESP_OK;
    }

public:
    SequentialFunctionBlockPlugin(DeviceManager *devicemanager) : devicemanager(devicemanager) {}

    void OnBegin(webmanager::iWebmanagerCallback *callback) override
    {
        // HTTP-Server konfigurieren
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = 8090; // Sicherer, nicht genutzter Port

        httpd_handle_t server = nullptr;
        if (httpd_start(&server, &config) == ESP_OK)
        {
            // URI-Handler registrieren
            httpd_uri_t sfc_data_uri = {
                .uri = "/sfc-data",
                .method = HTTP_POST,
                .handler = handle_sfc_data,
                .user_ctx = nullptr,
            };
            httpd_register_uri_handler(server, &sfc_data_uri);

            ESP_LOGI(TAG, "Sequential Function Block Plugin gestartet und hört auf Port %d", config.server_port);
        }
        else
        {
            ESP_LOGE(TAG, "Fehler beim Starten des HTTP-Servers für das Sequential Function Block Plugin");
        }
    }

    void OnWifiConnect(webmanager::iWebmanagerCallback *callback) override { (void)(callback); }
    void OnWifiDisconnect(webmanager::iWebmanagerCallback *callback) override { (void)(callback); }
    void OnTimeUpdate(webmanager::iWebmanagerCallback *callback) override { (void)(callback); }
    webmanager::eMessageReceiverResult ProvideWebsocketMessage(webmanager::iWebmanagerCallback *callback, httpd_req_t *req, httpd_ws_frame_t *ws_pkt, uint32_t ns, uint8_t *buf) override
    {
        return webmanager::eMessageReceiverResult::NOT_FOR_ME;
    }
};
#undef TAG