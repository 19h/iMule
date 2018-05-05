#!/bin/bash

source $HOME/.scripts/pbuilder_utils.sh

echo "****************************************"
echo "****************************************"
echo CHECKLIST
echo default loglevels of I2P router to ERROR
echo SAM_WIRETAP = no !
echo
echo "****************************************"
echo "****************************************"
echo TODO: NEXT RELEASE : nothing yet
echo "****************************************"
echo "****************************************"

if [ v"$1" == v ] ; then
	echo "usage : ./release.sh <version> <first step> [<last step>]"
	exit -1
fi

version=$1
debversion=$version-1

if [ v"$2" == v ] ; then
	echo "usage : ./release.sh <version> <step> [<last step>]"
	exit -2
fi

from=$2

if [ v"$3" == v ] ; then
	step=$from
else
	to=$3
	for i in `seq $from $to` ; do ./release.sh $version $i ; done || exit -3
	exit 0
fi

export SVNROOT=`svn info | grep ^URL | cut -d':' -f 2- | cut -d' ' -f 2-`
export SVNBASE=`svn info | grep ^URL | cut -d':' -f 2- | cut -d' ' -f 2- | sed -e 's,/iMule/.*,/iMule,g'`

noteshtml="$HOME/tmp/notes.html"
notestxt="$HOME/tmp/notes.txt"
publihost="$IMULE_PUBLISH_HOST"
publidir="$IMULE_PUBLISH_DIR"
runninghost="$IMULE_RUNNING_HOST"
runningdir="$IMULE_RUNNING_DIR"
publidirlist=`ssh $publihost ls $publidir`
if [ "`uname -m`" == "x86_64" ] ; then bits="64" ; else bits="32" ; fi


function make_ed2k_hash () { 
    #bitcollider -p $* | gawk -F= '/tag.filename.filename/ {name=$2;} /tag.file.length/ {size=$2;} /tag.ed2k.ed2khash/ {hash=$2;} END {printf("ed2k://|file|%s|%s|%s|/\n",name,size,hash);}' 
    alcc $*
}
function make_gentoo_manifest_hash() {
    siz=`cat "$1" | wc -c`
    sha256=`cat "$1" | sha256sum | gawk '{print $1}'`
    sha512=`cat "$1" | sha512sum | gawk '{print $1}'`
    whirlpool=`cat "$1" | whirlpooldeep`
    echo "$siz SHA256 $sha256 SHA512 $sha512 WHIRLPOOL $whirlpool"
}

touch $noteshtml

echo On fait la release $1 !

# recherche du dernier numéro de fichier publié
der=1
while ( echo $publidirlist | grep file$der.inc > /dev/null ) ; do der=$((der+1)); done
der=$((der-1))
echo $der
lastpublished=$der

# est-ce la même version ?
ssh $publihost grep $version $publidir/file$der.inc || der=$((der+1))
echo $der


#####################################################################
#
# Commentaires
#
#####################################################################

# la compilation windows peut avoir changé ça
sudo update-alternatives --auto wx-config

if (( $step == 1 )) ; then
    if (( $lastpublished != $der )) ; then
	echo "Editer le texte en html, sauver, et quitter !"
	date +%Y-%m-%d > $noteshtml
	echo "<h2>Latest version : $version</h2>" >> $noteshtml
	cat >> $noteshtml << EOF

<ul>
	<li></li>
</ul>


*** log original ***
EOF
	svn log >> $noteshtml
	emacs $noteshtml
	html2text $noteshtml > $notestxt
	emacs $notestxt &
	#firefox "http://www.imule.i2p/trac/wiki/NewNews?action=edit" &
	rm -rf ../svn-tmp
	mkdir -p ../svn-tmp
	svn co "$SVNROOT" "../svn-tmp/imule-$version"
	pushd "../svn-tmp/imule-$version" || exit -1
	dch --newversion=$debversion
	svn commit -m "new version in debian/changelog"
	popd
	svn update
    else
	scp $publihost:$publidir/noteshtml $noteshtml
	lastrev=`svn info $SVNBASE/tags/v$version | grep 'Révision de la dernière modification' \
	    | cut -d' ' -f 6`
	svn log $SVNBASE/tags/v$version -r $lastrev | head --lines=-2 | tail --lines=+4 > $notestxt
    fi
