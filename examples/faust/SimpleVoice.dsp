// SimpleVoice: a whole voice written in Faust, played from the keyboard.

declare name "SimpleVoice";
declare category "osc";
declare input_names "V/Oct, Gate, Vel";
declare output_names "Out";

import("stdfaust.lib");

// the knobs, in this order on the panel ([1] … set the order)
cutoff  = hslider("[1]Cutoff[unit:Hz][scale:log]", 600, 50, 10000, 1) : si.smoo;
res     = hslider("[2]Resonance", 0.3, 0, 0.9, 0.01) : si.smoo;
amount  = hslider("[3]Env amount[unit:oct]", 3, 0, 6, 0.1);
attack  = hslider("[4]Attack[unit:s][scale:log]", 0.005, 0.001, 2, 0.001);
decay   = hslider("[5]Decay[unit:s][scale:log]", 0.4, 0.005, 4, 0.001);
sustain = hslider("[6]Sustain", 0.5, 0, 1, 0.01);
release = hslider("[7]Release[unit:s][scale:log]", 0.3, 0.005, 6, 0.001);

// 1 V an octave, 0 V = C4 (261.63 Hz)
hz(v) = 261.6256 * 2 ^ v;

process(pitch, gate, vel) = osc : ve.moog_vcf_2bn(res, cut) : *(env * level * 5)
with {
    f = hz(pitch);
    // a gate is high from 1 V
    env = en.adsr(attack, decay, sustain, release, gate > 1);
    // velocity 0–10 V: from half to full level
    level = 0.5 + vel / 20;
    // the envelope opens the filter by up to `amount` octaves
    cut = min(cutoff * 2 ^ (amount * env * level), 15000);
    osc = 0.7 * os.sawtooth(f) + 0.3 * os.square(f / 2);
};
