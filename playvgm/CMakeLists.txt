add_executable(playvgm
    main.cpp
    serial.c
    serial.h)
target_link_libraries(playvgm
    libvgm)
