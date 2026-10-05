# The Rack bridge's licence

Copyright (c) 2026 Oroboro Limited

The files in `rack/include` and `rack/src` of the Oroboro Modular SDK (the
"Bridge": its own implementation of VCV Rack 2's plugin interface, and the
code that connects a Rack module to Oroboro Modular) are licensed under the
GNU General Public License, version 3 or (at your option) any later version
(`LICENSE` in this folder), with the additional permission below.

The rest of the SDK, including the example module in `rack/example`, is
under the MIT licence (the SDK's `LICENSE`).

## Oroboro Rack Bridge Exception, version 1.0

As an additional permission under section 7 of the GNU General Public
License, Oroboro Limited gives you permission to compile the Bridge
together with the source code of a module, and to distribute the resulting
compiled module under terms of your choice, including closed-source and
commercial terms, without the GNU General Public License applying to the
module's own code because the Bridge is compiled into it.

This permission has two limits:

1. **The Bridge stays under the GPL.** If you change the Bridge's files and
   distribute those changes, or a module built with them, make your changed
   Bridge files available under the GPL with this exception.
2. **It covers only the Bridge.** Any other code in a module keeps its own
   licence. A module built from a Rack plugin that is itself under the GPL,
   or that contains GPL code from others, is still subject to that licence.

You may remove this exception from your copy of the Bridge, in which case
the GNU General Public License alone applies to it.

VCV Rack is a trademark of VCV. The Bridge is not VCV's code, and Oroboro
isn't affiliated with or endorsed by VCV.
