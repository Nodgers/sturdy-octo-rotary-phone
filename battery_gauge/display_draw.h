#pragma once
#include "esphome.h"
#include <cmath>

namespace battery_gauge {

inline void draw_screen(esphome::display::Display &it,
                        esphome::font::Font *font_small,
                        esphome::font::Font *font_big,
                        float battery_pct,
                        float battery_kw,
                        float solar_kw,
                        float grid_kw,
                        float reserve_pct,
                        int &battery_flow_state,
                        float &flow_animation_phase) {

  using esphome::display::COLOR_OFF;

  it.fill(COLOR_OFF);

  // Nothing to render until sensors publish values.
  if (std::isnan(battery_pct)) {
    it.printf(0, 18, font_big, "--%%");
    it.printf(0, 54, font_small, "Waiting for data");
    return;
  }

  // Clamp noisy sensor input into expected ranges.
  if (battery_pct < 0) battery_pct = 0;
  if (battery_pct > 100) battery_pct = 100;

  if (std::isnan(reserve_pct)) reserve_pct = 20.0f;
  if (reserve_pct < 0) reserve_pct = 0;
  if (reserve_pct > 100) reserve_pct = 100;

  // ---------- Icon helpers ----------
  // Small helpers keep the drawing logic readable.
  auto thick_line = [&](int x1, int y1, int x2, int y2) {
    it.line(x1, y1, x2, y2);
    it.line(x1 + 1, y1, x2 + 1, y2);
  };

  auto draw_big_up_arrow = [&](int cx, int top_y) {
    thick_line(cx, top_y + 24, cx, top_y + 6);
    thick_line(cx, top_y, cx - 8, top_y + 8);
    thick_line(cx, top_y, cx + 8, top_y + 8);
  };

  auto draw_big_down_arrow = [&](int cx, int top_y) {
    thick_line(cx, top_y, cx, top_y + 18);
    thick_line(cx, top_y + 24, cx - 8, top_y + 16);
    thick_line(cx, top_y + 24, cx + 8, top_y + 16);
  };

  auto draw_big_sun = [&](int cx, int cy) {
    it.circle(cx, cy, 4);
    it.circle(cx, cy, 3);
    it.line(cx, cy - 9, cx, cy - 6);
    it.line(cx, cy + 6, cx, cy + 9);
    it.line(cx - 9, cy, cx - 6, cy);
    it.line(cx + 6, cy, cx + 9, cy);
    it.line(cx - 6, cy - 6, cx - 5, cy - 5);
    it.line(cx + 5, cy - 5, cx + 6, cy - 6);
    it.line(cx - 6, cy + 6, cx - 5, cy + 5);
    it.line(cx + 5, cy + 5, cx + 6, cy + 6);
  };

  auto draw_big_bolt = [&](int cx, int cy) {
    thick_line(cx + 3, cy - 10, cx - 3, cy - 2);
    thick_line(cx - 3, cy - 2, cx + 3, cy - 2);
    thick_line(cx + 3, cy - 2, cx - 3, cy + 10);
  };

  // ---------- Decide state with hysteresis ----------
  // Battery draw mode (charging/discharging) uses a little hysteresis to
  // stabilize the UI arrows.
  const float kChargeEnterKw = -0.08f;
  const float kDischargeEnterKw = 0.08f;
  const float kChargeExitKw = -0.03f;
  const float kDischargeExitKw = 0.03f;

  if (!std::isnan(battery_kw)) {
    if (battery_flow_state == 0) {
      if (battery_kw <= kChargeEnterKw) battery_flow_state = -1;
      else if (battery_kw >= kDischargeEnterKw) battery_flow_state = 1;
    } else if (battery_flow_state == -1) {
      if (battery_kw > kChargeExitKw) battery_flow_state = 0;
    } else if (battery_flow_state == 1) {
      if (battery_kw < kDischargeExitKw) battery_flow_state = 0;
    }
  }

  bool is_charging = (battery_flow_state == -1);
  bool is_discharging = (battery_flow_state == 1);

  bool from_grid = false;
  bool from_solar = false;

  if (is_charging) {
    if (!std::isnan(grid_kw) && grid_kw > 0.10f) from_grid = true;
    else if (!std::isnan(solar_kw) && solar_kw > 0.10f) from_solar = true;
  }

  // ---------- Top row ----------
  const char *mode_word = "Idle";
  if (is_discharging) mode_word = "Discharging";
  else if (is_charging) mode_word = "Charging";

  // Left: textual state, right: instantaneous power.
  it.printf(0, 0, font_small, "%s", mode_word);

  if (!std::isnan(battery_kw)) {
    it.printf(128, 0, font_small, esphome::display::TextAlign::TOP_RIGHT,
              "%.1f kW", battery_kw);
  }

  // ---------- Big percentage ----------
  it.printf(0, 16, font_big, "%.0f%%", battery_pct);

  // ---------- Animated flow ----------
  int arrow_top_y = 26;
  int icon_center_y = 40;

  const float kMaxKwForSpeed = 3.0f;
  float abs_battery_kw = (!std::isnan(battery_kw)) ? fabsf(battery_kw) : 0.0f;
  float speed_factor = std::min(std::max(abs_battery_kw / kMaxKwForSpeed, 0.0f), 1.0f);

  // The little circle slides along the track to show live power flow.
  flow_animation_phase += 0.020f + (0.14f * speed_factor);
  if (flow_animation_phase > 1.0f) flow_animation_phase -= 1.0f;

  auto draw_flow_track = [&](int x, int y_top, bool flow_downward) {
    int track_height = 22;
    int y0 = y_top + 2;
    int y1 = y0 + track_height;

    it.line(x, y0, x, y1);

    float t = flow_downward ? flow_animation_phase : (1.0f - flow_animation_phase);
    int y = y0 + int(t * track_height);

    it.filled_circle(x, y, 2);
    it.filled_circle(x, y0, 1);
    it.filled_circle(x, y1, 1);
  };

  if (is_discharging) {
    // Dropping stored energy: show flow from battery to loads.
    draw_big_down_arrow(104, arrow_top_y);
    draw_flow_track(118, arrow_top_y, true);
  } else if (is_charging) {
    // Charging: highlight source icon (solar/grid) and animate upward flow.
    draw_big_up_arrow(92, arrow_top_y);
    draw_flow_track(104, arrow_top_y, false);

    if (from_solar) draw_big_sun(114, icon_center_y);
    else if (from_grid) draw_big_bolt(114, icon_center_y);
    else it.filled_circle(114, icon_center_y, 2);
  } else {
    // Idle: draw simple circle instead of flow arrows.
    it.circle(104, icon_center_y, 7);
  }

  // ---------- Battery bar ----------
  int bar_left = 0, bar_top = 56, bar_width = 116, bar_height = 8;

  it.rectangle(bar_left, bar_top, bar_width, bar_height);
  it.rectangle(bar_left + bar_width + 2, bar_top + 2, 4, bar_height - 4);

  int filled_width = int((battery_pct / 100.0f) * bar_width);
  filled_width = std::min(std::max(filled_width, 0), bar_width);

  int reserve_marker_x = bar_left + int((reserve_pct / 100.0f) * bar_width);
  reserve_marker_x = std::min(std::max(reserve_marker_x, bar_left), bar_left + bar_width);

  // Marker line shows user-configured reserve threshold.
  it.line(reserve_marker_x, bar_top, reserve_marker_x, bar_top + bar_height - 1);

  int reserve_fill_limit = std::min(filled_width, reserve_marker_x - bar_left);
  for (int x = bar_left; x < bar_left + reserve_fill_limit; x += 2) {
    it.line(x, bar_top + 1, x, bar_top + bar_height - 2);
  }

  if (filled_width > reserve_marker_x - bar_left) {
    it.filled_rectangle(reserve_marker_x, bar_top + 1,
                        filled_width - (reserve_marker_x - bar_left),
                        bar_height - 2);
  }
}

}  // namespace battery_gauge
