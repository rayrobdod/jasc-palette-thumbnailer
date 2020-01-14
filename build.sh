#!/bin/bash
OUTDIR=bin
INSTALL_BINARY=/usr/bin/jasc-pal-thumbnailer
INSTALL_MIMETYPE=/usr/share/mime/packages/jasc-pal.xml
INSTALL_THUMBNAILERS=/usr/share/thumbnailers/jasc-pal.thumbnailer

function build_binary() {
	mkdir -p $OUTDIR
	gcc -o $OUTDIR/jasc-pal-thumbnailer src/main/c/jasc-pal-thumbnailer.c -lm
}

function build_thumbnailer() {
	echo "[Thumbnailer Entry]">$OUTDIR/jasc-pal.thumbnailer
	echo "TryExec=$INSTALL_BINARY">>$OUTDIR/jasc-pal.thumbnailer
	echo "Exec=$INSTALL_BINARY -s %s -i %i -o %o">>$OUTDIR/jasc-pal.thumbnailer
	echo "MimeType=application/x-jasc-palette;">>$OUTDIR/jasc-pal.thumbnailer
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
'build') build_binary ; build_thumbnailer ;;
'install') install ;;
*) echo "usage: ./build.sh clean|build|install" ;;
esac
