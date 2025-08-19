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

// Simple timer-based sleep using CPU cycles but with better efficiency
void sleep(uint8 times) {
    extern volatile int g_user_interrupt;
    
    // Use a more efficient delay calculation
    // Instead of 500,000 iterations per unit, use a smaller, more predictable number
    volatile uint32_t delay_cycles = times * 10000; // Reduced from 500,000
    
    // Use a more efficient loop structure
    for (volatile uint32_t i = 0; i < delay_cycles; i++) {
        // Check for user interrupt more frequently
        if (g_user_interrupt) break;
        
        // Add a small delay to prevent excessive CPU usage
        // This allows other potential tasks to run
        if (i % 1000 == 0) {
            // Small pause every 1000 iterations
            volatile uint32_t pause = 0;
            for (volatile uint32_t j = 0; j < 10; j++) {
                pause = j; // Prevent optimization
            }
        }
    }
}


