#/bin/sh

echo "Building the jbigi library with GMP"

echo "Extracting GMP..."
tar -xzf gmp-4.2.1.tar.gz
echo "Building..."
mkdir -p lib/
mkdir -p bin/local
cd bin/local
case `uname -sr` in
Darwin*)
# --with-pic is required for static linking
../../gmp-4.2.1/configure --with-pic;;
*)
../../gmp-4.2.1/configure;;
esac
make
sh ../../build_jbigi.sh static
cp *jbigi???* ../../lib/
cd ../..
