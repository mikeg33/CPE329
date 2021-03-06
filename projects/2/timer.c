#include "spi.h"
#include "dac.h"
#include "util.h"
#include "timer.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Initialized to 61 which should be set for 100Hz.
const uint8_t OCR0A_MAX = 61;
// Point index is used to determine which value of a waveform should be used.
int point_index = 0;

// Sets the clk division via bit twiddling TCCR0B.
void SetClkDiv(uint16_t freq_div) {
   // Clear the bottom three bits that dictate the clk division.
   TCCR0B &= ~(0b111);
   switch(freq_div) {
      case 8:
         TCCR0B |= 1 << CS01;
         break;
      case 64:
         TCCR0B |= 1 << CS01 | 1 << CS00;
         break;
      case 256:
         TCCR0B |= 1 << CS02;
         break;
      case 1024:
         TCCR0B |= 1 << CS02;
         break;
      default:
         TCCR0B |= 1 << CS00; // No prescaling.
         break;
   }
}

// Initialize the timer to use CTC mode.
void initTimer0(uint16_t freq_div) {
   SetClkDiv(freq_div);
   TCCR0B |= 1 << FOC0A;
   TCCR0A = 1 << WGM01 | 1 << COM0A0 | 1 << COM0A1; // CTC, Cmp A mode
   OCR0A = OCR0A_MAX;      // Count up to a max of 128
   TIFR0 = 1 << OCF0A;     // clear previous timer overflow
   TIMSK0 = 1 << OCIE0A;   // timer overflow interrupt enabled
}

// ISR to write to the dac.
ISR(TIMER0_COMPA_vect) {
   Transmit_SPI_Master(current_wave_array[point_index++]);

   // Wrap around the point_index value once it reaches the end of an array.
   if (point_index == MAX_POINTS) {
      point_index = 0;
      if (current_wave_array == white_noise) {
         FillWhiteNoise();
      }
   }
}
