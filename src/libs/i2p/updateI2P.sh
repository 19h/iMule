 #!/bin/bash
##
## makeLinks.sh
## 
## Made by mkvore
## Login   <mkvore@mail.i2p>
## 
## Started on  Tue May  8 20:06:09 2007 antoine
## Last update Sat May 12 19:57:23 2007 antoine
##

# update i2p
pushd coreI2P
mtn -k mkvore-commit@mail.i2p propagate i2p.i2p i2p.i2p.iMule
mtn up
popd

# remove empty java files
find coreI2P -name "*.java" -exec sh -c 'if [ x"`wc -l {} | cut -d" " -f1`" = x0 ] ; then mtn rm {}; fi ' \;


includes="coreI2P/core/java/src \
	  coreI2P/apps/ministreaming/java/src \
	  coreI2P/apps/streaming/java/src \
	  coreI2P/router/java/src \
	  coreI2P/apps/i2ptunnel/java/src \
	  coreI2P/apps/addressbook/java/src \
	  coreI2P/apps/sam/java/src"
#         coreI2P/apps/routerconsole/java/src"
#	  coreI2P/apps/susidns/src/java/src \
includeFiles="coreI2P/apps/routerconsole/java/src/net/i2p/router/web/ReseedHandler.java \
              coreI2P/apps/routerconsole/java/src/net/i2p/router/web/ContextHelper.java"

packages="addressbook com freenet gnu net org"

# delete old links
rm `find $packages -type l | grep -v .svn`

# path back from a directory

function backpath  {
  f=$1
  g=${f%/*}
  echo $g | sed 's/[^\/]*/../g'
}
function copyLink {
  f=$1
  g=$2
  h=`backpath $g`/$f
  echo $f $h
}
function delPrefix {
  f=$1
  p=$2
  echo ${f#$p}
}

function linkBack {
    cible=`delPrefix "$1" "$2"`
    source=`backpath $cible`/$1
    if [ -e $cible ] ; then 
	if [ ! -h $cible ] ; then
	    echo NOT A LINK : $cible $1
	    exit -1
	else
	    echo rm $cible
	    rm $cible
	fi
    fi
    mkdir -p ${cible%/*}
    echo ln -s $source $cible
    ln -s $source $cible
}


#link local files to corei2p
for f in `find $includes -type f ! -wholename "*.svn*" ! -name "*~"`; do
    linkBack $f "*/src/"
done
for f in $includeFiles; do
  linkBack $f "*/src/"
done


#find new and deleted files
#find $packages | grep -v .svn > i2p.links.list
#svn diff i2p.links.list | grep "^[\+\-]"

pushd coreI2P
svn status | grep -v "^M"
popd

pushd coreI2P
svn status | grep "^\?" | cut -c8- | xargs svn add
svn status | grep "^\!" | cut -c8- | xargs svn rm

#commit
svn commit -m "propagated from i2p.i2p"
