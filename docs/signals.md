# Signals, voices and timing

The rules every module works by.

## Signals are volts

Everything is in volts, like Eurorack:

| Signal | Range |
|---|---|
| Audio | about ±5 V |
| Modulation | ±5 V, or 0 to 10 V |
| Gates and triggers | 0 V off, 10 V on |
| Pitch | 1 V per octave, 0 V = C4 (261.63 Hz) |
| Velocity | 0 to 10 V |

The output module reaches full scale at 21.3 V, so a ±5 V voice sits
about 12 dB below it and leaves room for voices to add up.

## Inputs that follow the keyboard

With no cable plugged in, these inputs read the voice's key:

| Input name (any case) | Reads |
|---|---|
| `V/Oct` | the note's pitch, including bend, glide and vibrato |
| `Gate` | 0 or 10 V |
| `Vel` or `Velocity` | 0 to 10 V |

So a module can be a whole voice: an oscillator with an envelope plays
from the keyboard without any cables. A cable always wins over the key.

## Voices

The patch's voice area runs once per voice (1 to 32), and the FX area
runs once for all of them. Every instance of your module lives in one
voice, so it never needs to handle polyphony itself. Allocate memory when
the instance is created, not while it plays.

## One sample at a time

The plugin calls each module once per sample, in order. A feedback loop
has exactly one sample of delay. Keep anything that only depends on the
knobs out of the per-sample code where you can.

The sample rate is the host's, or double it with the plugin's "Engine at
96 kHz" switch. It never changes during an instance's life: a new rate
means new instances.

## Knobs

A knob has a range, a default and a unit in the module's own terms: 800
on a `Hz` knob means 800 Hz. Its scaling is `lin`, `exp` (minimum above
0, for frequencies and times) or `enum` (whole steps, for switches).

Knob changes reach your module on the audio thread, so don't block or
allocate there.

**Patches refer to knobs by position.** Add new knobs at the end, and
never reorder or remove knobs in a module that patches already use.
