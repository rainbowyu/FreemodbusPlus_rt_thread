cmake_minimum_required(VERSION 3.15)
project(FreeModbusPlus C)

set(CMAKE_C_STANDARD 11)

include_directories(modbus/ascii)
include_directories(modbus/include)
include_directories(modbus/rtu)
include_directories(modbus/tcp)
include_directories(port)

add_library(FreeModbusPlus
        modbus/ascii/mbascii.c
        modbus/ascii/mbascii.h
        modbus/functions/mbfunccoils.c
        modbus/functions/mbfuncdiag.c
        modbus/functions/mbfuncdisc.c
        modbus/functions/mbfuncholding.c
        modbus/functions/mbfuncinput.c
        modbus/functions/mbfunclot.c
        modbus/functions/mbfuncother.c
        modbus/functions/mbutils.c
        modbus/include/mb.h
        modbus/include/mbconb.h
        modbus/include/mbconfig.h
        modbus/include/mbdef.h
        modbus/include/mbframe.h
        modbus/include/mbfunc.h
        modbus/include/mbport.h
        modbus/include/mbproto.h
        modbus/include/mbutils.h
        modbus/rtu/mbcrc.c
        modbus/rtu/mbcrc.h
        modbus/rtu/mbrtu.c
        modbus/rtu/mbrtu.h
        modbus/tcp/mbtcp.c
        modbus/tcp/mbtcp.h
        modbus/mb.c
        port/mbport.c
        port/port.c
        port/port.h
        port/portevent.c
        port/portevent.h
        port/portserial.c
        port/portserial.h
        port/porttcp.c
        port/porttcp.h
        port/porttimer.c
        port/porttimer.h
        port/user_mb1_app.c
        port/user_mb1_app.h
        port/user_mb2_app.c
        port/user_mb2_app.h)
