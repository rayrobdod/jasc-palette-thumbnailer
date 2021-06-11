#!/bin/bash
OUTDIR=bin
INSTALL_BINARY=/usr/bin/jasc-pal-thumbnailer
INSTALL_DOCS=/usr/share/doc/jasc-pal-thumbnailer/
INSTALL_MIMETYPE=/usr/share/mime/packages/jasc-pal.xml
INSTALL_THUMBNAILERS=/usr/share/thumbnailers/jasc-pal.thumbnailer
CC=gcc
CFLAGS=-Wall\ -Werror\ -Wextra

function build_binary() {
	mkdir -p $OUTDIR
	$CC $CFLAGS -o $OUTDIR/jasc-pal-thumbnailer \
		src/main/c/bitstream.c \
		src/main/c/deflate.c \
		src/main/c/jasc-pal-thumbnailer.c \
		-lm
}

function build_thumbnailer() {
	echo "[Thumbnailer Entry]">$OUTDIR/jasc-pal.thumbnailer
	echo "TryExec=$INSTALL_BINARY">>$OUTDIR/jasc-pal.thumbnailer
	echo "Exec=$INSTALL_BINARY -s %s -i %i -o %o">>$OUTDIR/jasc-pal.thumbnailer
	echo "MimeType=application/x-jasc-palette;">>$OUTDIR/jasc-pal.thumbnailer
}

function build_package() {
	build_binary
	build_thumbnailer

	mkdir -p $OUTDIR/pkg-debian/usr/bin
	mkdir -p $OUTDIR/pkg-debian/usr/share/mime/packages
	mkdir -p $OUTDIR/pkg-debian/usr/share/thumbnailers

	cp $OUTDIR/jasc-pal-thumbnailer $OUTDIR/pkg-debian/$INSTALL_BINARY
	cp $OUTDIR/jasc-pal.thumbnailer $OUTDIR/pkg-debian/$INSTALL_THUMBNAILERS
	cp README.md $OUTDIR/pkg-debian/$INSTALL_DOCS/README.md
	cp -r src/package/identity/* $OUTDIR/pkg-debian/
	pushd $OUTDIR/pkg-debian/; find . -type f ! -regex '.*?DEBIAN.*' -printf '%P ' | xargs md5sum >DEBIAN/md5sums; popd

	dpkg -b $OUTDIR/pkg-debian/ $OUTDIR/jasc-pal-thumbnailer.deb
}

function install() {
	build_binary
	build_thumbnailer

	cp $OUTDIR/jasc-pal-thumbnailer $INSTALL_BINARY
	cp src/package/mime-info/jasc-pal.xml $INSTALL_MIMETYPE
	cp $OUTDIR/jasc-pal.thumbnailer $INSTALL_THUMBNAILERS
	update-mime-database /usr/share/mime
}

case $1 in
'clean') rm -r $OUTDIR ;;
'build_binary') build_binary ;;
'build_thumbnailer') build_thumbnailer ;;
'build_package') build_package ;;
'build') build_binary ; build_thumbnailer ;;
'install') install ;;
*) echo "usage: ./build.sh clean|build_binary|build_thumbnailer|build|install" ;;
esac
