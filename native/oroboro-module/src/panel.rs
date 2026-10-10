//! A module's own panel, and what it shows on it as it plays.
//!
//! [`Panel`] says where the module's knobs, jacks and lights are (pixels
//! from its top left corner: a panel 255 wide is a column of the plugin's
//! canvas, one 380 tall a Rack module's height); the plugin draws its own
//! knobs and jacks there. [`Panel::stock`] is a panel in the plugin's own
//! look, as its own modules have (its body, the module's name at the top,
//! captions in its letters, a screen as a well of the theme); with
//! [`Panel::new`] the panel is the module's picture ([`crate::Module::art`]:
//! an SVG, drawn in the plugin's theme, its text elements in the plugin's
//! letters).
//!
//! A [`Screen`] is what the panel shows of the module as it plays (a
//! scope, a spectrum, a step grid) and what the pointer does there. The
//! plugin draws while the audio thread plays the module, so a screen is
//! shared between the two: the module keeps a handle to it and writes what
//! it shows into it (atomics, or a lock `tick` never waits on), and the
//! plugin asks it for a [`Drawing`] each frame on its own thread. A screen
//! never touches the module itself.

/// A colour, red, green, blue and alpha, 0 to 1 each. The plugin tells it
/// to its theme as the panel's picture's colours are: lettering stays as
/// clear on what it's drawn on, hues stay.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Color(pub f32, pub f32, pub f32, pub f32);

impl Color {
    /// From 0–255 values, opaque.
    pub fn rgb(r: u8, g: u8, b: u8) -> Color {
        Color(f32::from(r) / 255.0, f32::from(g) / 255.0, f32::from(b) / 255.0, 1.0)
    }

    /// The same colour, this opaque (0 to 1).
    pub fn alpha(self, a: f32) -> Color {
        Color(self.0, self.1, self.2, a.clamp(0.0, 1.0))
    }
}

/// How a text stands at its point.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Align {
    /// From the point, the point on its letters' line.
    Left,
    /// About the point.
    Center,
    /// Up to the point.
    Right,
}

impl Align {
    fn number(self) -> f32 {
        // left, centre, right; the point on the letters' middle
        match self {
            Align::Left => 1.0 + 16.0,
            Align::Center => 2.0 + 16.0,
            Align::Right => 4.0 + 16.0,
        }
    }
}

/// What a module draws on its panel itself, as the plugin reads it (the
/// list `oroboro_module_draw` gives: `include/oroboro_module.h`). In the
/// panel's pixels.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Drawing {
    numbers: Vec<f32>,
    /// What's drawn is cut to this (left, top, right, bottom); none while
    /// left > right.
    clip: Option<[f32; 4]>,
}

/// A paint's numbers: kind, inner and outer colour, the way from the
/// panel's pixels to where it's laid out (6), its box's half sizes (2),
/// its corners' radius, its feather.
fn solid(c: Color) -> [f32; 19] {
    [0.0, c.0, c.1, c.2, c.3, c.0, c.1, c.2, c.3, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0]
}

/// From `top` at `from` down to `bottom` at `to` (NanoVG's linear
/// gradient: a box far wider than anything, feathered over the way).
fn downwards(top: Color, bottom: Color, from: f32, to: f32) -> [f32; 19] {
    const LARGE: f32 = 1e5;
    let d = (to - from).abs().max(1e-3);
    let sign = if to >= from { 1.0 } else { -1.0 };
    // laid out at (0, from − LARGE) going down: from the panel's pixels, back
    let back = [1.0, 0.0, 0.0, sign, 0.0, sign * (LARGE - from * sign)];
    [
        1.0, top.0, top.1, top.2, top.3, bottom.0, bottom.1, bottom.2, bottom.3, back[0], back[1], back[2], back[3],
        back[4], back[5], LARGE, LARGE + d * 0.5, 0.0, d.max(1.0),
    ]
}

impl Drawing {
    pub fn new() -> Drawing {
        Drawing::default()
    }

