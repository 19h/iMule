#!/bin/bash

# usage: publish.sh $version $num
# called from release.sh
# adds or update file$num.inc according to $version
#  and to files in current directory

version=$1
der=$2
debversion=$1-1

if [[ "$version" == "" ]] ; then
    exit -1
fi
if [[ "$der" == "" ]] ; then
    exit -2
fi

tbz32file=imule32-nomad-$version.tbz
tbz64file=imule64-nomad-$version.tbz
optzipfile=iMule-$version.zip
srctbzfile=iMule-$version-src.tbz
samfile=sam-$version.zip
portagetbzfile=iMule-$version-portage.tbz

function make_ed2k_hash () { 
	alcc $*
#    bitcollider -p $* | gawk -F= '/tag.filename.filename/ {name=$2;} /tag.file.length/ {size=$2;} /tag.ed2k.ed2khash/ {hash=$2;} END {printf("ed2k://|file|%s|%s|%s|/\n",name,size,hash);}' 
}

echo "iMule v. $version - `date +%Y-%m-%d `" > file$der.inc

if [ -e $samfile ]; then
    echo $samfile >> file$der.inc
    md5sum $samfile > $samfile.md5
    make_ed2k_hash $samfile > $samfile.ed2k
    cut -d' ' -f1 $samfile.md5 >> file$der.inc
    echo "NEEDED I2P LIBRARY<br /> Extract this into &lt;I2P install dir&gt;/libs/ if you run I2P independantly of iMule<br \>" \
         >> file$der.inc
fi

for DIST in xenial trusty jessie wily ; do 
      for ARCH in i386 amd64 ; do
        debfile=imule_$debversion-$DIST-$ARCH.deb
        if [ -e $debfile ]; then
           echo "$debfile" >> file$der.inc
           cut -d' ' -f1 $debfile.md5 >> file$der.inc
           echo "$ARCH $DIST Package<br \>" >> file$der.inc
        fi
      done
done

if [ -e $tbz32file ]; then
    echo "$tbz32file" >> file$der.inc
    cut -d' ' -f1 $tbz32file.md5 >> file$der.inc
    echo "Linux 32 bits Nomad Binaries<br \>" >> file$der.inc
fi
if [ -e $tbz64file ]; then
    echo "$tbz64file" >> file$der.inc
    cut -d' ' -f1 $tbz64file.md5 >> file$der.inc
    echo "Linux 64 bits Nomad Binaries<br \>" >> file$der.inc
fi
if [ -e $optzipfile ]; then
    echo $optzipfile >> file$der.inc
    cut -d' ' -f1 $optzipfile.md5 >> file$der.inc
    echo "Windows Binaries<br \>" >> file$der.inc
fi
if [ -e $srctbzfile ]; then
    echo $srctbzfile >> file$der.inc
    cut -d' ' -f1 $srctbzfile.md5 >> file$der.inc
    echo "Source-Code<br \>" >> file$der.inc
fi
if [ -e $portagetbzfile ]; then
    echo $portagetbzfile >> file$der.inc
    cut -d' ' -f1 $portagetbzfile.md5 >> file$der.inc
    echo "Gentoo ebuild<br \>" >> file$der.inc
fi

cp noteshtml news$der.inc
echo $version > ../lastversion


