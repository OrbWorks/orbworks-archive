setlocal
set SRC=D:\Development\PocketC\PocketC-Palm
copy %SRC%\pcNA* .
copy %SRC%\pcNH* .
par a %1 pcNA0000.bin pcNA0001.bin pcNH0000.txt
copy %SRC%\PocketC.prc D:\Development\PocketC\device_rt
copy %SRC%\PocketC.prc D:\Development\PocketC\PDE\Debug\device_rt
endlocal
rem pause