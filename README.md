# Experimental Conduit Plugin

Conduit is still experimental. Things are getting better but lots
still doesn't work, and a few things only work on Mac. 

You can download the nightly from here, but your best bet is:

```bash
git clone https://github.com/surge-synthesizer/conduit
cd conduit
git submodule update --init --recursive
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --target conduit_all
```

results in a `Conduit.clap` and `Conduit.vst3` in `build/conduit_products`.

The best way to interact with this project is

1. The `#conduit-dev` channel on surge discord
2. The `#wrappers` channel on clap discord or
3. Github Issues here


## An Important Note about Licensing

The source code in the 'src' directory and associated CMake files in
the root and source directories are licensed under the MIT License, as
described in LICENSE.md.

However, this project serves multiple purposes, including being an 
example of clap-first development and a test-bed and highlight for
the surge synth team libraries. The majority of those libraries,
as well as several of the SDK dependencies (VST3 and JUCE) are 
GPL3 code. As a result *the combined work of building this project,
if distributed, would trigger the GPL3 license terms*.

This is a bit odd, right? If you have GPL3 dependencies and are MIT
using the code in an MIT context is difficult. You could copy the code
but you would have to strip the dependencies. But that's exactly the 
goal here. Since one of the purposes of this project is to highlight
CLAP-first development, we want the way we structure our projects, our builds,
and our plugin to be something that developers can copy and use no matter
what their chosen software license.

So copy this project, strip out our dependencies, inject your own DSP
and UI, but keep the cmake and layout shenanigans, borrow our tricks,
and that's totally fine. You get no GPL3 contagion. But *dont* strip out
the GPL3 dependencies and you will trigger the GPL3 clauses.

If you don't understand this, basically it means "feel free to read the code
in src/ and cmake/ and copy the ideas, but if you find you have to use
the libraries in libs/ be careful".
