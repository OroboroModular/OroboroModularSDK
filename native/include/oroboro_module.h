/*
 * The Oroboro Modular module ABI, version 1: what a compiled module's
 * native library exports (docs/native-modules.md). The plugin loads it from
 * native/<Vendor>/ in its modules folder, makes one instance per voice and
 * calls oroboro_module_tick once per sample.
 *
 * Rules:
 *  - C calling convention, no exceptions or unwinding across these
 *    functions (catch them inside; a module that fails should fall silent).
 *  - Signals are volts: audio about +-5 V, gates 0/10 V, pitch 1 V an octave
 *    with 0 V = C4. Inputs named "V/Oct", "Gate" and "Vel" receive the
 *    voice's key while unpatched (the plugin does it).
 *  - The spec's JSON is the interface (module, category, inputs, outputs,
 *    params, source_hash); it must stay valid for the library's lifetime.
 *  - Nothing here is called from two threads for one instance at once; the
 *    plugin may call it from its audio thread, so tick and set_param must
 *    not block or allocate.
 */
#ifndef OROBORO_MODULE_H
#define OROBORO_MODULE_H

#include <stdint.h>

#ifdef _WIN32
#define OROBORO_EXPORT __declspec(dllexport)
#else
#define OROBORO_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define OROBORO_MODULE_ABI 1

/* The ABI version this library implements: 1. */
OROBORO_EXPORT uint32_t oroboro_module_abi(void);

/* The module's interface, as JSON (UTF-8, NUL-terminated), e.g.
 * {"module": "native/Acme/Fold", "category": "shaper",
 *  "inputs": [{"name": "In"}], "outputs": ["Out"],
 *  "params": [{"name": "Fold", "min": 1, "max": 10, "default": 1,
 *              "unit": "", "scaling": "lin"}],
 *  "source_hash": null}
 * A knob with "scaling": "enum" counting from 0 may name its positions:
 * "labels": ["Sine", "Saw"]. Its name says who makes it,
 * native/<Vendor>/<Name> (the plugin loads nothing else), and "vendor" may
 * say it again ("vendor": "Acme"): Add module lists compiled modules by
 * maker, then by category. "midi": true says it takes MIDI
 * (oroboro_module_midi). What a jack carries (its colour) goes by its
 * name; one whose name doesn't tell says it: an input
 * {"name": "Kick", "signal": "logic"}, an output the same object in place
 * of its name ("audio", "control" or "logic"). */
OROBORO_EXPORT const char *oroboro_module_spec(void);

/* A new instance at sample_rate, its knobs at their defaults; NULL if it
 * can't be made. */
OROBORO_EXPORT void *oroboro_module_new(float sample_rate);

/* Frees an instance from oroboro_module_new. */
OROBORO_EXPORT void oroboro_module_free(void *module);

/* Knob `index` (the spec's order) is `value`, in its own units. */
OROBORO_EXPORT void oroboro_module_set_param(void *module, uint32_t index, float value);

/* Forget the past: silence. */
OROBORO_EXPORT void oroboro_module_reset(void *module);

/* One sample: `inputs` has one value per input of the spec, `outputs` room
 * for one per output (write each). */
OROBORO_EXPORT void oroboro_module_tick(void *module, const float *inputs, float *outputs);

/* Optional (a library may leave it out): which inputs have something to
 * read, one byte per input of the spec (0 or 1): a cable, or the key for an
 * unpatched V/Oct, Gate or Vel. Told when the patch's cables change, before
 * the next tick. */
OROBORO_EXPORT void oroboro_module_connected(void *module, const uint8_t *inputs, uint32_t count);

/* Optional, for a module whose spec says "midi": true: one MIDI message
 * (its status byte and data bytes, `size` of them: 1 to 3), on the audio
 * thread, before the tick of the sample it falls on. In the voice area an
 * instance gets its own voice's notes (its note on and note off, and the
 * poly pressure of its key) and every channel's other messages (pitch
 * bend, channel pressure, controllers, program changes); in the FX area,
 * every message the plugin gets. */
