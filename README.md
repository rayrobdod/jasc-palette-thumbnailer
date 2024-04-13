# JASC Palette Thumbnailer

Generates thumbnails for JASC palette files

![A nautilus window displaying a set of \*.pal files; each file has a thumbnail displaying the colors in each palette](doc/sample.png)

## Building

Build just the binary with `./build.sh build_binary`. This creates an executable at `bin/jasc-pal-thumbnailer`.

Create a `*.deb` package, with the command `./build.sh build_package`.
When this package is installed, nautilus should render \*.pal files with thumbnails generated from this program.
