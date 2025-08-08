#include <types.h>
#include <stdint.h>
#include <stddef.h>
#include <util.h> // for g_user_interrupt



uint8 inportb (uint16 _port)
{
    	uint8 rv;
    	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    	return rv;
}

void outportb (uint16 _port, uint8 _data)
{
	__asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

uint16 inw(uint16 _port)
{
    uint16 rv;
    __asm__ __volatile__ ("inw %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

void outw(uint16 _port, uint16 _data)
{
    __asm__ __volatile__ ("outw %1, %0" : : "dN" (_port), "a" (_data));
}

void sleep(uint8 times) {
    volatile uint32_t i, j;
    extern volatile int g_user_interrupt;
    // Increase the delay multiplier to make sleep more noticeable
    for (i = 0; i < times * 500000; i++) {
        j = i; // prevent optimization
        if (g_user_interrupt) break;
    }
}


