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
## This script is called from Makefile and from
## makeMakefile.sh
## and builds java source links from coreI2P
##
## makeMakefile.sh still has to be called when
## source file lists change

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

# path back from a directory
function backpath  {
  f=$1
  g=${f%/*}
  echo $g | sed 's/[^\/]*/../g'
}
function delPrefix {
  f=$1
  p=$2
  echo ${f#$p}
}
function linkBack {
  cible=`delPrefix "$1" "$2"`
  source=`backpath $cible`/$1
  if [ ! -e $cible ] ; then
    mkdir -p ${cible%/*}
    echo ln -s $source $cible
    ln -s $source $cible
  fi
}

# remove old links
for f in `find $packages -type l`; do
    rm $f
done

#link local files to corei2p
for f in `find $includes -type f ! -wholename "*.svn*" ! -name "*~"`; do
  linkBack $f "*/src/"
done
for f in $includeFiles; do
  linkBack $f "*/src/"
done
