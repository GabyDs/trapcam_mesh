/*
 * Leaf Node Module Implementation
 * 
 * Manages the leaf node lifecycle:
 * 1. Boot/wake-up
 * 2. Connect to mesh network
 * 3. Process motion event (if PIR triggered)
 * 4. Return to deep sleep with ULP monitoring
 */

#include "leaf_node.h"
#include "ulp_pir.h"

#include <string.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mesh_lite.h"

static const char *TAG = "leaf_node";

/* RTC memory to persist data across deep sleep */
static RTC_DATA_ATTR uint32_t s_boot_count = 0;
static RTC_DATA_ATTR uint32_t s_motion_events = 0;

static bool s_initialized = false;
static bool s_mesh_connected = false;
static TimerHandle_t s_sleep_timer = NULL;

/* Forward declarations */
static void sleep_timer_callback(TimerHandle_t timer);
static void on_mesh_connected(void);
static void process_motion_event(void);

esp_err_t leaf_node_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Leaf node already initialized");
        return ESP_OK;
    }

    s_boot_count++;
    ESP_LOGI(TAG, "=== LEAF NODE BOOT #%"PRIu32" ===", s_boot_count);

    /* Check wake-up reason */
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_ULP:
            ESP_LOGI(TAG, "Woke up from ULP (PIR motion detected!)");
            s_motion_events++;
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            ESP_LOGI(TAG, "Fresh boot (power-on or reset)");
            break;
        default:
            ESP_LOGI(TAG, "Woke up from other source: %d", wakeup_reason);
            break;
    }

    /* Initialize ULP PIR subsystem */
    esp_err_t ret = ulp_pir_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ULP PIR: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Create sleep timer (one-shot) */
    s_sleep_timer = xTimerCreate(
        "sleep_timer",
        pdMS_TO_TICKS(LEAF_AWAKE_TIME_MS),
        pdFALSE,  /* One-shot */
        NULL,
        sleep_timer_callback
    );

    if (s_sleep_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create sleep timer");
        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Leaf node initialized. Total motion events: %"PRIu32, s_motion_events);

    return ESP_OK;
}

void leaf_node_start(void)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "Leaf node not initialized!");
        return;
    }

    ESP_LOGI(TAG, "Starting leaf node operation...");
    ESP_LOGI(TAG, "Will stay awake for %d ms after mesh connection", LEAF_AWAKE_TIME_MS);

    /* Check if this was a PIR wake-up */
    if (ulp_pir_was_triggered()) {
        ESP_LOGI(TAG, "*** MOTION DETECTED! Processing event... ***");
        process_motion_event();
        ulp_pir_clear_triggered();
    }

    /* 
     * The node will connect to mesh via esp_mesh_lite_start() called in main.
     * We start a timer to go back to sleep after LEAF_AWAKE_TIME_MS.
     * In a real application, you might want to:
     * - Wait for mesh connection confirmation
     * - Send sensor data to root node
     * - Wait for acknowledgment
     * - Then sleep
     */
    
    ESP_LOGI(TAG, "Starting sleep timer (%d ms)...", LEAF_AWAKE_TIME_MS);
    xTimerStart(s_sleep_timer, 0);
}

bool leaf_node_check_wakeup_reason(void)
{
    return ulp_pir_was_triggered();
}

void leaf_node_request_sleep(void)
{
    ESP_LOGI(TAG, "Sleep requested, entering deep sleep with ULP PIR monitoring...");
    
    /* Add small delay to allow logs to flush */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ulp_pir_enter_deep_sleep();
    /* Never returns */
}

uint32_t leaf_node_get_motion_count(void)
{
    return s_motion_events;
}

/* === Private Functions === */

static void sleep_timer_callback(TimerHandle_t timer)
{
    ESP_LOGI(TAG, "Sleep timer expired, preparing for deep sleep...");
    
    /* Check mesh level to confirm we're connected */
    int level = esp_mesh_lite_get_level();
    ESP_LOGI(TAG, "Current mesh level: %d", level);
    
    if (level > 0) {
        ESP_LOGI(TAG, "Connected to mesh at level %d. Going to sleep.", level);
    } else {
        ESP_LOGW(TAG, "Not connected to mesh, but sleeping anyway to conserve power");
    }

    /* Enter deep sleep with ULP monitoring */
    leaf_node_request_sleep();
}

static void process_motion_event(void)
{
    /*
     * TODO: Implement actual motion event processing:
     * - Capture image from camera
     * - Store to SD card
     * - Send notification to root node
     * - etc.
     * 
     * For now, just log the event
     */
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  MOTION EVENT #%"PRIu32, s_motion_events);
    ESP_LOGI(TAG, "  PIR Wake-up count: %"PRIu32, ulp_pir_get_wakeup_count());
    ESP_LOGI(TAG, "========================================");
    
    /* In the future, this is where you would:
     * 1. Take a photo
     * 2. Save to SD
     * 3. Send event to root via mesh
     */
}