    /// Cut what's drawn from here on to this box.
    pub fn clip(&mut self, left: f32, top: f32, right: f32, bottom: f32) {
        self.clip = Some([left, top, right, bottom]);
    }

    /// Draw anywhere on the panel from here on.
    pub fn no_clip(&mut self) {
        self.clip = None;
    }

    fn entry(&mut self, kind: f32, body: &[f32]) {
        let clip = self.clip.unwrap_or([1.0, 0.0, 0.0, 0.0]);
        self.numbers.push(kind);
        self.numbers.push((4 + body.len()) as f32);
        self.numbers.extend_from_slice(&clip);
        self.numbers.extend_from_slice(body);
    }

    fn outline(points: &[(f32, f32)], flag: f32, out: &mut Vec<f32>) {
        out.push(points.len() as f32);
        out.push(flag);
        for &(x, y) in points {
            out.push(x);
            out.push(y);
        }
    }

    /// The shape `points` go round, filled.
    pub fn fill(&mut self, points: &[(f32, f32)], color: Color) {
        self.fill_with(points, solid(color));
    }

    /// The shape `points` go round, filled from `top` at `from` (a y) down
    /// to `bottom` at `to`.
    pub fn fill_down(&mut self, points: &[(f32, f32)], top: Color, bottom: Color, from: f32, to: f32) {
        self.fill_with(points, downwards(top, bottom, from, to));
    }

    fn fill_with(&mut self, points: &[(f32, f32)], paint: [f32; 19]) {
        if points.len() < 3 {
            return;
        }
        let mut body = paint.to_vec();
        Drawing::outline(points, 0.0, &mut body);
        self.entry(1.0, &body);
    }

    /// A box, filled.
    pub fn rect(&mut self, x: f32, y: f32, w: f32, h: f32, color: Color) {
        self.fill(&[(x, y), (x + w, y), (x + w, y + h), (x, y + h)], color);
    }

    /// A line through `points`, `width` wide, round at its ends and bends.
    pub fn line(&mut self, points: &[(f32, f32)], width: f32, color: Color) {
        if points.len() < 2 {
            return;
        }
        let mut body = solid(color).to_vec();
        body.extend_from_slice(&[width.max(0.0), 1.0, 1.0, 10.0]);
        Drawing::outline(points, 0.0, &mut body);
        self.entry(2.0, &body);
    }

    /// `text` at (x, y), `size` pixels high, standing as `align` says (its
    /// middle at y). Set in the plugin's own letters.
    pub fn text(&mut self, x: f32, y: f32, text: &str, size: f32, color: Color, align: Align) {
        let mut body = vec![color.0, color.1, color.2, color.3, size, align.number(), x, y, 0.0, 0.0];
        body.extend(text.bytes().map(f32::from));
        self.entry(3.0, &body);
    }

    /// The numbers, as the ABI gives them.
    pub fn numbers(&self) -> &[f32] {
        &self.numbers
    }

    pub fn is_empty(&self) -> bool {
        self.numbers.is_empty()
    }
}

/// The pointer on the panel, in its pixels (what `oroboro_module_pointer`
/// gives).
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Pointer {
    /// A button pressed (0 the primary, 1 the secondary, 2 the middle;
    /// `mods`: 1 Shift, 2 Ctrl, 4 Alt).
    Press { x: f32, y: f32, button: u32, mods: u32 },
    Release { x: f32, y: f32, button: u32, mods: u32 },
    /// Moved by (dx, dy): a drag while a press the screen took is held,
    /// else the pointer over the panel.
    Move { x: f32, y: f32, dx: f32, dy: f32 },
    /// The wheel turned.
    Scroll { x: f32, y: f32, dx: f32, dy: f32 },
    DoubleClick { x: f32, y: f32 },
    /// Off the panel.
    Leave,
}

