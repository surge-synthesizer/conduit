# Experimental Conduit Plugin

This is experimental code. Right now all the DSP and processing is 
bad. So far this exists to set up the wrapper infrastructure
and to ponder how to do 'clap-first' development.

```bash
git clone https://github.com/surge-synthesizer/conduit
cd conduit
git submodule update --init --recursive
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --target conduit_all
```

results in a `Conduit.clap` and `Conduit.vst3` somewhere
in your build directory. Changes based on platform right now.
You can find em.

My todo list is in the BaconPaulTODO.md but is a mess

right now, hmu in the #wrappers channel on clap discord or
if you really want in #clap-chatter in surge discord

Said DSP and processing are getting incrementally less bad. But this is still 
primarily an infrastructure project. If you're looking for good plugins this 
still ain't quite the place. Feel free to try them but don't
expect things like "all the parameters do something" yet. :)

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
