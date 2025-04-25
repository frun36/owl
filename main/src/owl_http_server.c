#include "owl_http_server.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "owl_http_server";

static httpd_handle_t server_handle = NULL;
static int ws_fd = -1;

static char *index_html = NULL;
static size_t index_html_size = 0;

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, index_html_size);
    return ESP_OK;
}

static httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL,
};

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ws_fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "WS handshake done (fd = %d)", ws_fd);
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = NULL,
        .len = 0,
    };

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get frame length: %s", esp_err_to_name(ret));
        return ret;
    }

    if (frame.len > 0) {
        frame.payload = malloc(frame.len + 1);
        if (!frame.payload)
            return ESP_ERR_NO_MEM;

        ret = httpd_ws_recv_frame(req, &frame, frame.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read WS frame: %s", esp_err_to_name(ret));
            free(frame.payload);
            return ret;
        }
        frame.payload[frame.len] = '\0';
        ESP_LOGI(TAG, "Received: %s", (char *) frame.payload);
        free(frame.payload);
    }

    if (frame.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "WS closed");
        ws_fd = -1;
    }
    return ret;
}

static const httpd_uri_t ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = ws_handler,
    .user_ctx = NULL,
    .is_websocket = true,
};

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &ws);
    }
    return server;
}

void owl_ws_send(const char *message)
{
    if (ws_fd < 0 || server_handle == NULL) {
        ESP_LOGE(TAG, "No active WS connection");
        return;
    }

    httpd_ws_frame_t ws_pkt = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *) message,
        .len = strlen(message),
    };

    esp_err_t ret = httpd_ws_send_frame_async(server_handle, ws_fd, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send WS message: %s", esp_err_to_name(ret));
        ws_fd = -1;
    }
}

static esp_err_t load_index_html()
{
    ESP_LOGI(TAG, "Reading index.html");
    FILE *file = fopen("/spiffs/index.html", "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open index.html");
        return ESP_FAIL;
    }

    fseek(file, 0, SEEK_END);
    index_html_size = ftell(file);
    rewind(file);

    index_html = malloc(index_html_size + 1);
    if (!index_html) {
        ESP_LOGE(TAG, "Memory allocation for index.html failed");
        fclose(file);
        return ESP_ERR_NO_MEM;
    }

    fread(index_html, 1, index_html_size, file);
    index_html[index_html_size] = '\0'; // Null-terminate
    fclose(file);

    return ESP_OK;
}

static void handle_spiffs()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 1,
        .format_if_mount_failed = false,
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    size_t total = 0, used = 0;
    ESP_ERROR_CHECK(esp_spiffs_info(NULL, &total, &used));
    ESP_LOGI(TAG, "Partition size: total: %zu, used: %zu", total, used);

    ESP_ERROR_CHECK(load_index_html());

    ESP_ERROR_CHECK(esp_vfs_spiffs_unregister(NULL));
    ESP_LOGI(TAG, "SPIFFS unmounted");
}

void owl_init_http_server()
{
    server_handle = start_webserver();
    handle_spiffs();
    ESP_LOGI(TAG, "Initialized HTTP server");
}
