# bsparchive

A tool used to backup the `maps` directory of GoldSrc engine games by
finding all dependencies for each bsp file and zipping them up into individual
archives.

## Usage

Overview of options below:

```
Usage: bsparchive [-hvV] [-g <PATH>] -o <PATH> <PATH>
Identifies and archives all dependencies for bsp files.

  -h, --help                print this help and exit
  -v, --verbose             verbose output
  -V, --version             print version information and exit
  -g, --gamedir=<PATH>      the game directory
  -o, --output=<PATH>       where to output the zip files
  <PATH>                    bsp files or map directories
```

Example usage:

`bsparchive.exe -o output "C:\Games\Steam\steamapps\common\Half-Life\tfc\maps"`

Outputs the archived zip files containing required dependencies for all the bsp files
in the tfc maps folder to the `output` folder in the current directory.

## Limitations

* Only bsp version 30 files are supported. (GoldSrc)
* Excludes all out of the box game files for all games
  * This means when archiving `tfc` maps, all resources from `cstrike` will be
    excluded from the archive even if the `tfc` maps depend on them.

## Build

### Windows

Visual Studio 2017 solution can be found in `ide/vs2017`, this is a plug and
play solution and should build on any Windows machine that can install Visual
Studio 2017.

### Other platforms

To be determined.

## Author

Developed and maintained by [Clinton Bale](https://github.com/clintonbale)

## License

bsparchive is [MIT](https://github.com/clintonbale/bsparchive/blob/master/LICENSE.md)

It re-distributes other open source libraries:

 * [argtable3](http://www.argtable.org/)
   &mdash; BSD &mdash; A single-file, ANSI C, command-line parsing library that parses GNU-style command-line options
 * [miniz](https://github.com/richgel999/miniz)
   &mdash; MIT &mdash; Single C source file zlib-replacement library
 * [tinydir](https://github.com/cxong/tinydir)
   &mdash; Simplified BSD &mdash; Lightweight, portable and easy to integrate C directory and file reader