OROBORO_EXPORT void oroboro_module_midi(void *module, const uint8_t *message, uint32_t size);

/* Optional: what the module keeps beyond its knobs (settings of its own),
 * as text (JSON by custom). The plugin keeps it with the patch and gives
 * it back with set_state to every instance it makes, before their first
 * tick. get_state's text is the library's, valid until the next call for
 * that instance; NULL when there's nothing to keep. Never called from the
 * audio thread. */
OROBORO_EXPORT const char *oroboro_module_get_state(void *module);
OROBORO_EXPORT void oroboro_module_set_state(void *module, const char *state);

/* Optional: knob `index` as the instance has it (a menu's choice may move one). */
OROBORO_EXPORT float oroboro_module_get_param(void *module, uint32_t index);

/* Optional: the module's own menu of options, as JSON (UTF-8), for the
 * plugin to show in the module's menu; NULL when it has none. An array of
 *   {"kind": "item", "id": 3, "text": "Slow mode", "right": "", "checked": true,
 *    "disabled": false, "items": [ …the menu it opens, if it opens one… ]}
 *   {"kind": "slider", "id": 4, "text": "Feedback: 40%", "value": 0.4}
 *   {"kind": "label", "text": "Range"}   {"kind": "separator"}
 * menu_choose chooses the entry `id` of the last menu (for a slider,
 * `value` 0..1 is where it's set). These are asked of an instance the
 * plugin keeps for the module's face, which it never ticks: after a
 * choice it reads get_state from it and gives that to the playing ones.
 * Never called from the audio thread. */
OROBORO_EXPORT const char *oroboro_module_menu(void *module);
OROBORO_EXPORT void oroboro_module_menu_choose(void *module, uint32_t id, float value);

/* Optional: a panel of the module's own, as JSON: its size, and where each
 * knob, jack and light is on it (pixels from its top left corner; a panel
 * 380 high is drawn as tall as a Rack module):
 *   {"width": 90, "height": 380,
 *    "params":  [{"index": 0, "x": 30, "y": 40, "w": 30, "h": 30, "kind": "knob"}],
 *    "inputs":  [{"index": 0, "x": 8, "y": 300, "w": 24, "h": 24}],
 *    "outputs": [{"index": 0, "x": 58, "y": 300, "w": 24, "h": 24}],
 *    "lights":  [{"first": 0, "x": 42, "y": 20, "w": 6, "h": 6, "colors": ["#90c73e"]}]}
 * `kind` is knob, slider, switch or button. A light with several colours
 * is as many of the module's lights, from `first` on. Asked of the
 * instance the plugin keeps for the module's face. NULL: the plugin lays
 * the module out its own way.
 * With "style": "stock" the panel is in the plugin's own look (its body
 * and title, no picture), and may have labels and lines, in the theme's
 * colours:
 *    "texts": [{"x": 30, "y": 60, "text": "Rate", "size": 8.5, "anchor": "center"}],
 *    "lines": [{"x1": 8, "y1": 90, "x2": 247, "y2": 90}] */
OROBORO_EXPORT const char *oroboro_module_panel(void *module);

/* Optional: the panel's picture, `*size` bytes (the library's own, for as
 * long as it's loaded), drawn behind the controls of oroboro_module_panel:
 * an SVG's text (the plugin draws it as big as it shows it, in its theme's
 * colours: paths, shapes, fills, strokes, gradients; no text elements) or
 * a PNG (shown as it is). NULL: a plain panel. */
OROBORO_EXPORT const uint8_t *oroboro_module_art(uint32_t *size);

/* Optional: the module's own pictures (SVG files' text), by number, which
 * what it draws may draw (a drawing's picture entry, kind 4); null past
 * the last. */
OROBORO_EXPORT const uint8_t *oroboro_module_picture(uint32_t index, uint32_t *size);

/* Optional: how bright the module's lights are, 0 to 1 each, into `out`
 * (room for `count`). Returns how many lights the module has. Called
 * between ticks, on the audio thread: copy numbers, nothing else. */
