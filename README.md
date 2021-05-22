# Cybr Audio Server

The `cybr` server is a companion to the [`fluid-music` JavaScript library](https://www.npmjs.com/package/fluid-music). When using `cybr` with `fluid-music`, you can:

- Play and render audio from JavaScript
- Use and configure VST plugins from JavaScript

To get the most out of the the Fluid Music system, you will want to run an instance of `cybr` alongside your JavaScript code.

## Development

`cybr` uses git submodules for managing dependencies. You can checkout all the dependencies with the following command:

```
git clone https://github.com/fluid-music/cybr.git
cd cybr
git submodule update --init --recursive
```

To build `cybr`, open `cybr.jucer` using the [Projucer](https://juce.com/discover/projucer) and create an IDE project for your platform. `cybr.jucer` supports the following development environments:

- Windows: Visual Studio
- MacOS: XCode
- Linux: Makefile

Note that by default, `cybr.jucer` expects you to have a version of the Projucer compiled in GPL mode. If you downloaded the Projucer from the JUCE website, it will not be in GPL mode and you may need to update `cybr.jucer` by removing `JUCER_ENABLE_GPL_MODE=1` from the "Preprocessor Definitions" textbox.

### Building for Linux

You have two options when compiling for linux:

1. Use the default Makefile - **Do this if you want to avoid the Projucer**, and you want to accept all the default build options.
2. Use the [Projucer](https://juce.com/discover/projucer) to generate a customized Makefile - Do this if (for example) you want to compile without Jack support or without VST support.

#### 1. Building for Linux - Default Makefile
Make sure that you have the Jack development files installed. Then run `make`.

```sh
sudo apt install libjack-jackd2-dev # Ubuntu/Debian
cd Builds/LinuxMakefile/
make                     # build debug binary
env CONFIG=Release make  # build release binary
```

#### 2. Building for Linux - Custom Builds


First open `cybr.jucer` in [Projucer](https://juce.com/discover/projucer).

If you do not need [JACK](https://jackaudio.org/) support, disable it in the
projucer project via `modules -> juce_audio_devices -> JUCE_JACK`. If you leave
JACK enabled, make sure that you have the JACK development files installed:

```sh
sudo apt install libjack-jackd2-dev # Ubuntu/Debian
```

Save the `cybr.jucer` project with `^+p`. This will generate a makefile in `Builds/LinuxMakefile/Makefile`.

```sh
cd Builds/LinuxMakefile/
make                     # build debug binary
env CONFIG=Release make  # build release binary
```

## CLI

The most common invocation is `cybr -e -f`, which opens an empty document (`-e`) and starts a server listening for `fluid-music` recipes (`-f`). Recipes can then be sent from `fluid-music` client code.

For up-to-date info on all the possible commands, run `cybr -h`.

For detailed information about an option,  (ex `-f`) run `cybr -h -f`

```
cybr --driver=JACK                Select the audio driver
cybr --device=system              Choose input+output device (ex. USB Audio Interface)
cybr --device-out=system          Choose output device (ex. USB Audio Interface)
cybr --listen-interface=127.0.0.1 Choose TCP address to listen on. Default: 127.0.0.1
cybr -p|--listen-port=9999        Choose UDP+TCP port to listen on. Default: 9999
cybr -f|--fluid-server            Launch a server and listen for fluid engine OSC messages
cybr --scan-plugins               Scan for plugins, adding them to the settings file
cybr -a|--autodetect-pm           Add Tracktion Waveform project manager to the settings file
cybr --list-plugins               List available plugins
cybr --list-plugin-params=name    List all the automatable parameters in the given plugin
cybr --list-plugin-presets=name   Print all presets (programs) for a named plugin
cybr --list-projects              List all the projects from the project manager
cybr -i file.tracktionedit        Load the specified .tracktionedit file
cybr -o out.tracktionedit         Save/render the active edit to a .tracktionedit or .wav
cybr --list-clips                 Print a list of the clips in the active Edit
cybr --list-tracks                Print a list of tracks in the active Edit
cybr -e|--empty[=editfile]        Activate an empty edit with optional .tracktionedit file
cybr --print-length               Print the length in seconds of the active edit
cybr --print-config-filename      Print the complete settings filename.
cybr --play                       Play the active edit
cybr --list-io                    Print the engine's MIDI and Wave IO devices
cybr --list-edit-inputs           Print edit inputs (only print available inputs)
cybr --list-state                 Print type of top level chilren in the edit
cybr --target-port=9999           Set OSC destination port
cybr --target-host=127.0.0.1      Set OSC Destination hostname
cybr --ping-osc[=100]             Repeatedly send a test osc message
cybr --jack-test                  check if jack audio is supported
cybr --print-block-size           Print audio block size
cybr --query-param=plugin,param   Query and print data points for a plugin parameter
cybr -v|--version                 Print the cybr version
cybr --preset-path[=./path|!]     Print/Add/Reset preset search path
cybr --sample-path[=./path|!]     Print/Add/Reset sample search path
cybr -h|--help [-i ...]           Print detailed info for subsequent arguments
```
