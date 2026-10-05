// `rack.hpp`: what a VCV Rack 2 module's source includes. This one is the
// Oroboro Modular SDK's: its own code to Rack's plugin interface, so that
// a Rack module's source compiles unchanged into a module for Oroboro
// Modular (`oromod rack build`, docs/rack-modules.md). The parts of a
// module that are its sound (its knobs and jacks, its `process`, the
// `dsp` helpers) work as in Rack; the parts that are Rack's window (its
// panel, its menus, its drawing) are stand-ins that show nothing, since
// the plugin shows a module its own way.
//
// What a module has here that it doesn't in Rack, and the other way:
//  - a cable carries one channel (each voice of a patch has its own
//    instance of the module): `getChannels()` is 1 or 0;
//  - there is never a module beside it (`leftExpander.module` is null);
//  - its lights aren't shown, its menu isn't opened, and what it keeps
//    with a Rack patch (`dataToJson`) isn't kept: it plays as it's made.
#pragma once

#include "rack_core.hpp"
#include "rack_simd.hpp"
#include "rack_dsp.hpp"
#include "rack_engine.hpp"
#include "rack_io.hpp"
#include "rack_ui.hpp"

namespace rack {

// As Rack's own header does: its namespaces' names are `rack`'s too.
using namespace logger;
using namespace math;
using namespace window;
using namespace widget;
using namespace ui;
using namespace app;
using plugin::Model;
using plugin::Plugin;
using namespace engine;
using namespace componentlibrary;

} // namespace rack
