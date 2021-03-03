# File Search Paths

When running the `cybr` server, some methods like the one below accept a file name among their inputs.

- `/audiotrack/insert/wav` (use `--sample-path` to view/update paths)
- `/plugin/sampler/add` (use `--sample-path` to view/update paths)
- `/plugin/load` (use `--preset-path` to view/update paths)

How do these work, and what paths to they search? Generally, these tend to work like this:

1. If an absolute path is specified, use that.
1. Check if the file is relative to the active edit
1. Search the configurable list of paths for the relevant file type.

Here are the current defaults on mac (these will look different on Windows and Linux). Note that you can change these defaults with the relevant CLI.

- `/audiotrack/insert/wav` and `/plugin/sampler/add`
  1. `~/Library/Application Support/cybr/sample`
- `/plugin/load`
  1. `~/Library/Application Support/cybr/preset`
  1. `~/Library/Application Support/Tracktion/Waveform/Presets`

Note that `/plugin/load` and `/plugin/save` expect to look for `.trkpreset` files. If the filename supplied does not have a `.trkpreset` extension, one will be added automatically. This means that the methods can simply specify a preset name like `'4OSC Clinics Unison WMF'`, and if Tracktion Waveform is installed `cybr` should be able to find them.

Note that `/plugin/save` will save preset files in the *first* directory in the preset search path. Currently, saving a plugin with an absolute pathname is not supported.

Under the hood, these methods use the `CybrSearchPath` class to search a list of directories. These directories can be configured with the `--sample-path=/some/path` and `--preset-path=/some/path` command line arguments. These modify the cybr settings file by adding the supplied path to the start of the search path. Note that OSC method handlers will still check relative to the active edit file when searching for files.

It is worth looking at the OSC method handlers, because these might further customize the behavior.