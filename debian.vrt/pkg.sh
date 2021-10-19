#!/bin/sh
CN=`lsb_release -cs`
cat changelog | sed -r "s/%CN%/~$CN-1/" | sed -r "s/%VRT%/140/"   > 140/changelog
cat changelog | sed -r "s/%CN%/~$CN-1/" | sed -r "s/%VRT%/130/"   > 130/changelog
cat changelog | sed -r "s/%CN%/~$CN-1/" | sed -r "s/%VRT%/71/"    > 71/changelog
cat changelog | sed -r "s/%CN%/~$CN-1/" | sed -r "s/%VRT%/999/"   > trunk/changelog

