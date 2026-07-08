/* rows_sliders_input.h — fragment #included inside a cheat_entry array initializer.
 * No include guard. Field order: category, title, api, description, snippet, render. */

{ "Sliders & Drags", "Slider (float)",
  "bool eli_slider_float(label, float *v, v_min, v_max, fmt, flags)",
  "A horizontal slider over a float in [v_min, v_max]. fmt is a printf readout.",
  "static float v = 0.5f;\n"
  "eli_slider_float(\"amount\", &v, 0.0f, 1.0f, \"%.2f\", 0);",
  cheat_render_slider_float },

{ "Sliders & Drags", "Slider (int)",
  "bool eli_slider_int(label, int *v, v_min, v_max, fmt, flags)",
  "A horizontal slider over an int in [v_min, v_max].",
  "static int v = 5;\n"
  "eli_slider_int(\"level\", &v, 0, 10, \"%d\", 0);",
  cheat_render_slider_int },

{ "Sliders & Drags", "Slider (angle)",
  "bool eli_slider_angle(label, float *v_rad, deg_min, deg_max, fmt, flags)",
  "Slider that stores radians but displays and edits in degrees.",
  "static float angle = 0.0f;\n"
  "eli_slider_angle(\"rotation\", &angle, -180.0f, 180.0f, \"%.0f deg\", 0);",
  cheat_render_slider_angle },

{ "Sliders & Drags", "Slider (float3)",
  "bool eli_slider_float3(label, float v[3], v_min, v_max, fmt, flags)",
  "Three float sliders side by side sharing one range and label.",
  "static float rgb[3] = {0.4f, 0.7f, 1.0f};\n"
  "eli_slider_float3(\"color\", rgb, 0.0f, 1.0f, \"%.2f\", 0);",
  cheat_render_slider_float3 },

{ "Sliders & Drags", "Vertical Slider (float)",
  "bool eli_v_slider_float(label, eli_vec2 size, float *v, v_min, v_max, fmt, flags)",
  "A vertical slider of an explicit pixel size; higher position = larger value.",
  "static float v = 0.5f;\n"
  "eli_v_slider_float(\"##vol\", eli_make_vec2(18.0f, 80.0f), &v, 0.0f, 1.0f, \"%.2f\", 0);",
  cheat_render_v_slider_float },

{ "Sliders & Drags", "Drag (float)",
  "bool eli_drag_float(label, float *v, v_speed, v_min, v_max, fmt, flags)",
  "Click-drag to change a float; v_speed is the value change per pixel dragged.",
  "static float x = 10.0f;\n"
  "eli_drag_float(\"x pos\", &x, 0.5f, -100.0f, 100.0f, \"%.1f\", 0);",
  cheat_render_drag_float },

{ "Sliders & Drags", "Drag (int)",
  "bool eli_drag_int(label, int *v, v_speed, v_min, v_max, fmt, flags)",
  "Click-drag to change an int; v_speed scales the per-pixel step.",
  "static int count = 8;\n"
  "eli_drag_int(\"count\", &count, 0.2f, 0, 100, \"%d\", 0);",
  cheat_render_drag_int },

{ "Sliders & Drags", "Drag Float Range",
  "bool eli_drag_float_range2(label, float *v_min, float *v_max, speed, min, max, fmt, fmt_max, flags)",
  "Two float drags editing a [lo, hi] range side by side, kept ordered.",
  "static float lo = 20.0f;\n"
  "static float hi = 80.0f;\n"
  "eli_drag_float_range2(\"range\", &lo, &hi, 0.5f, 0.0f, 100.0f, \"%.1f\", NULL, 0);",
  cheat_render_drag_float_range2 },

{ "Input", "Input Text",
  "bool eli_input_text(label, char *buf, buf_size, flags, callback, user_data)",
  "Single-line text editor into a caller-owned NUL-terminated buffer.",
  "static char buf[128] = \"hello\";\n"
  "eli_input_text(\"name\", buf, sizeof(buf), ELI_INPUT_TEXT_NONE, NULL, NULL);",
  cheat_render_input_text },

{ "Input", "Input Text with Hint",
  "bool eli_input_text_with_hint(label, hint, char *buf, buf_size, flags, callback, user_data)",
  "Single-line text editor with a placeholder hint shown when empty and inactive.",
  "static char buf[128] = \"\";\n"
  "eli_input_text_with_hint(\"search\", \"type here...\",\n"
  "    buf, sizeof(buf), ELI_INPUT_TEXT_NONE, NULL, NULL);",
  cheat_render_input_text_with_hint },

{ "Input", "Input Text Multiline",
  "bool eli_input_text_multiline(label, char *buf, buf_size, eli_vec2 size, flags, callback, user_data)",
  "A multi-line text editor. Size determines the frame dimensions (0 = defaults).",
  "static char buf[256] = \"line one\\nline two\";\n"
  "eli_input_text_multiline(\"notes\", buf, sizeof(buf),\n"
  "    eli_make_vec2(-1.0f, 60.0f), ELI_INPUT_TEXT_NONE, NULL, NULL);",
  cheat_render_input_text_multiline },

{ "Input", "Input Int",
  "bool eli_input_int(label, int *v, step, step_fast, flags)",
  "Numeric field for an int with +/- step buttons (hidden when step == 0).",
  "static int v = 42;\n"
  "eli_input_int(\"count\", &v, 1, 10, ELI_INPUT_TEXT_NONE);",
  cheat_render_input_int },

{ "Input", "Input Float",
  "bool eli_input_float(label, float *v, step, step_fast, fmt, flags)",
  "Numeric field for a float with +/- step buttons (hidden when step == 0).",
  "static float v = 1.5f;\n"
  "eli_input_float(\"scale\", &v, 0.1f, 1.0f, \"%.3f\", ELI_INPUT_TEXT_NONE);",
  cheat_render_input_float },

{ "Input", "Input Double",
  "bool eli_input_double(label, double *v, step, step_fast, fmt, flags)",
  "Numeric field for a double with +/- step buttons (hidden when step == 0).",
  "static double v = 3.14159;\n"
  "eli_input_double(\"pi\", &v, 0.01, 1.0, \"%.5f\", ELI_INPUT_TEXT_NONE);",
  cheat_render_input_double },

{ "Input", "Input Float3",
  "bool eli_input_float3(label, float v[3], fmt, flags)",
  "Three float inputs side by side sharing one label; no step buttons.",
  "static float v[3] = {0.0f, 0.0f, 0.0f};\n"
  "eli_input_float3(\"position\", v, \"%.2f\", ELI_INPUT_TEXT_NONE);",
  cheat_render_input_float3 },
