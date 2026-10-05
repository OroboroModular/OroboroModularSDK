// TapeEcho: an example of a module of your own, written in Faust.
//
// Faust's documentation: https://faustdoc.grame.fr (the libraries used

// ---- what the module is called and what jacks it has ---------------------
declare name "TapeEcho";             // "faust/TapeEcho" in saved patches
declare category "fx";               // the Add module group: osc, filter, env, lfo, level, mixer, fx, …
declare input_names "In, Time CV";   // the inputs, in the order process takes them
declare output_names "L, R";         // the outputs, in the order process gives them

import("stdfaust.lib");

// ---- the knobs ---------------------------------------------------------
// Every slider is a knob on the panel, in real units:
//     hslider("Name[unit:…]", default, min, max, step)
// [unit:ms] is printed with the value; [scale:log] makes the knob sweep
// evenly in ratios, which suits times and frequencies. si.smoo glides to
// a new value instead of jumping (a jumping delay time clicks).
//
// The [1], [2] … at the front set the knobs' order on the panel (Faust
// would sort them by name otherwise); they don't show. The order also
// numbers the knobs for saved patches and macros, so give a new knob the
// next number rather than renumbering.
time  = hslider("[1]Time[unit:ms][scale:log]", 350, 20, 1000, 1) : si.smoo;
fb    = hslider("[2]Feedback[unit:%]", 45, 0, 105, 1) / 100 : si.smoo;
tone  = hslider("[3]Tone[unit:Hz][scale:log]", 3500, 300, 12000, 1) : si.smoo;
drive = hslider("[4]Drive[unit:%]", 25, 0, 100, 1) / 100 : si.smoo;
wow   = hslider("[5]Wow[unit:%]", 30, 0, 100, 1) / 100;
mix   = hslider("[6]Mix[unit:%]", 35, 0, 100, 1) / 100 : si.smoo;
// a checkbox is an on/off button
pp    = checkbox("[7]Ping-pong");

// ---- signals are volts ---------------------------------------------------
// As with Oroboro's own modules: audio is about ±5 V, modulation ±5 V,
// gates 0 or 10 V, pitch 1 V an octave with 0 V at C4. A cable from a
// legacy G2 module is converted on the way, so the same levels arrive here.

// Time CV: every 5 V doubles the delay time (-5 V halves it)
delay_ms(cv) = time * 2 ^ (cv / 5);

// wow (a slow wobble) and flutter (a fast one), in milliseconds
wobble_ms = wow * (1.5 * os.osc(0.5) + 0.2 * os.osc(6.7));

// the longest delay in samples, a power of two: 2.7 s at the engine's 96 kHz
N = 262144;

// the delay in samples; ma.SR is the rate the engine runs at
samples(cv) = (delay_ms(cv) + wobble_ms) * ma.SR / 1000 : max(1) : min(N - 2);

// the tape: soft saturation, the same gain for small signals, rounding
// off the peaks from about ±10 V (Drive 0) down to ±2.5 V (Drive 100 %)
sat(x) = 10 * ma.tanh(x * g / 10) / g
with {
    g = 1 + 3 * drive;
};

// one pass along the tape: the delay, darker, without rumble, saturated
pass(cv) = de.fdelay(N, samples(cv)) : fi.lowpass(2, tone) : fi.highpass(1, 60) : sat;

// the feedback path: each line back into itself, or with Ping-pong into
// the other one. (a, b) mix into (a, b), or swap into (b, a).
bounce(p, a, b) = a * (1 - p) + b * p, b * (1 - p) + a * p;

// two lines, left and right. "~" is a feedback loop: what comes out of the
// right-hand side goes back into the left-hand side's first inputs, one
// sample later (as any feedback loop in a patch).
echoes(cv) = (ro.interleave(2, 2) : (+ : pass(cv)), (+ : pass(cv))) ~ (*(fb), *(fb) : bounce(pp));

// dry and wet mixed for one side
side(dry, wet) = dry * (1 - mix) + wet * mix;

// the module: In and Time CV in, L and R out. With Ping-pong the input
// goes only into the left line, so the repeats alternate.
process(x, cv) = (x, x * (1 - pp)) : echoes(cv) : (side(x), side(x));
