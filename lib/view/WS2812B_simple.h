/*
 * WS2812B_core.h
 * * PWM + DMAを使ったWS2812Bドライバ（ヘッダーオンリー版）
 * 使い方: main.cで #define WS2812_MAX_LED [個数] をしてから include すること！
 */

#ifndef WS2812B_CORE_H
#define WS2812B_CORE_H

#include "main.h"

// --- コンフィグ設定 ---
// ユーザーが個数を決めてなかったら、とりあえず100個分確保する（安全策）
#ifndef WS2812_MAX_LED
    #define WS2812_MAX_LED 100
#endif

// --- 内部定数 (800kHz PWM用 / ARR=79想定) ---
#define WS2812_RESET_SLOTS 240  // リセット信号用 (約300us)
#define WS2812_BIT_0       26   // Duty 32%
#define WS2812_BIT_1       51   // Duty 64%

// --- ライブラリ内部変数 (staticで隠蔽) ---
// メモリ確保！ここで MAX_LED 分だけドカッと取る
static uint32_t _ws2812_buffer[(WS2812_MAX_LED * 24) + WS2812_RESET_SLOTS];
static TIM_HandleTypeDef* _ws2812_tim = 0;
static uint32_t  _ws2812_ch     = 0;
static int       _ws2812_use_count = 0;

// --- 初期化関数 ---
// num_leds: 今回実際に使うLEDの個数
static void WS2812_Init(TIM_HandleTypeDef *htim, uint32_t channel, int num_leds) {
    _ws2812_tim       = htim;
    _ws2812_ch        = channel;
    
    // 安全装置：MAXより多く指定されたらMAXに制限する
    if (num_leds > WS2812_MAX_LED) num_leds = WS2812_MAX_LED;
    _ws2812_use_count = num_leds;
}

// --- 色セット関数 ---
// n: LED番号 (0〜)
// r,g,b: 色 (0〜255)
// d: 明るさ (0〜100%)
static void WS2812_Set(int n, uint8_t r, uint8_t g, uint8_t b, uint8_t d) {
    // 初期化前 or 範囲外なら無視
    if (_ws2812_tim == 0 || n >= _ws2812_use_count) return;

    // 明るさ調整 (Simple scaling)
    uint8_t dim_r = (uint16_t)r * d / 100;
    uint8_t dim_g = (uint16_t)g * d / 100;
    uint8_t dim_b = (uint16_t)b * d / 100;

    int idx = n * 24;
    // WS2812Bは G -> R -> B の順
    for (int i = 0; i < 8; i++) _ws2812_buffer[idx + i]      = (dim_g & (0x80 >> i)) ? WS2812_BIT_1 : WS2812_BIT_0;
    for (int i = 0; i < 8; i++) _ws2812_buffer[idx + 8 + i]  = (dim_r & (0x80 >> i)) ? WS2812_BIT_1 : WS2812_BIT_0;
    for (int i = 0; i < 8; i++) _ws2812_buffer[idx + 16 + i] = (dim_b & (0x80 >> i)) ? WS2812_BIT_1 : WS2812_BIT_0;
}

// --- 送信実行関数 ---
static void WS2812_Show(void) {
    if (_ws2812_tim == 0) return;

    // データの後ろにリセット信号（Low）を埋める
    int total_len = (_ws2812_use_count * 24) + WS2812_RESET_SLOTS;
    for (int i = (_ws2812_use_count * 24); i < total_len; i++) _ws2812_buffer[i] = 0;

    // DMAで送信開始！
    HAL_TIM_PWM_Start_DMA(_ws2812_tim, _ws2812_ch, _ws2812_buffer, total_len);
}

// --- 完了コールバック用 (main.cから呼ぶ) ---
static void WS2812_Callback(TIM_HandleTypeDef *htim) {
    if (_ws2812_tim != 0 && htim->Instance == _ws2812_tim->Instance) {
        HAL_TIM_PWM_Stop_DMA(_ws2812_tim, _ws2812_ch);
    }
}

#endif
