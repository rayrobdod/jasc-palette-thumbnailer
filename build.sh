#!/bin/bash
OUTDIR=bin
INSTALL_BINARY=/usr/bin/jasc-pal-thumbnailer
INSTALL_DOCS=/usr/share/doc/jasc-pal-thumbnailer/
CC=gcc
CFLAGS=-Wall\ -Werror\ -Wextra
VERSION=2023.03.07

function build_binary() {
	mkdir -p $OUTDIR
	$CC $CFLAGS -o $OUTDIR/jasc-pal-thumbnailer \
		src/main/c/bitstream.c \
		src/main/c/deflate.c \
		src/main/c/jasc-pal-thumbnailer.c \
		-lm
}

function build_package() {
	build_binary

	# copy binary to the output directory
	mkdir -p $OUTDIR/pkg-debian/usr/bin
	cp $OUTDIR/jasc-pal-thumbnailer $OUTDIR/pkg-debian/$INSTALL_BINARY

	# copy readme file to the package's doc.
	mkdir -p $OUTDIR/pkg-debian/$INSTALL_DOCS
	cp README.md $OUTDIR/pkg-debian/$INSTALL_DOCS/README.md

	# copy package unprocessed files ; source directory layout matches target directory layout
	cp -r src/package/identity/* $OUTDIR/pkg-debian/

	# process package .m4 files ; source directory layout matches target directory layout
	pushd src/package/m4 >/dev/null
		files=$(find . -type f -regex '.*\.m4' -print)
	popd >/dev/null
	for file in $files; do
		mkdir -p $OUTDIR/pkg-debian/$(dirname $file)
		m4 \
			-DINSTALL_BINARY=$INSTALL_BINARY \
			-DINSTALLED_SIZE=$(du -s $OUTDIR/pkg-debian/ | awk 'BEGIN {FS="\t"} {print $1}') \
			-DVERSION=$VERSION \
			src/package/m4/$file >$OUTDIR/pkg-debian/$(dirname $file)/$(basename -s .m4 $file)
	done

	# generate package's md5sums file
	pushd $OUTDIR/pkg-debian/ >/dev/null
		find . -type f ! -regex '.*?DEBIAN.*' -print0 | xargs -0 md5sum >DEBIAN/md5sums
	popd >/dev/null

	# create package file from target directory
	dpkg -b $OUTDIR/pkg-debian/ $OUTDIR/jasc-pal-thumbnailer.deb
}

function install() {
	build_package
	dpkg --install $OUTDIR/jasc-pal-thumbnailer.deb
}

case $1 in
'clean') rm -r $OUTDIR ;;
'build_binary') build_binary ;;
'build_package') build_package ;;
'build') build_binary ; build_package ;;
'install') install ;;
*) echo "usage: ./build.sh clean|build_binary|build_package|build|install" ;;
esac
