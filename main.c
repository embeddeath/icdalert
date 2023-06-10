#include <avr/io.h>
#include <util/delay.h>

 int main()
 {
    volatile int condition; 
     // Set built-in LED pin as output
     //DDRB |= (1 << DDB5);
     while (1) {
        if (condition)
        {
            _delay_ms(500);
        }
         PORTB |=  (1 << PB5);   // LED on
         _delay_ms(500);         // wait 500ms
         PORTB &= ~(1 << PB5);   // LED off
         _delay_ms(500);         // wait 500ms
     }
     return 0;
 }