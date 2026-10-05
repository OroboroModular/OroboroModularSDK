//! Fold: a wavefolder, an example compiled module (the SDK's
//! `docs/native-modules.md`). The input, plus Bias, times Fold, folds back
//! each time it passes ±5 V, then mixes with the dry input. Fold CV adds to
//! Fold (1 per volt).
//!
//! Build it with `cargo build --release -p fold-module`, then
//! `oromod install target/release/fold.dll` (or `libfold.so`,
//! `libfold.dylib`), with developer mode on in the plugin's Settings.

use oroboro_module::{export_module, Module, Param, Spec};

pub struct Fold {
    fold: f32,
    bias: f32,
    mix: f32,
}

/// `x` folded back into ±5 V.
fn fold_back(x: f32) -> f32 {
    // a triangle of period 20 V through ±5 V: the identity between them
    let t = (x + 5.0).rem_euclid(20.0);
    if t < 10.0 {
        t - 5.0
    } else {
        15.0 - t
    }
}

impl Module for Fold {
    fn spec() -> Spec {
        Spec::new("native/Fold")
            .vendor("Oroboro")
            .category("shaper")
            .input("In")
            .input("Fold CV")
            .output("Out")
            .param(Param::new("Fold", 1.0, 10.0, 1.0))
            .param(Param::new("Bias", -5.0, 5.0, 0.0).unit("V"))
            .param(Param::new("Mix", 0.0, 1.0, 1.0))
    }

    fn new(_sample_rate: f32) -> Self {
        Fold { fold: 1.0, bias: 0.0, mix: 1.0 }
    }

    fn set_param(&mut self, index: usize, value: f32) {
        match index {
            0 => self.fold = value,
            1 => self.bias = value,
            2 => self.mix = value,
            _ => {}
        }
    }

    fn tick(&mut self, inputs: &[f32], outputs: &mut [f32]) {
        let gain = (self.fold + inputs[1]).max(0.0);
        let wet = fold_back((inputs[0] + self.bias) * gain);
        outputs[0] = inputs[0] + (wet - inputs[0]) * self.mix;
    }
}

export_module!(Fold);

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn folds_back_past_five_volts() {
        assert_eq!(fold_back(3.0), 3.0);
        assert_eq!(fold_back(6.0), 4.0);
        assert_eq!(fold_back(-7.0), -3.0);
        let mut fold = Fold::new(48_000.0);
        let mut out = [0.0];
        fold.set_param(0, 2.0);
        fold.tick(&[3.0, 0.0], &mut out);
        assert_eq!(out, [4.0], "3 V times 2 is 6 V, folded to 4 V");
    }
}
