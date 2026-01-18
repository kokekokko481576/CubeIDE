/*
 * WS2812B_core.h
 * バランス型 (uint16_t使用)
 * 重要: CubeMXのDMA設定で Memory Data Width を "Half Word" にすること！
 */

#ifndef WS2812B_CORE_H
#define WS2812B_CORE_H

#include "main.h"

// --- 設定 ---
#ifndef WS2812_MAX_LED
    #define WS2812_MAX_LED 100
#endif

// --- 内部定数 ---
#define WS2812_RESET_SLOTS 240
#define WS2812_BIT_0       26
#define WS2812_BIT_1       51

// --- 変数 ---
// ★ここを変更！ uint8_t -> uint16_t
static uint8_t _ws2812_buffer[(WS2812_MAX_LED * 24) + WS2812_RESET_SLOTS];
static TIM_HandleTypeDef* _ws2812_tim = 0;
static uint32_t  _ws2812_ch     = 0;
static int       _ws2812_use_count = 0;

// --- 初期化 ---
static void WS2812_Init(TIM_HandleTypeDef *htim, uint32_t channel, int num_leds) {
    _ws2812_tim       = htim;
    _ws2812_ch        = channel;
    if (num_leds > WS2812_MAX_LED) num_leds = WS2812_MAX_LED;
    _ws2812_use_count = num_leds;
}

// --- 個別セット ---
static void WS2812_Set(int n, uint8_t r, uint8_t g, uint8_t b, uint8_t d) {
    if (_ws2812_tim == 0 || n >= _ws2812_use_count) return;

    // 明るさ計算
    uint8_t dim_r = (uint16_t)r * d / 100;
    uint8_t dim_g = (uint16_t)g * d / 100;
    uint8_t dim_b = (uint16_t)b * d / 100;

    int idx = n * 24;
    // 配列への代入も uint16_t になる
    for (int i = 0; i < 8; i++) _ws2812_buffer[idx + i]      = (dim_g & (0x80 >> i)) ? WS2812_BIT_1 : WS2812_BIT_0;
    for (int i = 0; i < 8; i++) _ws2812_buffer[idx + 8 + i]  = (dim_r & (0x80 >> i)) ? WS2812_BIT_1 : WS2812_BIT_0;
    for (int i = 0; i < 8; i++) _ws2812_buffer[idx + 16 + i] = (dim_b & (0x80 >> i)) ? WS2812_BIT_1 : WS2812_BIT_0;
}

// --- 一括セット ---
static void WS2812_Fill(uint8_t r, uint8_t g, uint8_t b, uint8_t d) {
    for(int i=0; i<_ws2812_use_count; i++){
        WS2812_Set(i, r, g, b, d);
    }
}

// --- 送信 ---
static void WS2812_Show(void) {
    if (_ws2812_tim == 0) return;
    int total_len = (_ws2812_use_count * 24) + WS2812_RESET_SLOTS;

    // リセット区間を0埋め
    for (int i = (_ws2812_use_count * 24); i < total_len; i++) _ws2812_buffer[i] = 0;

    // DMA送信 (キャストを uint32_t* にして渡せばOK)
    HAL_TIM_PWM_Start_DMA(_ws2812_tim, _ws2812_ch, (uint32_t*)_ws2812_buffer, total_len);
}

// --- コールバック ---
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    if (_ws2812_tim != 0 && htim->Instance == _ws2812_tim->Instance) {
        HAL_TIM_PWM_Stop_DMA(_ws2812_tim, _ws2812_ch);
    }
}

#endif
