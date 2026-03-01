#include "dashboard_ui.h"

#include <stdint.h>

#include "lvgl.h"

#include "app/sensor_service.h"

#define CALIB_HOLD_MS 5000U
#define AUTO_CYCLE_MS 5000U

static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_center_speed_value = NULL;
static lv_obj_t *s_center_speed_unit = NULL;
static lv_obj_t *s_cal_hint_label = NULL;

static lv_obj_t *s_alt_min_label = NULL;
static lv_obj_t *s_alt_now_label = NULL;
static lv_obj_t *s_alt_max_label = NULL;
static lv_obj_t *s_pitch_label = NULL;
static lv_obj_t *s_roll_label = NULL;
static lv_obj_t *s_aux_label = NULL;

static lv_obj_t *s_gauge = NULL;
static lv_meter_indicator_t *s_pitch_needle = NULL;
static lv_meter_indicator_t *s_roll_needle = NULL;
static lv_meter_indicator_t *s_alt_needle = NULL;

static uint32_t s_hold_start_ms = 0;
static uint32_t s_cal_feedback_until_ms = 0;
static lv_obj_t *s_screens[3] = {0};
static int s_screen_index = 0;
static uint32_t s_last_tap_ms = 0;

#define DOUBLE_TAP_WINDOW_MS 350U

static float clampf(float value, float min_v, float max_v)
{
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

static void cycle_screen_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED) {
        return;
    }
    const uint32_t now = lv_tick_get();
    if ((now - s_last_tap_ms) <= DOUBLE_TAP_WINDOW_MS) {
        s_screen_index = (s_screen_index + 1) % 3;
        lv_scr_load_anim(s_screens[s_screen_index], LV_SCR_LOAD_ANIM_MOVE_LEFT, 180, 0, false);
        s_last_tap_ms = 0;
    } else {
        s_last_tap_ms = now;
    }
}

static void auto_cycle_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_screen_index = (s_screen_index + 1) % 3;
    lv_scr_load_anim(s_screens[s_screen_index], LV_SCR_LOAD_ANIM_MOVE_LEFT, 180, 0, false);
}

