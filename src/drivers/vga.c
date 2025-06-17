#include "../../include/tty.h"
#include "../../include/multiboot.h"

void drawPixel(multiboot_info_t *mbi, uint32 count, uint32 color)
{
    string screen = (string) mbi->framebuffer_addr;
    screen[count] = color & 255; // blue
    screen[count+1] = (color >> 8) & 255; // green
    screen[count+2] = (color >> 16) & 255; // red
}

static void fillrect(multiboot_info_t *mbi, unsigned char r, unsigned char g, unsigned char b, unsigned char w, unsigned char h)
{
    string where = (string) mbi->framebuffer_addr;
    int i, j;

    for (i=0; i<w; i++)
{
        for (j=0; j<w; j++)
        {
            where[j*mbi->framebuffer_width] = r;
            where[j*mbi->framebuffer_width + 1] = g;
            where[j*mbi->framebuffer_width + 2] = b;
        }
    where+=mbi->framebuffer_pitch;
    }
}