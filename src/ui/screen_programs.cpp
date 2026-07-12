// screen_programs.cpp — see screen_programs.h.
#include "screen_programs.h"

#include <cstdio>
#include <cstring>

#include "ui/ui_root.h"
#include "ui/list_style.h"

namespace ui {
namespace {
constexpr int MAXP = 12;
lv_obj_t* s_screen = nullptr;
lv_obj_t* s_list = nullptr;
char s_names[MAXP][24];
int s_n = 0;

void done_cb(lv_event_t*) { ui::show_presets(); }
void run_cb(lv_event_t* e) {
  ui::run_program((int)(intptr_t)lv_event_get_user_data(e));   // 0 = built-in
}

void rebuild() {
  if (!s_list) return;
  lv_obj_clean(s_list);
  for (int i = 0; i <= s_n; ++i) {   // slot 0 = built-in
    lv_obj_t* row = lv_btn_create(s_list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 48);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x2E5AAC), 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_add_event_cb(row, run_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    char txt[64];
    if (i == 0)
      std::snprintf(txt, sizeof(txt),
                    LV_SYMBOL_PLAY "  Smash Burgers x4\n#8a93a0 built-in#");
    else
      std::snprintf(txt, sizeof(txt), LV_SYMBOL_PLAY "  %s\n#8a93a0 saved#",
                    s_names[i - 1]);
    lv_obj_t* l = lv_label_create(row);
    lv_label_set_recolor(l, true);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 4, 0);
  }
  if (s_n == 0) {
    lv_obj_t* note = lv_label_create(s_list);
    lv_label_set_text(note,
        "Build more in the web Recipe Creator\n(http://<device-ip>/creator)");
    lv_obj_set_style_text_font(note, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(note, lv_color_hex(0x8A93A0), 0);
  }
}
}  // namespace

const char* programs_name(int i) {
  return (i >= 0 && i < s_n) ? s_names[i] : "";
}

void programs_set_list(const char (*names)[24], int n) {
  s_n = n > MAXP ? MAXP : (n < 0 ? 0 : n);
  for (int i = 0; i < s_n; ++i) {
    std::strncpy(s_names[i], names[i], 23);
    s_names[i][23] = '\0';
  }
  rebuild();
}

lv_obj_t* programs_create() {
  if (s_screen) return s_screen;
  lv_obj_t* scr = lv_obj_create(nullptr);
  s_screen = scr;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Recipe programs");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x8A93A0), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 10);

  lv_obj_t* done = lv_btn_create(scr);
  lv_obj_set_size(done, 84, 30);
  lv_obj_align(done, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_obj_add_event_cb(done, done_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* dl = lv_label_create(done);
  lv_label_set_text(dl, "Back");
  lv_obj_center(dl);

  s_list = lv_list_create(scr);
  lv_obj_set_size(s_list, 468, 272);
  lv_obj_align(s_list, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_obj_set_style_bg_color(s_list, lv_color_hex(0x101418), 0);
  lv_obj_set_style_pad_row(s_list, 4, 0);
  apply_scroll_list_style(s_list);
  rebuild();
  return s_screen;
}

}  // namespace ui
