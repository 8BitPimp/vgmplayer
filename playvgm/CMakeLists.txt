add_executable(playvgm
    main.cpp
    dev_opl_port.cpp
    dev_serial.cpp)
target_link_libraries(playvgm
    libvgm
    libserial)
