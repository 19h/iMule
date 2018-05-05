#!/bin/bash
echo
if [ "${1/*./}" == "cpp" -o "${1/*./}" == "h" ] ; then
    astyle "$1"
    astyle "../../imule.2.0.0/$1"
fi
meld "$1" "../../imule.2.0.0/$1"