fi


#####################################################################
#
# Modification de configure.ac et autogen
#
#####################################################################


if (( $step == 2 )) ; then
echo modification de configure.ac
sedcommand="s/AC_INIT.*/AC_INIT([iMule], [$version],[mkvore@mail.i2p])/g"
sed -s -i.precedent "$sedcommand" configure.ac
fi

if (( $step == 3 )) ; then
echo autogen.h
WANT_AUTOCONF_2_5="1" WANT_AUTOMAKE_1_6="1" ./autogen.sh
fi

#####################################################################
#
# configure
#
#####################################################################

if (( $step == 4 )) ; then
echo configure linux optimize version for traductions

( srcdir="$PWD" \
  && builddir="$PWD/../build/linopt/v$version" \
  && mkdir -p "$builddir" \
  && pushd "$builddir" \
  && "$srcdir/configure" --disable-irouter       \
                         --enable-unicode       \
                         --enable-maintainer-mode \
  && popd )  || exit -1
fi

#####################################################################
#
# Traductions
#
#####################################################################

if (( $step == 5 )) ; then
echo mise-à-jour des traductions et de version.texi

( srcdir="$PWD" \
  && builddir="$PWD/../build/linopt/v$version" \
  && pushd "$builddir/docs"                      \
  && make all                                  \
  && popd ) || exit -1

( srcdir="$PWD" \
  && builddir="$PWD/../build/linopt/v$version" \
  && pushd "$builddir/po"                      \
  && make update-po                            \
  && cd "$srcdir/po"                           \
  && lokalize fr.po                         \
  && popd ) || exit -1
fi

#####################################################################
#
# Commit SVN
#
#####################################################################

if (( $step == 6 )) ; then

echo getting nodes.dat from "$runninghost":"$runningdir"/nodes.dat
scp "$runninghost":"$runningdir"/nodes.dat docs/ || exit -1

echo commit/update svn

( svn commit -m"release version $version" \
  && svn update                              \
  && ./commits.sh "release version $version" \
) || exit -1
fi


#####################################################################
#
# WINDOWS, version DEBUG
#
#####################################################################

