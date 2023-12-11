add_executable(eink
        fractal_display.cpp
        Config/DEV_config.cpp
        e-Paper/EPD_7in3f.cpp
        GUI/GUI_Paint.cpp
        Fonts/font12.c
        Fonts/font12CN.c
        Fonts/font16.c
        Fonts/font20.c
        Fonts/font24.c
        Fonts/font24CN.c
        ImageData.cpp
        fractals.cpp
        )

# pull in common dependencies
target_link_libraries(eink 
    pico_stdlib
    hardware_spi
)

# create map/bin/hex file etc.
pico_add_extra_outputs(eink)

# add url via pico_set_program_url
pico_enable_stdio_usb(eink 1)
