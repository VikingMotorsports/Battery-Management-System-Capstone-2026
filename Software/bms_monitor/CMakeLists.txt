cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bms_firmware)

target_sources(app PRIVATE
    src/main.c
    src/debug/debug.c
    src/uart/uart.c
    src/i2c/i2c.c
    src/protocols/bq796xx_protocol.c
    src/protocols/ina238_protocol.c
    src/monitor/init/init.c
    src/monitor/voltage/voltage.c
    src/monitor/temperature/temperature.c
    src/monitor/faults/faults.c
    src/monitor/balancing/balancing.c
    src/monitor/current/current.c
)

target_include_directories(app PRIVATE
    src
    src/debug
    src/uart
    src/i2c
    src/protocols
    src/monitor/init
    src/monitor/register_maps
    src/monitor/voltage
    src/monitor/temperature
    src/monitor/faults
    src/monitor/balancing
    src/monitor/current
)
