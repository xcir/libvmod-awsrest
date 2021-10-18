#!/bin/sh
SCRIPT_DIR=$(cd $(dirname $0); pwd)
cd $SCRIPT_DIR
if [ -d debian.vrt/$1 ]; then
  ln -nfs debian.vrt/$1 debian
else
  echo "Not support VRT=$1"
fi

