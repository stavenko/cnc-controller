#include <avr/io.h>
#include <avr/interrupt.h>

#define FREQ 1000000
#define PI_ON PB2
#define DISPLAY_ON PB1
#define ENB PB0
#define ENB_INPUT PC0
#define TEST_OUTPUT PC1

#define SECONDS_TO_WAIT_BEFORE_DISPLAY  1
#define SECONDS_TO_WAIT_BEFORE_PI_AFTER_DISPLAY  4
#define SECONDS_TO_WAIT_BEFORE_UNLOCK_ENB  20

#define TIMER_TICKS 15414 
// #define TIMER_TICKS 65533


uint8_t seconds_passed = 0;
uint8_t enb_lock = 1;
uint8_t enb_changed_after_lock = 0;
uint8_t enb_initial = 0;
uint8_t enb_initial_read = 0;

uint8_t timer_triggerred = 0;
uint16_t counter = 0;

// logic:
// - Set enb to true - gives ~enb signal to motor drivers
// - Setup timer to interrupt once per second
// - 


/*
ISR (TIMER1_OVF_vect) {
  timer_triggerred = 1;
  TCNT1 = 0;// 0xffff - TIMER_TICKS;
}
*/

ISR (TIMER1_COMPA_vect) {

  seconds_passed += 1;
  timer_triggerred = 1;
  

  if (seconds_passed == SECONDS_TO_WAIT_BEFORE_DISPLAY) {
    PORTB |= (1 << DISPLAY_ON);
  }
  if (seconds_passed == SECONDS_TO_WAIT_BEFORE_PI_AFTER_DISPLAY +SECONDS_TO_WAIT_BEFORE_DISPLAY) {
    PORTB |= (1 << PI_ON);
  }
  if (seconds_passed > SECONDS_TO_WAIT_BEFORE_UNLOCK_ENB) {
    enb_lock = 0; // unlock enb
    TCCR1B = 0; // stop timer
  }
}

int main(void) {


  DDRB = (1 << PI_ON) | (1 << DISPLAY_ON) | (1 << ENB);
  DDRC = (1 << TEST_OUTPUT);

  PORTB = 0x00;
  PORTB |= (1 << ENB);

  cli();
  TCNT1 = 0;
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10); // clk/64
  // CTC mode compares times with OCR1A and clears TCNT and calls interrupt
  TIMSK = (1 << OCIE1A);
  OCR1A = TIMER_TICKS;

  sei();


  while (1) {


    if (counter > 2000) {
      counter = 0;
      timer_triggerred = 0;
    }

    if (timer_triggerred) {
      counter += 1; 
      PORTC |= (1 << TEST_OUTPUT);
    } else {
      PORTC &= ~(1 << TEST_OUTPUT);
    }


    if (enb_changed_after_lock) {
      uint8_t enb_input = (PINC & (1 << ENB_INPUT)) >> ENB_INPUT;
      if (enb_input) { // enb_input = 1 => enb_output = 0
        PORTB &= ~(1 << ENB);
      } else {
        PORTB |= (1 << ENB);
      }
    } else {
      if (!enb_lock) {
        if (!enb_initial_read) {
          enb_initial = (PINC & (1 << ENB_INPUT)) >> ENB_INPUT;
          enb_initial_read = 1;
        }
        uint8_t current_enb = (PINC & (1 << ENB_INPUT)) >> ENB_INPUT;
        if (current_enb != enb_initial) {
          enb_changed_after_lock = 1;
        }
      }
    }
  }
}
