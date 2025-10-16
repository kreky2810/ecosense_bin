#include <cstring>
#include "esp_err.h"
#include "ai_infer_c.h"

// ESP-DL
#include "dl_model_base.hpp"
#include "dl_tensor_base.hpp"
#include "fbs_loader.hpp"

using namespace dl;

// binarni simbol od modela u rodata:
extern "C" const uint8_t _binary_pvc_classifier_espdl_start[];

static Model* g_model = nullptr;
static int8_t* g_input = nullptr;

static constexpr int IN_W = 160;   // prilagodi tvom modelu
static constexpr int IN_H = 160;
static constexpr int IN_C = 3;

static inline uint8_t r5(uint16_t p){ return (uint8_t)((((p>>11)&0x1F)*527+23)>>6); }
static inline uint8_t g6(uint16_t p){ return (uint8_t)((((p>>5 )&0x3F)*259+33)>>6); }
static inline uint8_t b5(uint16_t p){ return (uint8_t)(( (p     &0x1F)*527+23)>>6); }

static void preprocess_rgb565_to_int8(const uint16_t* src, int sw, int sh, int8_t* dst) {
    // nearest resize + centrirano skaliranje (jednostavno)
    for (int y=0; y<IN_H; ++y) {
        int sy = (y * sh) / IN_H;
        for (int x=0; x<IN_W; ++x) {
            int sx = (x * sw) / IN_W;
            uint16_t p = src[sy*sw + sx];
            *dst++ = (int8_t)((int)r5(p) - 128);
            *dst++ = (int8_t)((int)g6(p) - 128);
            *dst++ = (int8_t)((int)b5(p) - 128);
        }
    }
}

extern "C" esp_err_t ai_init(const char * /*model_path_ignored*/) {
    if (g_model) return ESP_OK;
    g_input = (int8_t*)heap_caps_malloc(IN_W*IN_H*IN_C, MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if (!g_input) return ESP_ERR_NO_MEM;

    // Učitaj iz rodata:
    g_model = new Model((const char*)_binary_pvc_classifier_espdl_start,
                        fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    return g_model ? ESP_OK : ESP_FAIL;
}

extern "C" esp_err_t ai_process_frame(const void *buf, int w, int h, bool *is_pvc_out) {
    if (!g_model || !buf || !is_pvc_out) return ESP_ERR_INVALID_ARG;

    preprocess_rgb565_to_int8((const uint16_t*)buf, w, h, g_input);

    auto inputs = g_model->get_inputs();
    TensorBase* in = inputs.begin()->second; // očekuje [1,IN_H,IN_W,3] INT
    std::memcpy(in->get_element_ptr(), g_input, IN_W*IN_H*IN_C);

    g_model->run();

    auto outputs = g_model->get_outputs();
    TensorBase* out = outputs.begin()->second; // N-klasa, INT8/FP — ovdje pretpostavimo INT8

    int n = out->get_size();
    int best = 0; int8_t bestv = ((int8_t*)out->get_element_ptr())[0];
    for (int i=1;i<n;++i){
        int8_t v = ((int8_t*)out->get_element_ptr())[i];
        if (v > bestv){ best=i; bestv=v; }
    }
    float score = (bestv + 128) / 255.0f;

    // dogovor: 0=other, 1=PVC_bottle (prilagodi prema svom modelu!)
    *is_pvc_out = (best == 1) && (score >= 0.8f);
    return ESP_OK;
}