impl Pointer {
    /// From the ABI's numbers.
    pub fn of(kind: u32, x: f32, y: f32, dx: f32, dy: f32, button: u32, mods: u32) -> Option<Pointer> {
        // (the plugin's own flags, from bit 16 on: not keys held)
        let mods = mods & 0xffff;
        Some(match kind {
            1 => Pointer::Press { x, y, button, mods },
            2 => Pointer::Release { x, y, button, mods },
            3 => Pointer::Move { x, y, dx, dy },
            4 => Pointer::Scroll { x, y, dx, dy },
            5 => Pointer::DoubleClick { x, y },
            6 => Pointer::Leave,
            _ => return None,
        })
    }
}

/// What the panel shows of the module as it plays, and what the pointer
/// does there: shared with the plugin's window thread, which draws while
/// the audio thread plays (so `Sync`: write what it shows with atomics, or
/// a lock `tick` never waits on).
pub trait Screen: Send + Sync + 'static {
    /// What it shows now, into `d`. Each frame, on the window's thread.
    fn draw(&self, d: &mut Drawing);

    /// The pointer on the panel; whether the screen took it (a press it
    /// takes is its drag; one it doesn't, the plugin's: it moves the
    /// module). On the window's thread.
    fn pointer(&self, _event: Pointer) -> bool {
        false
    }

    /// Knob `index` as the screen has turned it, for a screen that turns
    /// its module's knobs (an equaliser's bands, dragged on its curve):
    /// kept with atomics the module reads too. The plugin reads them after
    /// each gesture on the screen and keeps those that moved in the patch.
    /// A module whose screen answers for one is asked here for all its
    /// knobs, never itself (it may be playing): answer None for the knobs
    /// the screen doesn't turn (the panel's, which a cable may be moving),
    /// and they're left as the patch has them. None for all (the default):
    /// the screen turns none.
    fn knob(&self, _index: usize) -> Option<f32> {
        None
    }
}

/// A place on a panel: what (by its number in the spec), its box, and for
/// a knob what kind.
#[derive(Clone, Debug, PartialEq)]
struct Place {
    index: usize,
    x: f32,
    y: f32,
    w: f32,
    h: f32,
    kind: &'static str,
}

/// A light's place and colours (`#rrggbb`): as many lights of the module's
/// as it has colours, from `first` on.
#[derive(Clone, Debug, PartialEq)]
struct LightPlace {
    first: usize,
    x: f32,
    y: f32,
    w: f32,
    h: f32,
    colors: Vec<String>,
}

/// A module's own panel: its size, and where its knobs, jacks and lights
/// are (each given by its middle). What it leaves out doesn't show.
#[derive(Clone, Debug, PartialEq)]
pub struct Panel {
    width: f32,
    height: f32,
    /// In the plugin's own look.
    stock: bool,
    params: Vec<Place>,
    inputs: Vec<Place>,
    outputs: Vec<Place>,
    lights: Vec<LightPlace>,
    /// Captions and texts: where (its middle, or its left end), what, how big.
    texts: Vec<(f32, f32, String, f32, bool)>,
    lines: Vec<[f32; 4]>,
    /// The screens (left, top, width, height): where what the module
    /// draws itself shows, on a stock panel.
    screens: Vec<[f32; 4]>,
    /// The last control or jack placed: what `label` names (kind, its box).
    last: Option<(&'static str, [f32; 4])>,
}

/// A jack's box (on a stock panel, the plugin's).
const JACK: f32 = 24.0;
const STOCK_JACK: f32 = 13.0;
/// The plugin's knob sizes, by name: small, medium (the usual), big.
pub const SMALL: f32 = 18.0;
pub const MEDIUM: f32 = 22.0;
pub const BIG: f32 = 26.0;
/// A caption's size under a control, over a jack.
const CAPTION: f32 = 8.5;
const JACK_NAME: f32 = 7.5;

impl Panel {
    /// A panel that's the module's picture (`Module::art`), `width` by
    /// `height`.
    pub fn new(width: f32, height: f32) -> Panel {
        Panel {
            width,
            height,
            stock: false,
            params: Vec::new(),
            inputs: Vec::new(),
            outputs: Vec::new(),
            lights: Vec::new(),
            texts: Vec::new(),
            lines: Vec::new(),
            screens: Vec::new(),
            last: None,
        }
    }

