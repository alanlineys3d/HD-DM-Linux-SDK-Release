#!/bin/sh
echo "run x86_64"
export LD_LIBRARY_PATH=../eSPDI:$LD_LIBRARY_PATH 
export LD_LIBRARY_PATH=../eSPDI/self_calibration/Self_Calibration_API/x86_64/lib/:$LD_LIBRARY_PATH
sync
./DMPreview_X86