// LED_TEST: 指定インデックスのLEDのみ点灯（LED診断用）
// hue フィールドにLEDインデックスを格納。rgb_matrix_config は左右同期済みなので両側で動作する
RGB_MATRIX_EFFECT(LED_TEST)

// SOLID_RIPPLE: 押したキーから波が広がるリアクティブエフェクト
// g_last_hit_tracker（QMKがスレーブに同期済み）を使うため左右両側で動作する
// key-up でも g_last_hit_tracker が発火するため、同座標300ms以内の2重起動を抑制する
RGB_MATRIX_EFFECT(SOLID_RIPPLE)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

static bool LED_TEST(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    uint8_t idx = rgb_matrix_config.hsv.h;  // hue = テスト対象LEDインデックス（両側で同期済み）
    for (uint8_t i = led_min; i < led_max; i++) {
        if (i == idx) {
            rgb_matrix_set_color(i, 255, 100, 0);
        } else {
            rgb_matrix_set_color(i, 0, 0, 0);
        }
    }
    return rgb_matrix_check_finished_leds(led_max);
}

static bool SOLID_RIPPLE(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    // ヒット検出用 static state（各サイドが独立して保持）
    // prev_tick: 0xFFFF 初期値により最初のヒット（tick=0）も正しく検出される
    static uint16_t prev_tick[8] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
                                    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    static uint8_t  rip_x[RIPPLE_MAX_HITS];
    static uint8_t  rip_y[RIPPLE_MAX_HITS];
    static uint16_t rip_start[RIPPLE_MAX_HITS];
    static bool     rip_active[RIPPLE_MAX_HITS];
    static uint8_t  rip_next = 0;

    // g_last_hit_tracker からキーダウン/アップを検出
    // tick がリセット（cur < prev）されたスロット = 新規イベント
    // ただし key-up も同座標でリセットされるため、同位置300ms以内の重複は除外する
    uint8_t cnt = g_last_hit_tracker.count;
    if (cnt > 8) cnt = 8;
    for (uint8_t j = 0; j < cnt; j++) {
        uint16_t cur = g_last_hit_tracker.tick[j];
        if (cur < prev_tick[j]) {
            uint8_t nx = g_last_hit_tracker.x[j];
            uint8_t ny = g_last_hit_tracker.y[j];
            // 同じ座標から直近300ms以内にリップルが起動済みならスキップ（key-up除外）
            bool dup = false;
            for (uint8_t m = 0; m < RIPPLE_MAX_HITS; m++) {
                if (rip_active[m] && rip_x[m] == nx && rip_y[m] == ny &&
                    timer_elapsed(rip_start[m]) < 300) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                rip_x[rip_next]      = nx;
                rip_y[rip_next]      = ny;
                rip_start[rip_next]  = timer_read();
                rip_active[rip_next] = true;
                rip_next = (rip_next + 1) % RIPPLE_MAX_HITS;
            }
        }
        prev_tick[j] = cur;
    }

    // リップルごとの現在 tick を事前計算し、期限切れを deactivate
    // max_dist は uint8_t(sqrt16) の上限 255 なので 255 + RIPPLE_TAIL_LEN で全LEDを通過
    uint16_t rip_tick_val[RIPPLE_MAX_HITS];
    for (uint8_t k = 0; k < RIPPLE_MAX_HITS; k++) {
        rip_tick_val[k] = 0;
        if (!rip_active[k]) continue;
        uint16_t elapsed = timer_elapsed(rip_start[k]);
        // speed設定に依存せず固定速度で計算（elapsed/4: dist=25の隣キーが100ms後に光る）
        uint16_t t = elapsed >> 2;
        if (t > (uint16_t)(255 + RIPPLE_TAIL_LEN)) {
            rip_active[k] = false;
        } else {
            rip_tick_val[k] = t;
        }
    }

    // 各 LED の輝度を計算（全LED対象、アンダーグロウ含む）
    for (uint8_t i = led_min; i < led_max; i++) {
        HSV hsv = rgb_matrix_config.hsv;
        hsv.v   = 0;
        for (uint8_t k = 0; k < RIPPLE_MAX_HITS; k++) {
            if (!rip_active[k]) continue;
            // 左右スプリット境界で波紋を分離（x=112基準）
            if ((rip_x[k] >= 112) != (g_led_config.point[i].x >= 112)) continue;
            int16_t dx   = (int16_t)g_led_config.point[i].x - (int16_t)rip_x[k];
            int16_t dy   = (int16_t)g_led_config.point[i].y - (int16_t)rip_y[k];
            uint8_t adx  = dx < 0 ? (uint8_t)(-dx) : (uint8_t)dx;
            uint8_t ady  = dy < 0 ? (uint8_t)(-dy) : (uint8_t)dy;
            // チェビシェフ距離: キーグリッド単位で隣キーが均等に同時点灯
            uint8_t  dist = adx > ady ? adx : ady;
            uint16_t tick = rip_tick_val[k];
            if (tick >= dist) {
                uint16_t tail = tick - dist;
                if (tail < RIPPLE_TAIL_LEN) {
                    uint8_t brightness = 255 - (uint8_t)(tail * 255 / RIPPLE_TAIL_LEN);
                    hsv.v = qadd8(hsv.v, brightness);
                }
            }
        }
        RGB rgb = rgb_matrix_hsv_to_rgb(hsv);
        rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
