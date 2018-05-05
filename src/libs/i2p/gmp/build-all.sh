#/bin/sh

case `uname -sr` in
MINGW*)
	echo "Building windows .dlls for all architectures";;
Linux*)
	echo "Building linux .sos for all architectures";;
FreeBSD*)
	echo "Building freebsd .sos for all architectures";;
*)
	echo "Unsupported build environment"
	exit;;
esac

echo "Extracting GMP..."
tar -xfz gmp.tgz
mv gmp-* gmp
echo "Building..."
mkdir lib
for x in none pentium pentiummmx pentium2 pentium3 pentium4 k6 k62 k63 athlon
do
	mkdir lib/$x
	cd lib/$x
	../../gmp/configure --host=$x
	make
	sh ../../build_gmp.sh
	case `uname -sr` in
	MINGW*)
		cp gmp.dll $1/gmp-windows-$x.dll;;
	Linux*)
		cp libgmp.so $1/libgmp-linux-$x.so;;
	FreeBSD*)
		cp libgmp.so $1/libgmp-freebsd-$x.so;;
	esac
	cd ..
	cd ..
done
