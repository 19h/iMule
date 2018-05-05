#!/bin/sh
# When executed in Mingw: Produces an gmp.dll
# When executed in Linux: Produces an libgmp.so

CC="gcc"

case `uname -sr` in
MINGW*)
	JAVA_HOME="c:/software/j2sdk1.4.2_05"
	COMPILEFLAGS="-Wall"
	INCLUDES="-I."
	LINKFLAGS="-shared -Wl,--kill-at"
	LIBFILE="gmp.dll";;
CYGWIN*)
	JAVA_HOME="c:/software/j2sdk1.4.2_05"
	COMPILEFLAGS="-Wall -mno-cygwin"
	INCLUDES="-I."
	LINKFLAGS="-shared -Wl,--kill-at"
	LIBFILE="gmp.dll";;
Darwin*)
        JAVA_HOME="/Library/Java/Home"
        COMPILEFLAGS="-Wall"
        INCLUDES="-I."
        LINKFLAGS="-dynamiclib -framework JavaVM"
        LIBFILE="libgmp.jnilib";;
*)
	COMPILEFLAGS="-fPIC -Wall"
	INCLUDES="-I."
	LINKFLAGS="-shared -Wl,-soname,libgmp.so"
	LIBFILE="libgmp.so";;
esac

#To link dynamically to GMP (use libgmp.so or gmp.lib), uncomment the first line below
#To link statically to GMP, uncomment the second line below
STATICLIBS=".libs/libgmp.a"

$CC $LINKFLAGS $INCLUDES $INCLUDELIBS -o $LIBFILE $STATICLIBS
