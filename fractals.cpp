#include "fractals.h"
#include <math.h>
#include <random>


// Constants
const double scaleY = EPD_7IN3F_HEIGHT / (EPD_7IN3F_WIDTH*2.0L);

inline long double zx(long double n, long double ni, long double c)
{
    return (n*n - ni*ni) + c;
}

inline long double zy(long double n, long double ni, long double ci)
{
    return (2L*ni*n) + ci;
}

inline uint8_t getPixel(uint8_t* image, uint32_t total_rows, uint32_t col)
{
    UBYTE p = image[(total_rows + col)/2];
    return col % 2 == 1 ? p & 0x0F : ((p & 0xF0)>>4);
}

// Perform a low-depth rendering of the mandelbrot fractal
static const unsigned int LOWRES_DEPTH = 50;
inline static void drawLowRes(UBYTE* image, long double invZoom, long double startPosX, long double startPosY, long double offsetX, long double offsetY)
{
    // For each pixel
    for (int row = 0; row < EPD_7IN3F_HEIGHT; row++)
    {
        // Find the relative y position
        long double relativeY = ((long double) row) / EPD_7IN3F_WIDTH;

        // Find the pixel rows
        unsigned int px_rows = EPD_7IN3F_WIDTH * row;

        for (int col = 0; col < EPD_7IN3F_WIDTH; col++)
        {
            // Find the relative x position
            long double relativeX = ((long double) col) / EPD_7IN3F_WIDTH;

            // Calculate each pixel
            long double valx = startPosX;
            long double valy = startPosY;
            long double xPos = (invZoom*(relativeX - 0.5L) + offsetX);
            long double yPos = (invZoom*(relativeY - scaleY) + offsetY);
            int escapeCount = -1;
            for (unsigned int i = 0; i < LOWRES_DEPTH; i++)
            {
                long double nValX = zx(valx, valy, xPos);
                valy = zy(valx, valy, yPos);
                valx = nValX;
                if (sqrt(valx*valx + valy*valy) > 2)
                {
                    escapeCount = i;
                    break;
                }
            }

            // Find the color
            UBYTE color = (escapeCount != -1) ? 0x1 : 0x0;

            // Find the position of the 2-pixel word our pixel is in
            UBYTE* pixels = image + ((px_rows + col)/2);

            // Each byte is two pixels
            if (col % 2 == 1)
            {
                *pixels = (*pixels & 0xF0) + color;
            }
            else
            {
                *pixels = color << 4;
            }
        }
    }
}

// Perform the high-depth rendering of the mandelbrot fractal
static const unsigned int HIRES_DEPTH = 100;
static const unsigned int COLOR_COUNT = 7;
static const unsigned int COLOR_REPETITIONS = 4;
static const long double  COLOR_SKEW = 1.4L;
inline static void drawHiRes(UBYTE* image, long double invZoom, long double startPosX, long double startPosY, long double offsetX, long double offsetY)
{
    // For each pixel
    for (int row = 0; row < EPD_7IN3F_HEIGHT; row++)
    {
        // Find the relative y position
        long double relativeY = ((long double) row) / EPD_7IN3F_WIDTH;

        // Find the pixel rows
        unsigned int px_rows = EPD_7IN3F_WIDTH * row;

        for (int col = 0; col < EPD_7IN3F_WIDTH; col++)
        {
            // Find the relative x position
            long double relativeX = ((long double) col) / EPD_7IN3F_WIDTH;

            // Calculate each pixel
            long double valx = startPosX;
            long double valy = startPosY;
            long double xPos = (invZoom*(relativeX - 0.5L) + offsetX);
            long double yPos = (invZoom*(relativeY - scaleY) + offsetY);
            int escapeCount = HIRES_DEPTH;
            for (unsigned int i = 0; i < HIRES_DEPTH; i++)
            {
                long double nValX = zx(valx, valy, xPos);
                valy = zy(valx, valy, yPos);
                valx = nValX;
                if (sqrt(valx*valx + valy*valy) > 2)
                {
                    escapeCount = i;
                    break;
                }
            }

            // Find the color
            // Color formula = 6 colors repeating over the range of escape counts. Add 1 to avoid re-using black.
            UBYTE color = (escapeCount != HIRES_DEPTH) ? 
                ((uint8_t)((((long double)COLOR_COUNT * COLOR_REPETITIONS) / HIRES_DEPTH) * (pow(HIRES_DEPTH - escapeCount, COLOR_SKEW) / pow(HIRES_DEPTH, COLOR_SKEW-1))) % COLOR_COUNT) + 1
                : 0x0;

            // Find the position of the 2-pixel word our pixel is in
            UBYTE* pixels = image + ((px_rows + col)/2);

            // Each byte is two pixels
            if (col % 2 == 1)
            {
                *pixels = (*pixels & 0xF0) + color;
            }
            else
            {
                *pixels = color << 4;
            }
        }
    }   
}

