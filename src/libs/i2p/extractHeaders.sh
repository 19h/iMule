#!/bin/bash -f

echo "* Extracting classes"
JAR_BASE=$*
echo $JAR_BASE
echo "* Removing class files"
find . -name "*.class" -exec rm {} \;
echo "* Extracting classes"
for f in ${JAR_BASE} ; do 
	unzip -o ${f}
done

echo "* Building C++ headers"
#for f in `find net -name "*.class" | xargs`
for f in `find . -name "*.class" | xargs`
do
    g=${f/\.class/}
    h=${g/\.\//}
    echo gcjh -classpath=. $h
    gcjh -classpath=. $h
done
#find . -type f ! -name "*.am" ! -name "*.sh" ! -name "*.class" ! -name "*.h" ! -name "*.jar" -exec rm {} \;
