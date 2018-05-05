#!/bin/bash
echo
if [ "${1/*./}" == "cpp" -o "${1/*./}" == "h" ] ; then
    astyle "$1"
    astyle "../../amule.2_3_1/$1"
fi
meld "$1" "../../amule.2_3_1/$1"
