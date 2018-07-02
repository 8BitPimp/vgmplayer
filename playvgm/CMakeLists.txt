find_package(SDL)

add_executable(playvgm
    main.cpp
    dev_opl_port.cpp
    dev_serial.cpp)

target_link_libraries(playvgm
    libvgm
    libserial
    lib_mame_sn76489
    lib_mame_ym2612
    ${SDL_LIBRARY})

include_directories(
    AFTER
    SYSTEM
    ${SDL_INCLUDE_DIR})