    /// A panel in the plugin's own look, as its own modules have: its body
    /// in the theme's colours, the module's name over the top 17 pixels,
    /// captions (`label`, `text`) and lines in its letters and colours,
    /// screens (`screen`) as wells of the theme. No picture.
    pub fn stock(width: f32, height: f32) -> Panel {
        Panel { stock: true, ..Panel::new(width, height) }
    }

    fn control(mut self, index: usize, x: f32, y: f32, size: (f32, f32), kind: &'static str) -> Panel {
        let place = Place { index, x: x - size.0 / 2.0, y: y - size.1 / 2.0, w: size.0, h: size.1, kind };
        self.last = Some(("control", [place.x, place.y, place.w, place.h]));
        self.params.push(place);
        self
    }

    /// Knob `index` (the spec's), `size` across (`MEDIUM` is the plugin's
    /// usual), its middle at (x, y).
    pub fn knob(self, index: usize, x: f32, y: f32, size: f32) -> Panel {
        self.control(index, x, y, (size, size), "knob")
    }

    /// A knob in whole steps as a button that shows its position (its
    /// name, where it has names) and steps through them, `width` wide: on
    /// a stock panel.
    pub fn selector(self, index: usize, x: f32, y: f32, width: f32) -> Panel {
        self.control(index, x, y, (width, 12.0), "button")
    }

    /// A caption for the control or jack placed last: under a control, over
    /// a jack, in the plugin's letters.
    pub fn label(mut self, text: &str) -> Panel {
        if let Some((kind, [x, y, w, h])) = self.last {
            let (at, size) = match kind {
                "jack" => (y - 6.0, JACK_NAME),
                _ => (y + h + 6.0, CAPTION),
            };
            self.texts.push((x + w / 2.0, at, text.to_string(), size, true));
        }
        self
    }

    /// A text of its own, `size` high, its middle at (x, y) (`centered`)
    /// or starting there.
    pub fn text(mut self, x: f32, y: f32, text: &str, size: f32, centered: bool) -> Panel {
        self.texts.push((x, y, text.to_string(), size, centered));
        self
    }

    /// A line from (x1, y1) to (x2, y2), in the theme's colour for them.
    pub fn line(mut self, x1: f32, y1: f32, x2: f32, y2: f32) -> Panel {
        self.lines.push([x1, y1, x2, y2]);
        self
    }

    /// A screen: the box (left, top, width, height) where what the
    /// module's `Screen` draws shows, on the theme's well; the pointer
    /// there is the screen's.
    pub fn screen(mut self, left: f32, top: f32, width: f32, height: f32) -> Panel {
        self.screens.push([left, top, width, height]);
        self
    }

    /// A switch (a knob whose positions are whole steps), stepped by a click.
    pub fn switch(self, index: usize, x: f32, y: f32) -> Panel {
        self.control(index, x, y, (14.0, 22.0), "switch")
    }

    /// A button: on while it's held (`momentary` is the knob's own: one
    /// with two positions).
    pub fn button(self, index: usize, x: f32, y: f32) -> Panel {
        self.control(index, x, y, (18.0, 18.0), "button")
    }

    /// A fader, `w` by `h`.
    pub fn slider(self, index: usize, x: f32, y: f32, w: f32, h: f32) -> Panel {
        self.control(index, x, y, (w, h), "slider")
    }

    fn jack(&mut self, x: f32, y: f32) -> Place {
        let size = if self.stock { STOCK_JACK } else { JACK };
        let place = Place { index: 0, x: x - size / 2.0, y: y - size / 2.0, w: size, h: size, kind: "" };
        self.last = Some(("jack", [place.x, place.y, place.w, place.h]));
        place
    }

