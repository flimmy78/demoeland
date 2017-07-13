set remotetimeout 20 
shell start /B D:/MyCloud/Working/NetworkClock/MiCO_SDK/helloworld/MiCoder/OpenOCD/Win32/openocd_mico.exe -s ./ -f ./mico-os/makefiles/OpenOCD/interface/jlink_swd.cfg -f ./mico-os/makefiles/OpenOCD/MX1290/MX1290.cfg -f ./mico-os/makefiles/OpenOCD/MX1290/MX1290_gdb_jtag.cfg -l ./build/openocd_log.txt 
