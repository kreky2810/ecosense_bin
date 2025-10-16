#pragma once
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ai_init(const char *model_path);                     // npr. "/spiffs/pvc_classifier.espdl"
esp_err_t ai_process_frame(const void *buf, int w, int h, bool *is_pvc_out);

#ifdef __cplusplus
}
#endif