static void add_cycle_hotspot(lv_obj_t *parent)
{
    lv_obj_t *tap = lv_obj_create(parent);
    lv_obj_set_size(tap, 480, 480);
    lv_obj_set_style_bg_opa(tap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(tap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(tap, 0, 0);
    lv_obj_add_flag(tap, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tap, cycle_screen_event_cb, LV_EVENT_SHORT_CLICKED, NULL);
}

static void center_hold_event_cb(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    const uint32_t now = lv_tick_get();

    if (code == LV_EVENT_PRESSED) {
        s_hold_start_ms = now;
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        if (s_hold_start_ms == 0) {
            s_hold_start_ms = now;
        }
        const uint32_t elapsed = now - s_hold_start_ms;
        if (elapsed < CALIB_HOLD_MS) {
            const uint32_t remain_tenths = (CALIB_HOLD_MS - elapsed + 99U) / 100U;
            lv_label_set_text_fmt(s_cal_hint_label, "Hold %.1fs to Cal", (float)remain_tenths / 10.0f);
        } else {
            lv_label_set_text(s_cal_hint_label, "Release to Calibrate");
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (s_hold_start_ms != 0) {
            const uint32_t elapsed = now - s_hold_start_ms;
            if (elapsed >= CALIB_HOLD_MS && sensor_service_calibrate_level_zero() == ESP_OK) {
                s_cal_feedback_until_ms = now + 1800U;
            }
        }
        s_hold_start_ms = 0;
    }
}

static void update_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    gauge_metrics_t m = {0};
    if (!sensor_service_get_snapshot(&m)) {
        return;
    }

    const float speed_mph = m.speed_mps * 2.236936f;
    const float alt_ft = m.altitude_m * 3.28084f;
    const float alt_min_ft = m.min_altitude_m * 3.28084f;
    const float alt_max_ft = m.max_altitude_m * 3.28084f;
    const float vs_ftps = m.vertical_speed_mps * 3.28084f;

    lv_meter_set_indicator_value(s_gauge, s_pitch_needle, (int32_t)clampf(m.pitch_deg, -15.0f, 15.0f));
    lv_meter_set_indicator_value(s_gauge, s_roll_needle, (int32_t)clampf(m.roll_deg, -15.0f, 15.0f));
    lv_meter_set_indicator_value(s_gauge, s_alt_needle, (int32_t)clampf(alt_ft, -1000.0f, 12000.0f));

    lv_label_set_text_fmt(s_center_speed_value, "%.1f", speed_mph);
    lv_label_set_text(s_center_speed_unit, "GPS MPH");

    lv_label_set_text_fmt(s_alt_max_label, "Max\n%.0f", alt_max_ft);
    lv_label_set_text_fmt(s_alt_now_label, "ALT\n%.0f", alt_ft);
    lv_label_set_text_fmt(s_alt_min_label, "Min\n%.0f", alt_min_ft);
    lv_label_set_text_fmt(s_pitch_label, "Pitch %+.1f", m.pitch_deg);
    lv_label_set_text_fmt(s_roll_label, "Roll %+.1f", m.roll_deg);
    lv_label_set_text_fmt(s_aux_label, "Yaw %+.1f  VSpd %+.2f", m.yaw_deg, vs_ftps);

    lv_label_set_text_fmt(
        s_status_label,
        "IMU:%s BARO:%s P:%.1f hPa",
        m.imu_ok ? "OK" : "OFF",
        m.baro_ok ? "OK" : "OFF",
        m.pressure_hpa);

    if (s_hold_start_ms != 0) {
        const uint32_t elapsed = lv_tick_get() - s_hold_start_ms;
        if (elapsed < CALIB_HOLD_MS) {
            const uint32_t remain_tenths = (CALIB_HOLD_MS - elapsed + 99U) / 100U;
            lv_label_set_text_fmt(s_cal_hint_label, "Hold %.1fs to Cal", (float)remain_tenths / 10.0f);
            lv_obj_set_style_text_color(s_cal_hint_label, lv_color_hex(0xF2C14E), 0);
        } else {
            lv_label_set_text(s_cal_hint_label, "Release to Calibrate");
            lv_obj_set_style_text_color(s_cal_hint_label, lv_color_hex(0xFFB86C), 0);
        }
    } else if (lv_tick_get() < s_cal_feedback_until_ms) {
        lv_label_set_text(s_cal_hint_label, "Calibrated");
        lv_obj_set_style_text_color(s_cal_hint_label, lv_color_hex(0x62E07D), 0);
    } else {
        lv_label_set_text(s_cal_hint_label, "Hold 5.0s to Cal");
        lv_obj_set_style_text_color(s_cal_hint_label, lv_color_hex(0x9CB2C9), 0);
    }
}

void dashboard_ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(scr, lv_color_hex(0xF2F5F8), 0);
    s_screens[0] = scr;

    lv_obj_t *root = lv_obj_create(scr);
    lv_obj_set_size(root, 440, 440);
    lv_obj_center(root);
    lv_obj_set_style_radius(root, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(root, true, 0);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0A111A), 0);
    lv_obj_set_style_bg_grad_color(root, lv_color_hex(0x111C2A), 0);
    lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(root, 2, 0);
    lv_obj_set_style_border_color(root, lv_color_hex(0x1E2E43), 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    s_status_label = lv_label_create(root);
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0x8CA3BD), 0);
    lv_obj_align(s_status_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_text(s_status_label, "IMU:-- BARO:--");

    // Single shared gauge keeps all scales perfectly concentric and aligned.
    s_gauge = lv_meter_create(root);
    lv_obj_set_size(s_gauge, 420, 420);
    lv_obj_center(s_gauge);
    lv_obj_set_style_bg_opa(s_gauge, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(s_gauge, LV_OPA_TRANSP, 0);

    // Left pitch scale: 120..240 deg (center at 180, i.e. left).
    lv_meter_scale_t *pitch_scale = lv_meter_add_scale(s_gauge);
    lv_meter_set_scale_ticks(s_gauge, pitch_scale, 31, 2, 12, lv_color_hex(0x253646));
    lv_meter_set_scale_major_ticks(s_gauge, pitch_scale, 5, 4, 18, lv_color_hex(0xD8E4EE), 14);
    lv_meter_set_scale_range(s_gauge, pitch_scale, -15, 15, 120, 120);
    lv_meter_indicator_t *pitch_band = lv_meter_add_arc(s_gauge, pitch_scale, 12, lv_color_hex(0x43C75A), 0);
    lv_meter_set_indicator_start_value(s_gauge, pitch_band, -15);
    lv_meter_set_indicator_end_value(s_gauge, pitch_band, 15);
    s_pitch_needle = lv_meter_add_needle_line(s_gauge, pitch_scale, 4, lv_color_hex(0xFFFFFF), -26);

    // Bottom roll scale: 225..315 deg (center at 270, i.e. bottom).
    lv_meter_scale_t *roll_scale = lv_meter_add_scale(s_gauge);
    lv_meter_set_scale_ticks(s_gauge, roll_scale, 31, 2, 12, lv_color_hex(0x253646));
    lv_meter_set_scale_major_ticks(s_gauge, roll_scale, 5, 4, 18, lv_color_hex(0xD8E4EE), 14);
    lv_meter_set_scale_range(s_gauge, roll_scale, -15, 15, 90, 225);
    lv_meter_indicator_t *roll_band = lv_meter_add_arc(s_gauge, roll_scale, 12, lv_color_hex(0x43C75A), 0);
    lv_meter_set_indicator_start_value(s_gauge, roll_band, -15);
    lv_meter_set_indicator_end_value(s_gauge, roll_band, 15);
    s_roll_needle = lv_meter_add_needle_line(s_gauge, roll_scale, 4, lv_color_hex(0xFFFFFF), -36);

    // Right altitude scale: 300..60 deg wrap (120 deg span).
    lv_meter_scale_t *alt_scale = lv_meter_add_scale(s_gauge);
    lv_meter_set_scale_ticks(s_gauge, alt_scale, 41, 2, 10, lv_color_hex(0x27507D));
    lv_meter_set_scale_major_ticks(s_gauge, alt_scale, 8, 4, 16, lv_color_hex(0xCFE2F5), 12);
    lv_meter_set_scale_range(s_gauge, alt_scale, -1000, 12000, 120, 300);
    lv_meter_indicator_t *alt_band = lv_meter_add_arc(s_gauge, alt_scale, 12, lv_color_hex(0x2E8CE0), 0);
    lv_meter_set_indicator_start_value(s_gauge, alt_band, -1000);
    lv_meter_set_indicator_end_value(s_gauge, alt_band, 12000);
    s_alt_needle = lv_meter_add_needle_line(s_gauge, alt_scale, 4, lv_color_hex(0xFFFFFF), -46);

    // Center hub (speed + long-press calibration target).
    lv_obj_t *hub = lv_obj_create(root);
    lv_obj_set_size(hub, 176, 176);
    lv_obj_center(hub);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub, lv_color_hex(0x030507), 0);
    lv_obj_set_style_border_color(hub, lv_color_hex(0x1D2D40), 0);
    lv_obj_set_style_border_width(hub, 2, 0);
    lv_obj_set_style_pad_all(hub, 0, 0);
    lv_obj_add_flag(hub, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(hub, center_hold_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(hub, center_hold_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(hub, center_hold_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(hub, center_hold_event_cb, LV_EVENT_PRESS_LOST, NULL);

    s_center_speed_value = lv_label_create(hub);
    lv_obj_set_style_text_font(s_center_speed_value, &lv_font_montserrat_20, 0);
    lv_obj_align(s_center_speed_value, LV_ALIGN_CENTER, 0, -16);
    lv_label_set_text(s_center_speed_value, "0.0");

    s_center_speed_unit = lv_label_create(hub);
    lv_obj_set_style_text_color(s_center_speed_unit, lv_color_hex(0xD8E4F0), 0);
    lv_obj_align(s_center_speed_unit, LV_ALIGN_CENTER, 0, 20);
    lv_label_set_text(s_center_speed_unit, "GPS MPH");

    s_cal_hint_label = lv_label_create(root);
    lv_obj_set_style_text_color(s_cal_hint_label, lv_color_hex(0x9CB2C9), 0);
    lv_obj_align(s_cal_hint_label, LV_ALIGN_CENTER, 0, 104);
    lv_label_set_text(s_cal_hint_label, "Hold 5.0s to Cal");

    s_pitch_label = lv_label_create(root);
    lv_obj_align(s_pitch_label, LV_ALIGN_LEFT_MID, 18, -8);
    lv_label_set_text(s_pitch_label, "Pitch --");

    s_roll_label = lv_label_create(root);
    lv_obj_align(s_roll_label, LV_ALIGN_BOTTOM_MID, -64, -12);
    lv_label_set_text(s_roll_label, "Roll --");

    s_aux_label = lv_label_create(root);
    lv_obj_set_style_text_color(s_aux_label, lv_color_hex(0x9CB2C9), 0);
    lv_obj_align(s_aux_label, LV_ALIGN_BOTTOM_MID, 70, -12);
    lv_label_set_text(s_aux_label, "Yaw --");

    s_alt_max_label = lv_label_create(root);
    lv_obj_align(s_alt_max_label, LV_ALIGN_RIGHT_MID, -18, -92);
    lv_label_set_text(s_alt_max_label, "Max\n--");

    s_alt_now_label = lv_label_create(root);
    lv_obj_align(s_alt_now_label, LV_ALIGN_RIGHT_MID, -18, -8);
    lv_label_set_text(s_alt_now_label, "ALT\n--");

    s_alt_min_label = lv_label_create(root);
    lv_obj_align(s_alt_min_label, LV_ALIGN_RIGHT_MID, -18, 76);
    lv_label_set_text(s_alt_min_label, "Min\n--");

    add_cycle_hotspot(scr);

    lv_obj_t *speedo_daily = lv_obj_create(NULL);
    s_screens[1] = speedo_daily;
    lv_obj_set_style_bg_color(speedo_daily, lv_color_hex(0x000000), 0);
    lv_obj_t *daily_val = lv_label_create(speedo_daily);
    lv_label_set_text(daily_val, "SPEEDO DAILY\n72 mph");
    lv_obj_set_style_text_align(daily_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(daily_val);
    add_cycle_hotspot(speedo_daily);

    lv_obj_t *speedo_track = lv_obj_create(NULL);
    s_screens[2] = speedo_track;
    lv_obj_set_style_bg_color(speedo_track, lv_color_hex(0x000000), 0);
    lv_obj_t *track_val = lv_label_create(speedo_track);
    lv_label_set_text(track_val, "SPEEDO TRACK\n128 mph");
    lv_obj_set_style_text_align(track_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(track_val);
    add_cycle_hotspot(speedo_track);

    lv_timer_create(update_timer_cb, 100, NULL);
    lv_timer_create(auto_cycle_timer_cb, AUTO_CYCLE_MS, NULL);
    update_timer_cb(NULL);
}