// Find the black pixels and randomly select one
static std::random_device rd;
static std::mt19937 gen(rd());
#define isBlack(color) color == 0x0
struct pixel
{
    uint16_t x, y;
};
static const uint16_t pixelPositionsMax = EPD_7IN3F_WIDTH*10;
static pixel pixelPositions[pixelPositionsMax];
inline static pixel findInterestingPixel(UBYTE* image)
{
    // Find all black pixel tangent to a non-black pixel, and pick one randomly
    uint16_t pixnum = 0;
    for (uint16_t row = 1; row < EPD_7IN3F_HEIGHT-1; row++)
    {
        // Find the pixel rows
        unsigned int px_rows = EPD_7IN3F_WIDTH * row;

        for (uint16_t col = 1; col < EPD_7IN3F_WIDTH-1; col++)
        {
            UBYTE p = getPixel(image, px_rows, col);

            // Look for black pixels
            if (isBlack(p))
            {
                // Ensure one tangential pixel is not black
                if (!isBlack(getPixel(image, px_rows - EPD_7IN3F_WIDTH, col)) || 
                    !isBlack(getPixel(image, px_rows + EPD_7IN3F_WIDTH, col)) || 
                    !isBlack(getPixel(image, px_rows, col-1)) ||
                    !isBlack(getPixel(image, px_rows, col+1)))
                {
                    pixelPositions[pixnum++] = { col, row };
                }

                // Check if we ran out of room for black pixels - if so, just return
                if (pixnum >= pixelPositionsMax)
                {
                    std::uniform_int_distribution<int> distribution(0, pixnum);
                    int pixind = distribution(gen);
                    return pixelPositions[pixind];
                }
            }
        }
    }
    std::uniform_int_distribution<int> distribution(0, pixnum);
    int pixind = distribution(gen);
    return pixelPositions[pixind];
}

// Draw a mandelbrot fractal on the given image
static const uint8_t ZOOM_COUNT = 3;
void drawFractal(UBYTE* image)
{
    // The inverse of the zoom
    long double invZoom = 2.0L;

    // Position vars
    long double offsetX = 0,     offsetY = 0;
    long double startPosX = 0,   startPosY = 0;

    // Find an 'interesting point' on low-depth images and zoom in a set number of times
    printf("\rFinding an interesting point to zoom...\n");
    printf("Percent Complete: 0%");
    for (uint8_t i = 0; i < ZOOM_COUNT; i++)
    {
        // Zoom in by 2x
        invZoom *= 0.5L;

        // Render the low-depth image
        drawLowRes(image, invZoom, startPosX, startPosY, offsetX, offsetY);

        // Find an interesting pixel
        pixel p = findInterestingPixel(image);
    
        // Set the offset to that pixel
        offsetX += ((p.x / ((long double) EPD_7IN3F_WIDTH))-0.5L) * invZoom;
        offsetY += ((p.y / ((long double) EPD_7IN3F_HEIGHT))-0.5L) * invZoom;

        // Let the user know how close we are...
        printf("\rPercent Complete: %d%", ((i+1)*100)/ZOOM_COUNT);
    }

    // Perform a high-depth render of the set at the interesting point
    printf("\rRendering the image...                     \n");
    printf("Offset: %f, %f\n", offsetX, offsetY);
    drawHiRes(image, invZoom, startPosX, startPosY, offsetX, offsetY);
}