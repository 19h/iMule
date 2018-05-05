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

# update links from coreI2P
./link_coreI2P.sh

# remove empty java files
find . -name "*.java" -exec sh -c 'if [ x"`wc -l {} | cut -d" " -f1`" = x0 ] ; then rm {}; fi ' \;

basemakefile=$PWD/Makefile.base.am
makefile=$PWD/Makefile.extra.am



packages="addressbook com freenet gnu net org"

fi=$PWD/i2pIncludes
fjs=$PWD/i2pJavaSources
fcs=$PWD/i2pCppSources
fh=$PWD/i2pHeaders
fbh=$PWD/i2pBuiltHeaders
fbc=$PWD/i2pBuiltClasses

frjs=$PWD/muleJavaRouterSources
flcs=$PWD/muleCppSources
frcs=$PWD/muleCpproutersources
flh=$PWD/muleHeaders
flbh=$PWD/muleBuiltHeaders

ft=$PWD/tmpsort

rm -f $fi $fjs $fcs $fh $fbh $fbc $frjs $flcs $flh $flbh $ft


for dir in $packages ; do
	find $dir    -name "*.java" -exec grep "javax" {} \; -exec rm {} \; -exec echo "removed: "{} \;
	find $dir    -name "*.java" | sed 's/\.\///g' >> $fjs
	find $dir    -name "*.cpp" | sed 's/\.\///g'  >> $fcs
	find $dir    -name "*.h"  | sed 's/\.\///g'>> $fh
done

echo $PWD
echo >> $fi
ls -1 *.java | grep -v "test.*\.java" > $frjs
ls -1 *.cpp | grep -v -E "test|Launcher|Router|Reseeder|JConversions|Logger|StringWriter"  > $flcs
ls -1 *.h     | grep -v "test.*\.h"   > $flh
ls -1 *.cpp | grep -v -E "test" | grep -E "Launcher|Router|Reseeder|JConversions|Logger|StringWriter" > $frcs

# list built headers
# from I2P
sed 's/\.java/.h/g' $fjs > $fbh
# from imule
ls -1 *.java | sed 's/\.java/.h/g' > $flbh

# sort each list in alphabetical order
for f in $fjs $fh $fcs $fbh $frjs $flh $flcs $flbh; do
	sort $f -o $ft
	cp $ft $f
done


# replace $ by $$

sed 's/\$/$$/g' $fbh > $ft
cp $ft $fbh

sed 's/\$/$$/g' $flbh > $ft
cp $ft $flbh



echo "headers"

echo                                  >> $makefile
echo                                  >> $makefile
echo                                  >> $makefile
echo                                  >> $makefile
echo "mulei2pheaders = \\"            >> $makefile
cat $flh |gawk '{print "\t"$0" \\";}' >> $makefile
echo                                  >> $makefile

echo "corei2pheaders = \\"            >> $makefile
cat $fh |gawk '{print "\t"$0" \\";}'  >> $makefile
echo                                  >> $makefile

echo "sources C"

echo "mulei2psources = \\"                      >> $makefile
	cat $flcs  |gawk '{print "\t"$0" \\";}' >> $makefile
	echo                                    >> $makefile


echo "corei2pCsources = \\"                    >> $makefile
	cat $fcs  |gawk '{print "\t"$0" \\";}' >> $makefile
	cat $frcs |gawk '{print "\t"$0" \\";}' >> $makefile
	echo                                   >> $makefile


echo " sources Java"

echo "javasources = \\"                                               >> $makefile
	cat $fjs |gawk '{print "\t"$0" \\";}'                         >> $makefile
	cat $frjs |gawk '{print "\t"$0" \\";}'                        >> $makefile
	echo                                                          >> $makefile



cat > removeSlashes.py << EOF
import sys
f = open( sys.argv[1], 'r' );
l1 = f.readline()
for l2 in f.readlines() :
________if (l2=="\n" and l1[len(l1)-2]=='\\\\')  :
________________print l1[0:len(l1)-2]
________else :
________________print l1[0:len(l1)-1]
________l1=l2
print l1[0:len(l1)-1]
EOF
sed -e "s/________/\t/g" -i removeSlashes.py

python removeSlashes.py $makefile > $ft
cp $ft $makefile

echo Final stage
cat $basemakefile $makefile > Makefile.am

echo Removing temporary files
rm $makefile $frjs $flcs $frcs $flh $flbh $fjs $fcs $fh $fbh $fi $ft removeSlashes.py
