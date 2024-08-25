# Set the project name
project(bacnet_rpi)

# Set the C standard
set(CMAKE_C_STANDARD 11)

# Add source files
set(SRC_FILES
    ../src/abort.c
    ../src/address.c
    ../src/ai.c
    ../src/ao.c
    ../src/apdu.c
    ../src/av.c
    ../src/bacaddr.c
    ../src/bacapp.c
    ../src/bacdcode.c
    ../src/bacdevobjpropref.c
    ../src/bacerror.c
    ../src/bacfile.c
    ../src/bacint.c
    ../src/bacprop.c
    ../src/bacpropstates.c
    ../src/bacreal.c
    ../src/bacstr.c
    ../src/bactimevalue.c
    ../src/bi.c
    ../src/bo.c
    ../src/bv.c
    ../src/bvlc.c
    ../src/calendar.c
    ../src/cov.c
    ../src/crc.c
    ../src/datalink.c
    ../src/datetime.c
    ../src/dcc.c
    ../src/device.c
    ../src/dlmstp.c
    ../src/fifo.c
    ../src/h_iam.c
    ../src/h_npdu.c
    ../src/h_rp.c
    ../src/h_rpm.c
    ../src/h_whohas.c
    ../src/h_whois.c
    ../src/h_wp.c
    ../src/h_wpm.c
    ../src/iam.c
    ../src/ihave.c
    ../src/indtext.c
    ../src/memcopy.c
    ../src/ms-input.c
    ../src/msv.c
    ../src/noserv.c
    ../src/npdu.c
    ../src/proplist.c
    ../src/proprietary.c
    ../src/reject.c
    ../src/ringbuf.c
    ../src/rp.c
    ../src/rpm.c
    ../src/s_dcc.c
    ../src/s_iam.c
    ../src/s_ihave.c
    ../src/s_readrange.c
    ../src/s_rp.c
    ../src/s_ts.c
    ../src/s_upt.c
    ../src/s_whois.c
    ../src/s_wp.c
    ../src/schedule.c
    ../src/timer.c
    ../src/timesync.c
    ../src/trendlog.c
    ../src/tsm.c
    ../src/txbuf.c
    ../src/whohas.c
    ../src/whois.c
    ../src/wp.c
    ../src/wpm.c
    ../private/decode.c
    ../private/bac_control.c
    ../private/ptransfer.c
    ../private/rs485.c
    ../private/user_data.c
    ../private/bac_interface.c
    ../private/bip.c
    ../src/h_rr.c
    ../src/h_rr_a.c
    ../src/readrange.c
)

# Add include directories
include_directories(include)

# Create the executable
add_executable(${PROJECT_NAME} ${SRC_FILES})