    pub fn input(mut self, index: usize, x: f32, y: f32) -> Panel {
        let place = Place { index, ..self.jack(x, y) };
        self.inputs.push(place);
        self
    }

    pub fn output(mut self, index: usize, x: f32, y: f32) -> Panel {
        let place = Place { index, ..self.jack(x, y) };
        self.outputs.push(place);
        self
    }

    /// Light `first` (and the next, for each colour past the first),
    /// `size` across, in `colors` (`#rrggbb`).
    pub fn light(mut self, first: usize, x: f32, y: f32, size: f32, colors: &[&str]) -> Panel {
        let colors = colors.iter().map(|c| c.to_string()).collect();
        self.lights.push(LightPlace { first, x: x - size / 2.0, y: y - size / 2.0, w: size, h: size, colors });
        self
    }

    /// As `oroboro_module_panel` gives it.
    pub fn to_json(&self) -> String {
        let n = |x: f32| if x.is_finite() { format!("{x}") } else { "0".into() };
        let places = |list: &[Place], kinds: bool| {
            let items: Vec<String> = list
                .iter()
                .map(|p| {
                    let kind = if kinds { format!(", \"kind\": \"{}\"", p.kind) } else { String::new() };
                    format!("{{\"index\": {}, \"x\": {}, \"y\": {}, \"w\": {}, \"h\": {}{kind}}}", p.index, n(p.x), n(p.y), n(p.w), n(p.h))
                })
                .collect();
            format!("[{}]", items.join(", "))
        };
        let lights: Vec<String> = self
            .lights
            .iter()
            .map(|l| {
                let colors: Vec<String> = l.colors.iter().map(|c| crate::json_text(c)).collect();
                format!(
                    "{{\"first\": {}, \"x\": {}, \"y\": {}, \"w\": {}, \"h\": {}, \"colors\": [{}]}}",
                    l.first,
                    n(l.x),
                    n(l.y),
                    n(l.w),
                    n(l.h),
                    colors.join(", ")
                )
            })
            .collect();
        let texts: Vec<String> = self
            .texts
            .iter()
            .map(|(x, y, text, size, centered)| {
                let anchor = if *centered { ", \"anchor\": \"center\"" } else { "" };
                format!("{{\"x\": {}, \"y\": {}, \"text\": {}, \"size\": {}{anchor}}}", n(*x), n(*y), crate::json_text(text), n(*size))
            })
            .collect();
        let lines: Vec<String> = self
            .lines
            .iter()
            .map(|l| format!("{{\"x1\": {}, \"y1\": {}, \"x2\": {}, \"y2\": {}}}", n(l[0]), n(l[1]), n(l[2]), n(l[3])))
            .collect();
        let screens: Vec<String> = self
            .screens
            .iter()
            .map(|s| format!("{{\"x\": {}, \"y\": {}, \"w\": {}, \"h\": {}}}", n(s[0]), n(s[1]), n(s[2]), n(s[3])))
            .collect();
        let stock = match self.stock {
            true => format!(
                ", \"style\": \"stock\", \"texts\": [{}], \"lines\": [{}], \"screens\": [{}]",
                texts.join(", "),
                lines.join(", "),
                screens.join(", ")
            ),
            false => String::new(),
        };
        format!(
            "{{\"width\": {}, \"height\": {}, \"params\": {}, \"inputs\": {}, \"outputs\": {}, \"lights\": [{}]{stock}}}",
            n(self.width),
            n(self.height),
            places(&self.params, true),
            places(&self.inputs, false),
            places(&self.outputs, false),
            lights.join(", ")
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A drawing is the ABI's list: each entry its kind and length, its
    /// clip, then a fill's paint and outline, a line's paint, width and
    /// points, a text's colour, size, alignment, point and bytes.
    #[test]
    fn a_drawing_is_the_abis_list() {
        let mut d = Drawing::new();
        d.rect(1.0, 2.0, 3.0, 4.0, Color(1.0, 0.0, 0.0, 1.0));
        d.clip(0.0, 0.0, 10.0, 10.0);
        d.line(&[(0.0, 0.0), (5.0, 5.0)], 2.0, Color::rgb(0, 255, 0));
        d.text(3.0, 4.0, "Hi", 8.0, Color(0.0, 0.0, 1.0, 1.0), Align::Center);
        let n = d.numbers();
        // the fill: kind 1, 4 + 19 + 2 + 8 numbers, no clip, a solid paint, the box
        assert_eq!((n[0], n[1], &n[2..6]), (1.0, 33.0, &[1.0, 0.0, 0.0, 0.0][..]));
        assert_eq!((n[6], n[7], n[25], n[26]), (0.0, 1.0, 4.0, 0.0));
        assert_eq!(&n[27..35], &[1.0, 2.0, 4.0, 2.0, 4.0, 6.0, 1.0, 6.0]);
        // the line: clipped, 2 wide, round, two points
        let at = 35;
        assert_eq!((n[at], n[at + 1], &n[at + 2..at + 6]), (2.0, 4.0 + 19.0 + 4.0 + 2.0 + 4.0, &[0.0, 0.0, 10.0, 10.0][..]));
        assert_eq!(&n[at + 25..at + 29], &[2.0, 1.0, 1.0, 10.0]);
        // the text: its bytes last
        let at = at + 2 + 4 + 19 + 4 + 2 + 4;
        assert_eq!((n[at], n[at + 1]), (3.0, 4.0 + 10.0 + 2.0));
        assert_eq!(&n[at + 6..at + 12], &[0.0, 0.0, 1.0, 1.0, 8.0, 18.0]);
        assert_eq!(&n[at + 16..], &[72.0, 105.0]);
    }

    /// A stock panel says its style, its captions (under a control, over a
    /// jack), its lines and its screens.
    #[test]
    fn a_stock_panel_has_captions_and_screens() {
        let json = Panel::stock(255.0, 200.0)
            .screen(8.0, 22.0, 239.0, 100.0)
            .knob(0, 40.0, 150.0, MEDIUM)
            .label("Gain")
            .input(0, 30.0, 185.0)
            .label("In")
            .line(8.0, 170.0, 247.0, 170.0)
            .to_json();
        for part in [
            "\"style\": \"stock\"",
            "\"x\": 40, \"y\": 167, \"text\": \"Gain\", \"size\": 8.5, \"anchor\": \"center\"",
            "\"x\": 30, \"y\": 172.5, \"text\": \"In\", \"size\": 7.5",
            "\"screens\": [{\"x\": 8, \"y\": 22, \"w\": 239, \"h\": 100}]",
            "\"lines\": [{\"x1\": 8, \"y1\": 170, \"x2\": 247, \"y2\": 170}]",
            "\"index\": 0, \"x\": 23.5, \"y\": 178.5, \"w\": 13, \"h\": 13",
        ] {
            assert!(json.contains(part), "{part} in {json}");
        }
    }

    /// A panel says its places as boxes around the middles given.
    #[test]
    fn a_panel_is_json_the_plugin_reads() {
        let json = Panel::new(255.0, 300.0).knob(0, 40.0, 200.0, 30.0).input(0, 20.0, 270.0).light(0, 240.0, 10.0, 6.0, &["#ff0000"]).to_json();
        assert_eq!(
            json,
            "{\"width\": 255, \"height\": 300, \"params\": [{\"index\": 0, \"x\": 25, \"y\": 185, \"w\": 30, \"h\": 30, \"kind\": \"knob\"}], \
             \"inputs\": [{\"index\": 0, \"x\": 8, \"y\": 258, \"w\": 24, \"h\": 24}], \"outputs\": [], \
             \"lights\": [{\"first\": 0, \"x\": 237, \"y\": 7, \"w\": 6, \"h\": 6, \"colors\": [\"#ff0000\"]}]}"
        );
    }
}
