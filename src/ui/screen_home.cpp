// screen_home.cpp — home / cooking screen (base spec §9.1). M2 Thermometer +
// M3 Target Assist: big smoothed temperature, rate + trend, target adjuster,
// ETA, a color-coded action bar, and a full-screen alert overlay for
// READY / TURN DOWN NOW / TOO HOT (§9.3). Portable LVGL; owns its screen.
#include "screen_home.h"

#include <cstdio>

#include "ui/ui_root.h"
#include "app_config.h"
#include "core/presets.h"

namespace ui {
namespace {

lv_obj_t* s_screen = nullptr;
lv_obj_t* s_mode = nullptr;
lv_obj_t* s_conf = nullptr;
lv_obj_t* s_batt = nullptr;
lv_obj_t* s_unit_btn_lbl = nullptr;
lv_obj_t* s_target_lbl = nullptr;
lv_obj_t* s_temp = nullptr;
lv_obj_t* s_rate = nullptr;
lv_obj_t* s_eta = nullptr;
lv_obj_t* s_note = nullptr;
lv_obj_t* s_z2 = nullptr;       // secondary-pan tile (M12)
lv_obj_t* s_z2_lbl = nullptr;
lv_obj_t* s_arc = nullptr;
lv_obj_t* s_cook = nullptr;
lv_obj_t* s_bar = nullptr;
lv_obj_t* s_bar_lbl = nullptr;
lv_obj_t* s_overlay = nullptr;
lv_obj_t* s_overlay_lbl = nullptr;
lv_obj_t* s_overlay_sub = nullptr;

void temp_tap_cb(lv_event_t*) { ui::show_thermal(); }
void unit_cb(lv_event_t*) { ui::toggle_unit(); }
void preset_tap_cb(lv_event_t*) { ui::show_presets(); }
void z2_tap_cb(lv_event_t*) { ui::show_presets_zone2(); }
void bar_tap_cb(lv_event_t*) { ui::recipe_cmd(2); }   // ack a recipe cue

inline float cToF(float c) { return c * 9.0f / 5.0f + 32.0f; }

void minus_cb(lv_event_t*) { ui::target_adjust(-TARGET_STEP_F); }
void plus_cb(lv_event_t*)  { ui::target_adjust(+TARGET_STEP_F); }

// guidance -> (action-bar text, color)
struct BarStyle { const char* text; uint32_t color; };
BarStyle bar_for(GuidanceState g) {
  switch (g) {
    case GuidanceState::HEAT_MORE:      return {"Heat more",        0x2E5AAC};
    case GuidanceState::HOLD:           return {"Hold",             0x2E5AAC};
    case GuidanceState::TURN_DOWN_SOON: return {"Turn down soon",   0xC08A00};
    case GuidanceState::TURN_DOWN_NOW:  return {"TURN DOWN NOW",     0xE07000};
    case GuidanceState::READY:          return {"READY",            0x2E7D32};
    case GuidanceState::TOO_HOT:        return {"TOO HOT",           0xC62828};
    case GuidanceState::COOLING:        return {"Cooling",          0x2E5AAC};
    case GuidanceState::RECOVERING:     return {"Recovering",       0x2E5AAC};
    case GuidanceState::CHECK_AIM:      return {"Check aim",        0xC08A00};
    case GuidanceState::LOW_CONFIDENCE: return {"Low confidence - trend only", 0x555C66};
    case GuidanceState::NO_PAN:         return {"No pan - aim the sensor", 0x555C66};
    default:                            return {"--",               0x2A323C};
  }
}

lv_obj_t* mk_btn(lv_obj_t* p, const char* txt, lv_event_cb_t cb, int w, int h) {
  lv_obj_t* b = lv_btn_create(p);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_bg_color(b, lv_color_hex(0x2A323C), 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_center(l);
  return b;
}

}  // namespace

lv_obj_t* home_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  // status bar — mode/preset label doubles as the preset-picker button
  s_mode = lv_label_create(scr);
  lv_label_set_text(s_mode, "Generic " LV_SYMBOL_LIST);
  lv_obj_set_style_text_color(s_mode, lv_color_hex(0x8A93A0), 0);
  lv_obj_set_style_text_font(s_mode, &lv_font_montserrat_14, 0);
  lv_obj_align(s_mode, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_add_flag(s_mode, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_ext_click_area(s_mode, 20);
  lv_obj_add_event_cb(s_mode, preset_tap_cb, LV_EVENT_CLICKED, nullptr);

  s_conf = lv_label_create(scr);
  lv_obj_set_style_text_color(s_conf, lv_color_hex(0x8A93A0), 0);
  lv_obj_set_style_text_font(s_conf, &lv_font_montserrat_14, 0);
  lv_obj_align(s_conf, LV_ALIGN_TOP_RIGHT, -142, 10);

  s_batt = lv_label_create(scr);
  lv_obj_set_style_text_color(s_batt, lv_color_hex(0x8A93A0), 0);
  lv_obj_set_style_text_font(s_batt, &lv_font_montserrat_14, 0);
  lv_obj_align(s_batt, LV_ALIGN_TOP_RIGHT, -80, 10);

  lv_obj_t* ubtn = mk_btn(scr, "\xC2\xB0" "F", unit_cb, 62, 30);
  lv_obj_align(ubtn, LV_ALIGN_TOP_RIGHT, -8, 6);
  s_unit_btn_lbl = lv_obj_get_child(ubtn, 0);

  // target adjuster:  [-]  350°F  [+]
  lv_obj_t* minus = mk_btn(scr, "-", minus_cb, 52, 40);
  lv_obj_align(minus, LV_ALIGN_TOP_MID, -104, 40);
  lv_obj_t* plus = mk_btn(scr, "+", plus_cb, 52, 40);
  lv_obj_align(plus, LV_ALIGN_TOP_MID, 104, 40);
  s_target_lbl = lv_label_create(scr);
  lv_obj_set_style_text_font(s_target_lbl, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(s_target_lbl, lv_color_hex(0xF5F5F5), 0);
  lv_obj_align(s_target_lbl, LV_ALIGN_TOP_MID, 0, 46);

  // big temperature (tap -> thermal view)
  s_temp = lv_label_create(scr);
  lv_label_set_text(s_temp, "--");
  lv_obj_set_style_text_font(s_temp, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(s_temp, lv_color_hex(0xF5F5F5), 0);
  lv_obj_align(s_temp, LV_ALIGN_CENTER, 0, -6);
  lv_obj_add_flag(s_temp, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_ext_click_area(s_temp, 30);
  lv_obj_add_event_cb(s_temp, temp_tap_cb, LV_EVENT_CLICKED, nullptr);

  s_rate = lv_label_create(scr);
  lv_obj_set_style_text_font(s_rate, &lv_font_montserrat_20, 0);
  lv_obj_align(s_rate, LV_ALIGN_CENTER, 0, 40);

  s_eta = lv_label_create(scr);
  lv_obj_set_style_text_font(s_eta, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_eta, lv_color_hex(0x8A93A0), 0);
  lv_obj_align(s_eta, LV_ALIGN_CENTER, 0, 70);

  // Secondary-pan tile (M12): tap to set zone 2's target.
  s_z2 = lv_btn_create(scr);
  lv_obj_set_size(s_z2, 132, 46);
  lv_obj_align(s_z2, LV_ALIGN_TOP_RIGHT, -8, 44);
  lv_obj_set_style_bg_color(s_z2, lv_color_hex(0x1A2027), 0);
  lv_obj_add_event_cb(s_z2, z2_tap_cb, LV_EVENT_CLICKED, nullptr);
  s_z2_lbl = lv_label_create(s_z2);
  lv_obj_set_style_text_font(s_z2_lbl, &lv_font_montserrat_14, 0);
  lv_obj_center(s_z2_lbl);
  lv_obj_add_flag(s_z2, LV_OBJ_FLAG_HIDDEN);

  // Food-timer countdown arc around the temperature (roadmap §2.7)
  s_arc = lv_arc_create(scr);
  lv_obj_set_size(s_arc, 214, 214);
  lv_obj_align(s_arc, LV_ALIGN_CENTER, 0, -6);
  lv_arc_set_rotation(s_arc, 270);
  lv_arc_set_bg_angles(s_arc, 0, 360);
  lv_arc_set_range(s_arc, 0, 100);
  lv_obj_remove_style(s_arc, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x2A323C), LV_PART_MAIN);
  lv_obj_set_style_arc_color(s_arc, lv_color_hex(0xE07000), LV_PART_INDICATOR);
  lv_obj_add_flag(s_arc, LV_OBJ_FLAG_HIDDEN);

  s_cook = lv_label_create(scr);
  lv_obj_set_style_text_font(s_cook, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_cook, lv_color_hex(0xE07000), 0);
  lv_obj_align(s_cook, LV_ALIGN_CENTER, 0, 96);

  // stainless banner (base spec §7.5) — just above the action bar
  s_note = lv_label_create(scr);
  lv_obj_set_style_text_font(s_note, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_note, lv_color_hex(0xC08A00), 0);
  lv_obj_align(s_note, LV_ALIGN_BOTTOM_MID, 0, -52);

  // action bar (bottom, full width)
  s_bar = lv_obj_create(scr);
  lv_obj_remove_style_all(s_bar);
  lv_obj_set_size(s_bar, 480, 46);
  lv_obj_align(s_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(s_bar, LV_OBJ_FLAG_CLICKABLE);      // tap to ack recipe cues
  lv_obj_add_event_cb(s_bar, bar_tap_cb, LV_EVENT_CLICKED, nullptr);
  s_bar_lbl = lv_label_create(s_bar);
  lv_obj_set_style_text_font(s_bar_lbl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_bar_lbl, lv_color_hex(0xFFFFFF), 0);
  lv_obj_center(s_bar_lbl);

  // full-screen alert overlay (hidden unless READY/TURN DOWN NOW/TOO HOT)
  s_overlay = lv_obj_create(scr);
  lv_obj_remove_style_all(s_overlay);
  lv_obj_set_size(s_overlay, 480, 320);
  lv_obj_center(s_overlay);
  s_overlay_lbl = lv_label_create(s_overlay);
  lv_obj_set_style_text_font(s_overlay_lbl, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(s_overlay_lbl, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(s_overlay_lbl, LV_ALIGN_CENTER, 0, -16);
  s_overlay_sub = lv_label_create(s_overlay);
  lv_obj_set_style_text_font(s_overlay_sub, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(s_overlay_sub, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(s_overlay_sub, LV_ALIGN_CENTER, 0, 44);
  lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
  return s_screen;
}

void home_update(const UiState& s, bool useF) {
  if (!s_screen) home_create();
  char buf[48];

  lv_label_set_text(s_unit_btn_lbl, useF ? "\xC2\xB0" "F" : "\xC2\xB0" "C");
  std::snprintf(buf, sizeof(buf), "conf %u%%", s.confidence);
  lv_label_set_text(s_conf, buf);

  if (s.battery.valid) {
    std::snprintf(buf, sizeof(buf), "%s %u%%",
                  s.battery.usbPresent ? LV_SYMBOL_CHARGE : LV_SYMBOL_BATTERY_2,
                  s.battery.soc);
    lv_label_set_text(s_batt, buf);
    lv_obj_set_style_text_color(
        s_batt, lv_color_hex(s.battery.soc <= 15 && !s.battery.usbPresent
                                 ? 0xE07000 : 0x8A93A0), 0);
  } else {
    lv_label_set_text(s_batt, s.battery.usbPresent ? LV_SYMBOL_USB : "");
  }

  std::snprintf(buf, sizeof(buf), "%s " LV_SYMBOL_LIST, preset(s.presetId).name);
  lv_label_set_text(s_mode, buf);
  const int tgt = useF ? s.targetCenterF : int((s.targetCenterF - 32) * 5 / 9);
  std::snprintf(buf, sizeof(buf), "%d\xC2\xB0%s", tgt, useF ? "F" : "C");
  lv_label_set_text(s_target_lbl, buf);

  const bool absent = s.presence == PanPresence::ABSENT;
  if (absent || !s.modelValid) {
    lv_label_set_text(s_temp, "--");
    lv_label_set_text(s_rate, "");
    lv_label_set_text(s_eta, "");
  } else {
    const float t = useF ? cToF(s.displayTempC) : s.displayTempC;
    std::snprintf(buf, sizeof(buf), "%d\xC2\xB0%s", int(t + 0.5f), useF ? "F" : "C");
    lv_label_set_text(s_temp, buf);

    const float rate = useF ? s.rateCPerMin * 9.0f / 5.0f : s.rateCPerMin;
    const char* arrow = LV_SYMBOL_MINUS;
    if (s.trend == Trend::RISING_SLOW || s.trend == Trend::RISING_FAST) arrow = LV_SYMBOL_UP;
    else if (s.trend == Trend::COOLING) arrow = LV_SYMBOL_DOWN;
    if (s.rateCPerMin == 0.0f && s.trend == Trend::STABLE)
      std::snprintf(buf, sizeof(buf), "estimating...");
    else
      std::snprintf(buf, sizeof(buf), "%s %d\xC2\xB0%s/min", arrow,
                    int(rate + (rate < 0 ? -0.5f : 0.5f)), useF ? "F" : "C");
    lv_label_set_text(s_rate, buf);

    if (s.etaSeconds >= 0) {
      std::snprintf(buf, sizeof(buf), "ready in %d:%02d", s.etaSeconds / 60,
                    s.etaSeconds % 60);
      lv_label_set_text(s_eta, buf);
    } else {
      lv_label_set_text(s_eta, "");
    }
  }

  // note line: recovery hint (§7.4) takes priority over the stainless banner
  const bool stainless = preset(s.presetId).stainlessHints || s.stainlessHint;
  if (s.recovering && s.recoveryHint == RecoveryHint::SLOW)
    lv_label_set_text(s_note, "Recovery slow - raise heat?");
  else if (s.recovering && s.recoveryHint == RecoveryHint::FAST)
    lv_label_set_text(s_note, "Recovering fast - watch heat");
  else
    lv_label_set_text(s_note, stainless
                                  ? "Bare stainless reads low - trust the trend"
                                  : "");

  // Food timer: countdown arc + side/batch + safety note (roadmap §2.7)
  if (s.food && s.foodTimer.phase == FoodTimerOut::COOKING) {
    const int total = s.food->sideSec[s.foodTimer.side - 1];
    const int rem = s.foodTimer.remainingSec;
    const int pct = total > 0 ? (100 * (total - rem)) / total : 0;
    lv_arc_set_value(s_arc, pct < 0 ? 0 : (pct > 100 ? 100 : pct));
    lv_obj_clear_flag(s_arc, LV_OBJ_FLAG_HIDDEN);
    const char* act = (s.foodTimer.side < s.food->sides) ? "FLIP" : "REMOVE";
    if (s.batchCount > 0)
      std::snprintf(buf, sizeof(buf), "Batch %u  -  Side %u/%u  -  %s in %d:%02d",
                    s.batchCount + 1, s.foodTimer.side, s.food->sides, act, rem / 60, rem % 60);
    else
      std::snprintf(buf, sizeof(buf), "Side %u/%u  -  %s in %d:%02d",
                    s.foodTimer.side, s.food->sides, act, rem / 60, rem % 60);
    lv_label_set_text(s_cook, buf);
    lv_obj_clear_flag(s_cook, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(s_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_cook, LV_OBJ_FLAG_HIDDEN);
  }
  // Food safety + cold-pan notes take the note line when a food is active.
  if (s.food && s.food->safeInternalF > 0) {
    std::snprintf(buf, sizeof(buf), "Surface timing only - verify %u\xC2\xB0" "F internal",
                  s.food->safeInternalF);
    lv_label_set_text(s_note, buf);
  } else if (s.food && s.foodTimer.k < 0.9f &&
             s.foodTimer.phase == FoodTimerOut::COOKING) {
    lv_label_set_text(s_note, "Cooler pan - timer extended to match");
  }

  // Secondary-pan tile (M12): independent zone-2 temperature + guidance color.
  if (s.zone2Present) {
    const float t2 = useF ? cToF(s.zone2TempC) : s.zone2TempC;
    std::snprintf(buf, sizeof(buf), "Pan 2\n%d\xC2\xB0%s", int(t2 + 0.5f), useF ? "F" : "C");
    lv_label_set_text(s_z2_lbl, buf);
    lv_obj_set_style_bg_color(s_z2, lv_color_hex(bar_for(s.zone2Guidance).color), 0);
    lv_obj_clear_flag(s_z2, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(s_z2, LV_OBJ_FLAG_HIDDEN);
  }

  // action bar — a running recipe's cue takes it over (tap to advance).
  const BarStyle bs = bar_for(s.guidance);
  if (s.recipeActive && s.recipeCue[0]) {
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x2E5AAC), 0);
    lv_label_set_text(s_bar_lbl, s.recipeCue);
  } else {
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(bs.color), 0);
    lv_label_set_text(s_bar_lbl, bs.text);
  }
  lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, 0);

  // full-screen overlay for the loud states (§9.3). "Add next batch" (recovery
  // complete, §7.4) takes priority. While actively cooking a food, the timer is
  // the focus — only TOO_HOT (safety) still takes over the screen.
  const bool cooking = s.food && s.foodTimer.phase == FoodTimerOut::COOKING;
  const bool loud = s.guidance == GuidanceState::TOO_HOT ||
                    (!cooking && (s.guidance == GuidanceState::READY ||
                                  s.guidance == GuidanceState::TURN_DOWN_NOW));
  if (s.pluginWarning) {
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0xC62828), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_label_set_text(s_overlay_lbl, "PLUG ME IN");
    lv_label_set_text(s_overlay_sub, "battery critical");
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
  } else if (s.addBatchPrompt) {
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x2E7D32), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_label_set_text(s_overlay_lbl, "ADD NEXT BATCH");
    const float t = useF ? cToF(s.displayTempC) : s.displayTempC;
    std::snprintf(buf, sizeof(buf), "pan recovered - %d\xC2\xB0%s",
                  int(t + 0.5f), useF ? "F" : "C");
    lv_label_set_text(s_overlay_sub, buf);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
  } else if (loud) {
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(bs.color), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_label_set_text(s_overlay_lbl, bs.text);
    if (s.guidance == GuidanceState::TURN_DOWN_NOW && s.projectedPeakF > 0) {
      const float pk = useF ? s.projectedPeakF : (s.projectedPeakF - 32) * 5 / 9;
      std::snprintf(buf, sizeof(buf), "peak ~%d\xC2\xB0%s", int(pk + 0.5f), useF ? "F" : "C");
    } else {
      const float t = useF ? cToF(s.displayTempC) : s.displayTempC;
      std::snprintf(buf, sizeof(buf), "%d\xC2\xB0%s", int(t + 0.5f), useF ? "F" : "C");
    }
    lv_label_set_text(s_overlay_sub, buf);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
  }
}

}  // namespace ui
