#!/bin/sh
echo "run TI"
export LD_LIBRARY_PATH=../eSPDI:$LD_LIBRARY_PATH 
sync
./DMPreview_TI
