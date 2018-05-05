 #!/bin/bash
##
## makeLinks.sh
## 
## Made by antoine
## Login   <antoine@Broutille>
## 
## Started on  Tue May  8 20:06:09 2007 antoine
## Last update Sat May 12 19:57:23 2007 antoine
##

# get i2p
mtn -k mkvore-commit@mail.i2p -d ~/compilation/i2p/monotone/i2p.mtn co -r 0801db0cfcde1b6cec961b490a357168eadf6943 coreI2P


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

packages="addressbook com freenet gnu net org"

#create folders
for dir in $includes ; do
        echo $dir
        find $dir -type d -exec sh -c 'f={} ; mkdir -p ${f#*/src/}' \;
done

# path back from a directory
cat > /tmp/linkBack.sh << EOF
function backpath  {
  f=\$1
  g=\${f%/*}
  echo \$g | sed 's/[^\/]*/../g'
}
function copyLink {
  f=\$1
  g=\$2
  h=\`backpath \$g\`/\$f
  echo \$f \$h
}
function delPrefix {
  f=\$1
  p=\$2
  echo \${f#\$p}
}
cible=\`delPrefix "\$1" "\$2"\`
source=\`backpath \$cible\`/\$1
if [ -e \$cible ] ; then 
  if [ ! -h \$cible ] ; then
    echo cp \$cible \$1
    cp \$cible \$1
  else
    echo rm \$cible
    svn rm \$cible
  fi
else
  pushd coreI2P
  echo mtn rm \${1#*coreI2P/}
  mtn rm \${1#*coreI2P/}
fi
EOF

#mv local files to corei2p
find $includes -type f ! -wholename "*.svn*" -exec bash /tmp/linkBack.sh {} */src/ \;


#find new local files and add them to monotone coreI2P
cat > /tmp/findBack.sh <<EOF
f=\`find \$2 -wholename "*/\$1"\`
if [ x"\$f" == x ] ; then
  echo \$1
  f=\`find \$2 -wholename "*/\${1%/*}" ! -wholename "*/test/*" ! -wholename "*/routerconsole/*"\` 
  echo \$f
  if [ x"\$f" != x ] ; then
    cp \$1 \$f
    pushd \$f
    mtn add \${1##*/}
  fi
fi
EOF
find $packages -type f ! -wholename "*.svn*" -exec bash /tmp/findBack.sh {} coreI2P \;

mtn commit -m "adding and replacing from iMule" --branch=i2p.i2p.iMule


#
#
# and that's all, folks !
# remaining can be used for updating iMule from i2p
#
#




#replace back local files by coreI2P ones
cat > /tmp/putBack.sh <<EOF
cp \$1 \${1#\$2}
EOF
find $includes -type f ! -wholename "*.svn*" -exec bash /tmp/putBack.sh {} */src/ \;

#find new files
svn status | grep -v "^~"


cat > /tmp/rmMtn.sh <<EOF
pushd coreI2P
echo mtn rm \${1#*coreI2P/}
mtn rm \${1#*coreI2P/}
EOF
# remove them from packages
find $packages -name "*.java" -exec grep "javax" {} \; -exec rm {} \; -exec echo removed: {} \;
# remove files with javax from mtn
find $includes -name "*.java" -exec grep "javax" {} \; -exec bash /tmp/rmMtn.sh {} \;


# ajout de ReseedHandler.java
dir=coreI2P/apps/routerconsole/java/src
mkdir -p net/i2p/router/web
mtn mv $dir/net/i2p/router/web/ReseedHandler.java net/i2p/router/web/
mtn mv $dir/net/i2p/router/web/ContextHelper.java net/i2p/router/web/


