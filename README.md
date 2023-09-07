# Experimental Conduit Plugin

This is experimental code. Right now all the DSP and processing is 
bad. So far this exists to set up the wrapper infrastructure
and to ponder how to do 'clap-first' development.

```bash
git clone https://github.com/baconpaul/conduit
cd conduit
git submodule udpate --init --recursive
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --target conduit_all
```

results in a `Conduit.clap` and `Conduit.vst3` somewhere
in your build directory. Changes based on platform right now.
You can find em.

My todo list is in the BaconPaulTODO.md but is a mess

right now, hmu in the #wrappers channel on clap discord or
if you really want in #clap-chatter in surge discord

but seriously, if you aren't one of the folks active in
the wrapeprs channel, you will be happier if you come back
in a month, once I push this to the surge-synthesizer repo