OROBORO_EXPORT uint32_t oroboro_module_lights(void *module, float *out, uint32_t count);

/* Optional: what the module draws on its panel itself, now (a screen, a
 * scope): a drawing, `*count` numbers (the library's own, valid until the
 * next call for that instance), which the plugin draws over the panel's
 * picture and under its controls, each frame. Asked of an instance that
 * plays, on the plugin's own thread while the audio thread ticks it: read
 * what the module shows the way a panel reads it, and change nothing.
 * The list is entries of `kind, n` and then `n` numbers, in the panel's
 * pixels:
 *   1 a fill:   clip (4), paint (19), then outlines: count, hole, x y × count
 *   2 a stroke: clip (4), paint (19), width, cap, join, miter limit,
 *               then lines: count, closed, x y × count
 *   3 a text:   clip (4), r g b a, size, align, x, y, angle, face, its bytes
 *   4 a picture: clip (4), which, its width and height, a b c d e f
 * A clip is left, top, right, bottom (none while left > right). A paint:
 * its kind (0 a colour, 1 a gradient), the inner and the outer colour
 * (r g b a each), the way from the panel's pixels to where it's laid out
 * (6), its box's half sizes (2), its corners' radius, its feather. `cap`
 * is 0 butt, 1 round, 2 square; `join` 1 round, 3 bevel, 4 miter; `align`
 * 1 left, 2 centre, 4 right with 8 top, 16 middle, 32 bottom, 64 baseline;
 * `face` 1 typewriter letters, 2 bold. A picture is one of the module's
 * own (`which`: its number for oroboro_module_picture), drawn at its width
 * and height of its own pixels through a b c d e f onto the panel's
 * (x' = a x + c y + e, y' = b x + d y + f, as NanoVG's transforms are).
 * Colours are told to the plugin's
 * theme as the panel's are. NULL: it draws nothing. */
OROBORO_EXPORT const float *oroboro_module_draw(void *module, uint32_t *count);

/* Optional: the pointer on the module's own panel, on what it draws itself
 * (a screen, a grid of steps), given to the instance that plays, at (x, y)
 * in the panel's pixels. `kind`: 1 a button pressed (`button` 0 the
 * primary, 1 the secondary, 2 the middle; `mods` 1 Shift, 2 Ctrl, 4 Alt),
 * 2 let go, 3 moved by (dx, dy) (a drag while a press the module took is
 * held, else the pointer over the panel), 4 the wheel turned by (dx, dy),
 * 5 a double click, 6 off the panel. Returns 1 when the module took it: a
 * press it takes is its drag, not the plugin's (the plugin moves the module
 * by a press it doesn't take, and turns its own knobs and plugs its own
 * jacks where the panel puts them). What the pointer changed in the
 * module's state or knobs is read back afterwards and kept with the patch.
 * When `mods` has OROBORO_POINTER_MAY_ASK, it's asked on a thread of the
 * plugin's own, where a dialog may open and the plugin waits for it (a
 * press, a let-go, a double click); else on the plugin's window thread,
 * where no dialog may open. */
#define OROBORO_POINTER_MAY_ASK (1u << 16)
OROBORO_EXPORT uint32_t oroboro_module_pointer(void *module, uint32_t kind, float x, float y, float dx, float dy,
                                               uint32_t button, uint32_t mods);

/* Optional: 1 when the module's menu (oroboro_module_menu, _menu_choose),
 * its state (_get_state, _set_state) and its knobs (_get_param) may be
 * asked of an instance while it plays, on another thread than the audio
 * thread's (as Rack asks a module from its window): then a menu's choice
 * is done on the instance that plays, and what it changes is kept with
 * the patch without making the module afresh. Without it (or 0), they're
 * asked only of an instance that doesn't play, and a changed state makes
 * the playing instances afresh. A module that says 1 takes care of its
 * own threads. */
OROBORO_EXPORT uint32_t oroboro_module_live(void);

#ifdef __cplusplus
}
#endif

#endif
