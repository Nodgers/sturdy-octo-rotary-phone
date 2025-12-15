# ESPHome Battery Gauge

A physical, servo-driven battery gauge powered by **ESPHome** and **Home Assistant**.

This project reads live energy data from Home Assistant (battery SoC, charge/discharge state, power flow) and drives:
- a **positional servo** as a classic analogue battery gauge
- an **OLED display** showing SoC, charge state, power flow, and reserve level

It’s designed to feel like a real instrument rather than a twitchy dashboard.

---

## ✨ Features

- ESP32 + ESPHome
- Physical servo needle mapped to battery state of charge
- OLED display with:
  - Large SoC percentage
  - Charging / discharging / idle state
  - Animated power-flow indicator
  - Backup reserve marker
- Calm behaviour:
  - Servo updates throttled
  - Hysteresis on charge/discharge state
- Clean, modular ESPHome config
- VS Code build + upload workflow

---

## 🧰 Hardware

- ESP32 (tested with `esp32dev`)
- Positional servo (e.g. MF90S / SG90-class)
- SSD1306 128×64 I²C OLED
- External 5 V supply recommended for servos
- Common ground between ESP32 and servo PSU

> ⚠️ Do **not** power servos directly from the ESP32 when adding more than one.

---

## 🔌 Wiring (basic)

| Component | ESP32 |
|--------|------|
| OLED SDA | GPIO21 |
| OLED SCL | GPIO22 |
| Servo PWM | GPIO18 |
| Servo GND | GND |
| Servo V+ | External 5 V |

---

## 🧠 Home Assistant Requirements

You’ll need entities equivalent to:

- Battery state of charge (%)
- Battery power (kW)
- Solar power (kW)
- Grid power (kW)
- Backup reserve (%)

These are referenced via `sensor:` and `number:` entries in ESPHome.

---

## 📁 Project Structure

```text
esphome/
├─ battery-gauge.yaml
├─ battery_gauge/
│  ├─ base.yaml
│  ├─ servo.yaml
│  ├─ sensors.yaml
│  ├─ display.yaml
│  └─ display_draw.h
├─ secrets.example.yaml
├─ .gitignore
└─ .vscode/
   └─ tasks.json