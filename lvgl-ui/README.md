# LVGL Editor Project Folder

This folder is a dedicated LVGL XML project.

- Top-level `globals.xml` is present (required by the LVGL VS Code extension).
- `project.xml` defines a 480x480 RGB565 target.
- Starter screen XML: `screens/dashboard.xml`.

## How To Use In VS Code

1. Open workspace file `esp32-s3-gauge.code-workspace`.
2. Run `LVGL: Open Editor` from Command Palette.
3. Optional debug output: run `Output: Show Output Channels`, then select `LVGL Editor`.
4. In the LVGL editor, click `Generate code`.
5. In the LVGL editor, click `Compile project`.

Notes:
- The preview panel renders any LVGL XML file you open.
- If compile fails once, run compile again (known LVGL extension issue).
