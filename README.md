# esp32-s3-gauge

Toy dashboard gauge project for the Waveshare **ESP32-S3-Touch-LCD-2.1**.

This scaffold gives you:
- LVGL + RGB display bring-up for the ST7701 480x480 panel
- CST820 touch polling
- QMI8658 IMU sampling (roll/pitch/yaw estimate)
- LPS22HB pressure sampling (altitude + min/max altitude)
- Toy speed estimate from IMU acceleration
- Built-in default gauge UI
- Optional LVGL editor generated UI import path

## Prerequisites

- VS Code extension: `espressif.esp-idf-extension`
- LVGL editor extensions (optional but recommended):
- `LVGL.lvgl-editor`
- `LVGL.lvgl-project-creator-vscode`
- ESP-IDF v5.1+ installed/configured in the Espressif extension

## First Run

1. Open this folder in VS Code.
2. Use `ESP-IDF: Set Espressif Device Target` and select `esp32s3`.
3. Use `ESP-IDF: Build your project`.
4. Flash from the extension or terminal:

```powershell
idf.py -p COMx flash monitor
```

The default dashboard UI starts automatically.

## LVGL Editor Workflow

Workspace folder setup is already included:
- Open `esp32-s3-gauge.code-workspace`
- It includes one LVGL project folder: `ui`
- `ui` has top-level `globals.xml` (required by the extension)

Then in VS Code:
- Run `LVGL: Open Editor` from Command Palette
- If that command is missing, open an XML in `ui` first, then run it again
- Use `Output: Show Output Channels` and pick `LVGL Editor` for logs
- Click `Generate code`
- Click `Compile project` (first compile can take time)
- If compile fails once, retry compile (known issue)

1. Design/export your UI as C files from the LVGL editor (or SquareLine export).
2. Run VS Code task: `LVGL: Import Editor Export`.
3. Provide the export folder path when prompted.
4. Enable `Gauge App Settings -> Use LVGL editor generated UI` in menuconfig.
5. Rebuild and flash.

Imported files are placed under:
- `main/ui/editor/generated`

Build logic auto-detects `ui.h` there and compiles generated files.

### Fast Editor Preview + Device Loop

Use this when iterating UI quickly in LVGL Editor:

1. Open `esp32-s3-gauge.code-workspace`.
2. Open `ui/screens/dashboard.xml`.
3. Run `LVGL: Open Editor` and use the live preview pane.
4. Click `Generate code` in LVGL Editor.
5. Run VS Code task: `LVGL: Import ui`.
6. Flash device with task: `ESP-IDF: Flash + Monitor`.

Notes:
- `ui` preview is your editor design surface.
- Firmware default dashboard in `main/ui/dashboard_ui.c` is code-driven.
- If you want the generated editor UI to run on device, enable:
  `Gauge App Settings -> Use LVGL editor generated UI`.

## Config Knobs

Use `idf.py menuconfig`:
- `Gauge App Settings -> Sensor loop period (ms)`
- `Gauge App Settings -> Sea-level pressure (hPa x10)`
- `Gauge App Settings -> Auto-baseline sea-level pressure at startup`
- `Gauge App Settings -> Speed damping (permil)`
- `Gauge App Settings -> LCD backlight percent`

## Hardware Pin Map (Waveshare ESP32-S3-Touch-LCD-2.1)

- RGB data/clock pins: copied from Waveshare demo for this board
- LCD init SPI: `MOSI=1`, `SCLK=2`, CS via EXIO
- I2C bus: `SDA=15`, `SCL=7`
- Touch INT: `GPIO16`
- Backlight PWM: `GPIO6`
- EXIO expander: TCA9554 at `0x20`
- IMU: QMI8658 at `0x6B`
- Pressure sensor: LPS22HB at `0x5C`

## Notes

- Speed is currently a toy inertial estimate and will drift over time.
- Altitude is pressure-based; accuracy depends on weather and calibration.
- If touch axes are inverted for your board revision, update `main/board/board_touch.c`.
- Board reference: Waveshare ESP32-S3-Touch-LCD-2.1 wiki
  https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.1
