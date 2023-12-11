#include "fractal_display.h"
#include "e-Paper/EPD_7in3f.h"
#include "fractals.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "hardware/structs/spi.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


bool inline spi_initialize()
{
    stdio_init_all();

    // Give time to connect to serial
    for (int i = 0; i < 10; i++)
    {
        printf("\rWaiting");
        for (int j = 0; j < i; j++)
            printf(".");
        sleep_ms(1000);
    }
    printf("\n");

    // Configure Chip Select
    gpio_init(CS); 
    gpio_set_dir(CS, GPIO_OUT); // Set SPI to output instead of input
    gpio_put(CS, 1); // Set CS high to indicate no current comms


    // Initialize SPI communication with the specified baud
    spi_init(SPI_PORT, 10000000);
    spi_set_format(SPI_PORT,   // SPI instance
                   8,          // Number of bits per transfer
                   SPI_CPOL_0, // Polarity (CPOL)
                   SPI_CPHA_0, // Phase (CPHA)
                   SPI_MSB_FIRST);

    // Initialize GPIO pins for SPI communication
    gpio_set_function(MISO, GPIO_FUNC_SPI);
    gpio_set_function(SCLK, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);

    return true;
}

int main()
{
    // Initialize SPI communications
    printf("Initializing SPI\n");
    if (!spi_initialize()) {
        printf("Failed spi initialization...\n");
        return -1;
    }

    // Initialize the GPIO pins
    printf("Initializing GPIO\r\n");
    if(DEV_Module_Init()!=0){
        printf("Failed GPIO initialization...\n");
        return -1;
    }

    // Initialize the board
    printf("Clearing E-Paper...\r\n");
    EPD_7IN3F_Init();
    EPD_7IN3F_Clear(EPD_7IN3F_WHITE);
    DEV_Delay_ms(1000);
    printf("E-Paper Clear!\r\n");
	
    //Create a new image cache
    UBYTE *image;
    UDOUBLE Imagesize = ((EPD_7IN3F_WIDTH % 2 == 0)? (EPD_7IN3F_WIDTH / 2 ): (EPD_7IN3F_WIDTH / 2 + 1)) * EPD_7IN3F_HEIGHT;
    if((image = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to allocate memory for image...\r\n");
        return -1;
    }

    // Generate the fractal
    printf("Drawing Fractal...\r\n");
    drawFractal(image);
    printf("Done calculating fractal!\r\n");

    // Display the image
    EPD_7IN3F_Display(image);
    printf("Image Drawn!\r\n");
    DEV_Delay_ms(60000);

    // Clear the screen
    printf("Clearing Image...\r\n");
    //EPD_7IN3F_Clear(EPD_7IN3F_WHITE);
    printf("Image Clear!\r\n");

    // Send the screen to sleep
    printf("Going to Sleep\r\n");
    EPD_7IN3F_Sleep();
    free(image);
    image = NULL;
    DEV_Delay_ms(2000); // important, at least 2s

    // Turn off
    DEV_Module_Exit();
    
    return 0;
}
