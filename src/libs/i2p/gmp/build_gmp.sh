#!/bin/sh
# When executed in Mingw: Produces an gmp.dll
# When executed in Linux: Produces an libgmp.so


echo BUILD: $BUILD   TARGET: $TARGET

CONFIGUREFLAGS=

case $TARGET in
*mingw*)
	COMPILEFLAGS="-Wall"
	LINKFLAGS="-shared -Wl,--kill-at"
	LIBFILE="gmp.dll"
        ;;
*cygwin*)
	COMPILEFLAGS="-Wall -mno-cygwin"
	LINKFLAGS="-shared -Wl,--kill-at"
	LIBFILE="gmp.dll";;
*darwin*)
        COMPILEFLAGS="-Wall -fPIC"
        LINKFLAGS="-dynamiclib -framework JavaVM"
        LIBFILE="libgmp.jnilib"
        CONFIGUREFLAGS="$CONFIGUREFLAGS --with-pic";;
*)
	COMPILEFLAGS="-fPIC -Wall"
	LINKFLAGS="-shared -Wl,-soname,libgmp.so"
	LIBFILE="libgmp.so"
	CONFIGUREFLAGS="$CONFIGUREFLAGS --with-pic";;
esac
INCLUDES="-Isrc/gmp-4.2.1 -Ibuild"

if [ x"$BUILD" != x ] ; then
    CONFIGUREFLAGS="$CONFIGUREFLAGS --host=$TARGET --build=$BUILD"
fi
echo CONFIGUREFLAGS=$CONFIGUREFLAGS


# renew src dir
echo rm -rf src
rm -rf src
echo mkdir src
mkdir src
echo cd src
cd src
echo tar xvfz ../gmp.tgz
tar xvfz ../gmp.tgz
echo srcdir=src/`ls | xargs`
srcdir=src/`ls | xargs`
echo cd ..
cd ..

# renew build dir
echo rm -rf build
rm -rf build
echo mkdir build
mkdir build

# configure
echo cd build
cd build
echo ../$srcdir/configure $CONFIGUREFLAGS
../$srcdir/configure $CONFIGUREFLAGS

# build gmp
echo "Building gmp lib"
echo $MAKE
CFLAGS=$COMPILEFLAGS $MAKE


# build libgmp....
echo cd ..
cd ..
echo STATICLIBS="build/.libs/libgmp.a"
STATICLIBS="build/.libs/libgmp.a"

#echo "Compiling C code..."
echo rm -f $LIBFILE
rm -f $LIBFILE
#echo $CC -c $COMPILEFLAGS $INCLUDES forlib.c -o forlib.o
#$CC -c $COMPILEFLAGS $INCLUDES "$1/forlib.c" -o forlib.o

echo $CC $COMPILEFLAGS $LINKFLAGS $INCLUDES $INCLUDELIBS forlib.c -o $LIBFILE $STATICLIBS
$CC $COMPILEFLAGS $LINKFLAGS $INCLUDES $INCLUDELIBS forlib.c -o $LIBFILE $STATICLIBS
