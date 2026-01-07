#!/bin/sh
echo "run ARM"
export LD_LIBRARY_PATH=../eSPDI:$LD_LIBRARY_PATH 
sync
./DMPreview_ARM
