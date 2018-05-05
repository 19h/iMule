#!/bin/bash

echo
echo '***************** mtn sync -kmkvore-transport@mail.i2p mtn://192.168.14.11:8999?i2p.imule*'  
mtn sync -kmkvore-transport@mail.i2p mtn://192.168.14.11:8999?i2p.imule*

echo
echo '***************** mtn up'
mtn up

echo
echo '*****************'  mtn commit -m \"$1\"
mtn commit -m "$1"

echo
echo '*****************' svn commit -m \"$1\"
svn commit -m "$1"

echo
echo '***************** mtn sync -kmkvore-transport@mail.i2p mtn://192.168.14.11:8999?i2p.imule*'
mtn sync -kmkvore-transport@mail.i2p mtn://192.168.14.11:8999?i2p.imule*

echo
echo '***************** mtn sync -kmkvore-transport@mail.i2p mtn://192.168.14.11:8998?i2p.imule*'
echo not working: mtn sync -kmkvore-transport@mail.i2p mtn://192.168.14.11:8998?i2p.imule*

echo '***************** done'