if (( $step == -7 )) ; then
echo construction de la version debug windows
mkdir -p "/media/brun/iMuleWin32/v$version/debug/build"
( pushd "/media/brun/iMuleWin32/v$version/debug/build" \
&& ADDR2LINE="/usr/mingw32/bin/i686-pc-mingw32-addr2line" AR="/usr/mingw32/bin/i686-pc-mingw32-ar" AS="/usr/mingw32/bin/i686-pc-mingw32-as" CC="/usr/mingw32/bin/i686-pc-mingw32-gcc" CPPFLAGS="-I/mingw/local/include -I/mingw/include" CXX="/usr/mingw32/bin/i686-pc-mingw32-g++" GCJ="/usr/mingw32/bin/i686-pc-mingw32-gcj" GCJH="/usr/mingw32/bin/i686-pc-mingw32-gcjh" LD="/usr/mingw32/bin/i686-pc-mingw32-ld" LDFLAGS="-L/mingw/local/lib -L/mingw/lib" PATH="/mingw/local/bin:/mingw/bin:/usr/mingw32/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/bin/X11:/usr/games:/usr/local/tts/mbrola:/usr/local/tts/lia_phon:/usr/local/mplayer/bin" RANLIB="/usr/mingw32/bin/i686-pc-mingw32-ranlib" STRIP="/usr/mingw32/bin/i686-pc-mingw32-strip" WINDRES="/usr/mingw32/bin/i686-pc-mingw32-windres" "/media/brun/progs/iMule/configure" --enable-irouter --enable-debug --enable-unicode --host=i686-pc-mingw32 --prefix=/media/brun/iMuleWin32/v$version/debug/local \
&& PATH="/usr/mingw32/bin:$PATH" WANT_AUTOCONF_2_5="1" WANT_AUTOMAKE_1_6="1" make install \
&& zipfile=iMule-$version-dbg.zip \
&& echo Mise en ligne de la version debug : $zipfile \
&& cd "/media/brun/iMuleWin32/v$version/debug/local" \
&& mkdir -p iMule \
&& cp -r bin/* lib/* share/locale share/applications share/pixmaps /media/brun/iMuleWin32/dlls/* iMule \
&& /usr/mingw32/bin/i686-pc-mingw32-strip iMule/imule.exe \
&& /usr/mingw32/bin/i686-pc-mingw32-strip iMule/ed2k.exe \
&& zip -r $zipfile iMule \
&& md5sum $zipfile > $zipfile.md5 \
&& make_ed2k_hash $zipfile > $zipfile.ed2k \
&& scp $zipfile $zipfile.md5 $zipfile.ed2k $publihost:$publidir/ ) \
|| exit -1
popd
fi


#####################################################################
#
# WINDOWS version OPTIMISEE
#
#####################################################################

# && \
#     PATH=/usr/$WINTARGET/bin:${PATH} $srcdir/configure --disable-irouter --disable-debug --enable-optimize \
#     --host=$WINTARGET --enable-static --prefix="$installdir" --enable-ccache \
#     --with-crypto-prefix=/usr/$WINTARGET --disable-upnp --enable-maintainer-mode \

if (( $step == 7 )) ; then
    WINTARGET=i686-w64-mingw32
    #WINTARGET=i586-mingw32msvc
    #sudo update-alternatives --set wx-config /usr/lib/wx/config/$WINTARGET-msw-unicode-static-3.0
    sudo update-alternatives --set wx-config /usr/$WINTARGET/lib/wx/config/$WINTARGET-msw-unicode-static-3.0
echo configure la version optimisée pour Windows TARGET=$WINTARGET

( srcdir="$PWD" \
  && builddir="$PWD/../build/winopt/v$version" \
  && installdir="$PWD/../install/winopt/iMule-$version" \
  && mkdir -p "$builddir" \
  && pushd "$builddir" \
  && \
      PATH=${PATH} $srcdir/configure --disable-irouter --disable-debug --enable-optimize \
      --host=$WINTARGET --enable-static --prefix="$installdir" --enable-ccache \
      --with-crypto-prefix=/usr/$WINTARGET --disable-upnp --enable-maintainer-mode \
      --without-wxdebug \
      --enable-wxcas --enable-alc --enable-alcc \
      --enable-imule-gui --enable-imule-daemon \
      --with-denoise-level=0 \
  && echo Construction de la version optimisée pour Windows \
  && WANT_AUTOCONF_2_5="1" WANT_AUTOMAKE_1_6="1" make -j9 install-nomad \
  && zipfile=iMule-$version.zip \
  && echo Mise en ligne de la version optimisée : $zipfile \
  && cd "$installdir"/.. \
  && zip -r $zipfile iMule-$version \
  && md5sum $zipfile > $zipfile.md5 \
  && make_ed2k_hash $zipfile > $zipfile.ed2k \
  && scp $zipfile $zipfile.md5 $zipfile.ed2k $publihost:$publidir/ \
  && popd \
) || exit -1

fi

#####################################################################
#
# LINUX version OPTIMISEE
#
#####################################################################

if (( $step == -9 )) ; then
echo configure la version optimisée

( srcdir="$PWD" \
  && builddir="$PWD/../build/linopt/imule-$version" \
  && rm -rf "$builddir" \
  && svn export . "$builddir" \
  && pushd "$builddir" \
  && echo Construction de la version optimisée pour Linux \
  && fakeroot debian/rules binary \
  && cd .. \
  && tbzfile="imule$bits-nomad-$version.tbz" \
  && mv "imule-nomad.tbz" "$tbzfile" \
  && debfiletmp=`\ls imule_$version*.deb | head -1` \
  && debfile=imule$bits-$version.deb \
  && mv $debfiletmp $debfile \
  && echo Mise en ligne de la version optimisée : $tbzfile et $debfile \
  && md5sum $tbzfile > $tbzfile.md5 \
  && make_ed2k_hash $tbzfile > $tbzfile.ed2k \
  && md5sum $debfile > $debfile.md5 \
  && make_ed2k_hash $debfile > $debfile.ed2k \
  && scp $tbzfile $tbzfile.md5 $tbzfile.ed2k $publihost:$publidir/ \
  && scp $debfile $debfile.md5 $debfile.ed2k $publihost:$publidir/ \
  && popd \
) || exit -1
fi

if (( $step == 8 )) ; then
echo construit la version optimisée nomade

( srcdir="$PWD" \
  && builddir="$PWD/../build/linopt/imule-$version" \
  && rm -rf "$builddir" \
  && svn export . "$builddir" \
  && pushd "$builddir" \
  && echo Construction de la version optimisée nomade pour Linux \
  && DEB_BUILD_OPTIONS="parallel=9"  fakeroot debian/rules binary-nomad \
  && cd .. \
  && tbzfile="imule$bits-nomad-$version.tbz" \
  && mv "imule-nomad.tbz" "$tbzfile" \
  && echo Mise en ligne de la version optimisée nomade : $tbzfile \
  && md5sum $tbzfile > $tbzfile.md5 \
  && make_ed2k_hash $tbzfile > $tbzfile.ed2k \
  && scp $tbzfile $tbzfile.md5 $tbzfile.ed2k $publihost:$publidir/ \
  && popd \
) || exit -1
fi

if (( $step == 9 )) ; then
echo création des packages xenial trusty wily jessie

( srcdir="$PWD" \
  && mkpackagedir="$PWD/../mkpackage" \
  && mkdir -p "$mkpackagedir" \
  && rm -rf "$mkpackagedir"/imule* \
  && builddir="$mkpackagedir/imule-$version" \
  && svn export . "$builddir" \
  && pushd "$builddir" \
  && echo Construction des versions optimisées pour Linux \
  && ( echo | dh_make -s --createorig --rulesformat dh7 || echo ) \
  && debuild -eDEB_BUILD_OPTIONS="parallel=9" -S -sa --lintian-opts -i \
  && cd "$mkpackagedir" \
  && dsc=imule_$debversion.dsc \
  && for DIST in xenial trusty jessie wily ; do
      ( update_pbuilder $DIST ) \
      && for ARCH in amd64 i386 ; do
       debfiletmp=/var/cache/pbuilder/$DIST-$ARCH/result/imule_${debversion}_$ARCH.deb \
       && debfile=imule_$debversion-$DIST-$ARCH.deb \
       && if [ ! -e "$debfiletmp" ] ; then \
	   sudo DIST=$DIST   ARCH=$ARCH   pbuilder build --debbuildopts "-j9" $dsc \
            && cp $debfiletmp $debfile \
            && echo Mise en ligne de la version optimisée : $debfile \
            && md5sum $debfile > $debfile.md5 \
            && make_ed2k_hash $debfile > $debfile.ed2k \
            && scp $debfile $debfile.md5 $debfile.ed2k $publihost:$publidir/
          fi
      done
     done \
  && popd \
) || exit -1


fi


#####################################################################
#
# mise à jour SVN TRAC et compression des sources
#
#####################################################################

if (( $step == 10 )) ; then
#echo mise à jour du trac / svn
    distsrcdir="/home/$USER/compilation/imule/perso"
    mkdir -p "$distsrcdir"
    pushd "$distsrcdir" || exit -1
    rm -rf iMule
    svn co $SVNPERSOROOT/trunk
    find trunk/src -type f ! -path "*/.svn*" -exec rm {} \;
    svn export "$SVNROOT" trunk/src --force || exit -1
    cd trunk/src
    (svn status | grep "^\?" | cut -c 2- | xargs svn add) || echo Pas de fichier ajouté
    (svn status | grep "^\!" | cut -c 2- | xargs svn rm) || echo Pas de fichier retiré
    svn commit -F $notestxt || exit -1
    svn update || exit -1
    cd ..
    rm -rf iMule-$version-src
    svn export "$SVNROOT" iMule-$version-src || exit -1
    tar cvfj iMule-$version-src.tbz iMule-$version-src || exit -1
    md5sum iMule-$version-src.tbz > iMule-$version-src.tbz.md5 || exit -1
    make_ed2k_hash iMule-$version-src.tbz > iMule-$version-src.tbz.ed2k || exit -1
    scp iMule-$version-src.tbz iMule-$version-src.tbz.md5 iMule-$version-src.tbz.ed2k $publihost:$publidir/ || exit -1
    cd iMule-$version-src/gentoo/net-p2p/imule
    rm -f Manifest
    touch Manifest
    ls files | while read f ; do
	echo "AUX $f " `make_gentoo_manifest_hash "files/$f"` >> Manifest
    done
    echo "DIST iMule-$version-src.tbz" `make_gentoo_manifest_hash ../../../../iMule-$version-src.tbz` >> Manifest
    f=imule-$version.ebuild
    cp ../../imule.ebuild $f
    echo "EBUILD $f" `make_gentoo_manifest_hash $f` >> Manifest
    echo "MISC metadata.xml" `make_gentoo_manifest_hash metadata.xml` >> Manifest
    cd ../..
    tar cvfj iMule-$version-portage.tbz net-p2p || exit -1
    md5sum iMule-$version-portage.tbz > iMule-$version-portage.tbz.md5 || exit -1
    make_ed2k_hash iMule-$version-portage.tbz > iMule-$version-portage.tbz.ed2k || exit -1
    scp iMule-$version-portage.tbz iMule-$version-portage.tbz.md5 iMule-$version-portage.tbz.ed2k $publihost:$publidir/ || exit -1
    popd    
fi


#####################################################################
#
# Mise en ligne
#
#####################################################################



if (( $step == 11 )) ; then
( scp $noteshtml $publihost:$publidir/noteshtml \
  && scp ./publish.sh $publihost:$publidir/ \
  && echo ssh $publihost \" cd $publidir \&\& ./publish.sh $version $der\" \
  && ssh $publihost " cd $publidir && ./publish.sh $version $der " \
  ) || exit -1
fi


######################################################################
#
#
#  création de tag et branchies dans SVN
#
#
######################################################################

if (( $step == 12 )) ; then
echo svn cp $SVNROOT $SVNBASE/tags/v$version -F $notestxt
svn cp $SVNROOT $SVNBASE/tags/v$version -F $notestxt
svn update || exit -1
fi

######################################################################
#
#
#  propagation dans monotone
#
#
######################################################################

if (( $step == 13 )) ; then
mtn commit --message-file="$notestxt"
mtnrevision=`mtn status | grep Parent | cut -d' ' -f 3`
mtn tag $mtnrevision v$version
#mtn -kmkvore-transport@mail.i2p sync mtn://192.168.1.11:8998?i2p.imule*
mtn -kmkvore-transport@mail.i2p sync mtn://192.168.14.11:8999?i2p.imule*
fi


######################################################################
#
#
#  copie de nodes.dat sur le site web
#
#
######################################################################

if (( $step == 14 )) ; then
echo getting nodes.dat from "$publihost":"$publidir"/../nodes.dat
scp "$publihost":"$publidir"/../nodes.dat ./nodes.dat
scp "$publihost":"$publidir"/../nodes2.dat ./nodes2.dat

echo copying nodes2.dat to "$publihost":"$publidir"/../nodes-$der.dat
scp nodes.dat "$publihost":"$publidir"/../nodes-$der.dat
scp nodes2.dat "$publihost":"$publidir"/../nodes2-$der.dat

echo getting nodes2.dat from "$runninghost":"$runningdir"/nodes.dat
scp "$runninghost":"$runningdir"/nodes.dat ./nodes2.dat

echo copying nodes.dat to "$publihost":"$publidir"/../nodes2.dat
scp nodes2.dat "$publihost":"$publidir"/../nodes2.dat
fi

# la compilation windows peut avoir changé ça
sudo update-alternatives --auto wx-config
