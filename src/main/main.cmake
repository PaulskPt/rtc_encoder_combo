cmake_minimum_required(VERSION 3.10...3.17 FATAL_ERROR)

set(OUTPUT_NAME rtc_enc)
project(${OUTPUT_NAME})

add_executable(${OUTPUT_NAME} ${CMAKE_CURRENT_LIST_DIR}/main.cpp)

target_include_directories(${OUTPUT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
set_target_properties(${OUTPUT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

# pull in pico libraries that we need
target_link_libraries(${OUTPUT_NAME}  
pico_stdlib 
pico_explorer 
pico_graphics 
breakout_encoder 
breakout_ioexpander 
ioexpander 
rv3028 
st7789  
hardware_i2c 
hardware_spi
)


# enable usb output, enable uart output
pico_enable_stdio_usb(${OUTPUT_NAME} 1)
pico_enable_stdio_uart(${OUTPUT_NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${OUTPUT_NAME})
