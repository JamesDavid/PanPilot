// list_style.h — the ONE scroll-list format (bench 2026-07-12: "all lists
// should be that format. factor it out"): rows own the left ~90% of the list
// and a fat, always-visible scrollbar owns a reserved right gutter, so the two
// never overlap and the bar doubles as a drag handle + scroll cue. Rows should
// use lv_pct(100) width — that resolves against the content area, which the
// gutter padding already shrinks.
#pragma once
#include <lvgl.h>

namespace ui {

inline void apply_scroll_list_style(lv_obj_t* list) {
  lv_obj_set_style_pad_right(list, 30, 0);          // the scrollbar's gutter
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ON);
  lv_obj_set_style_width(list, 20, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(list, lv_color_hex(0x3A4552), LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_opa(list, LV_OPA_70, LV_PART_SCROLLBAR);
  lv_obj_set_style_radius(list, 6, LV_PART_SCROLLBAR);
}

}  // namespace ui
