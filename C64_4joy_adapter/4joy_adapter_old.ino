// Protovision 4 player interface / Classical Games adapter (1997)
// Arduino Pro Micro
// https://en.wikipedia.org/wiki/Commodore_64_joystick_adapters
// https://www.protovision.games/hardw/build4player.php?language=en
// http://cloud.cbm8bit.com/penfold42/joytester.zip
// https://arduino.stackexchange.com/questions/8758/arduino-interruption-on-pin-change/8926
// http://www.pighixxx.net/wp-content/uploads/2016/07/pro_micro_pinout_v1_0_red.png
// https://opencircuit.shop/ProductInfo/1000378/ATmega32U4-Datasheet.pdf
// https://cdn.sparkfun.com/datasheets/Dev/Arduino/Boards/Pro_Micro_v13b.pdf

// TXD (INT3,PD3) and RXD (INT2,PD2) to userport L.
// INT2(RXD) is used for rising edge and INT3(TXD) for falling edge.
// Interrupts takes only less than 1us to change output port state after select-signal.

// Joystick port 3
// GND = GND (8)
#define up1 (~PD & _BV(7)) // 6,PD7 = (1)
#define down1 (~PC & _BV(6)) // 5,PC6 = (2)
#define left1 (~PD & _BV(4)) // 4,PD4 = (3)
#define right1 (~PD & _BV(0)) // 3,PD0 = (4)
#define fire1 (~PD & _BV(1)) // 2,PD1 = (6)

// Joystick port 4
// GND = GND (8)
#define up2 (~PF & _BV(7)) // A0,PF7 = (1)
#define down2 (~PF & _BV(6)) // A1,PF6 = (2)
#define left2 (~PF & _BV(5)) // A2,PF5 = (3)
#define right2 (~PF & _BV(4)) // A3,PF4 = (4)
#define fire2 (~PE & _BV(6)) // 7,PE6 = (6)

// Arduino <-> Userport
// VCC = +5V (2)
// GND = GND (A)
// TXD+RXD = Select = PB7 (L)
#define upC 1 // 15,PB1 = PB0 (C) 
#define downC 3 // 14,PB3 = PB1 (D)
#define leftC 2 // 16,PB2 = PB2 (E)
#define rightC 6 // 10,PB6 = PB3 (F)
#define fire1C 4 // 8,PB4 = PB4 (H)
#define fire2C 5 // 9,PB5 = PB5 (J)

// LEDS:
// RX = D17,PB0
// TX = -,PD5

volatile uint8_t output1;
volatile uint8_t output2;
volatile uint16_t last_interrupt;
volatile uint8_t mode = 0; // 0 = 3-joystick mode, 1 = 4-joystick mode
volatile uint8_t last_joystick = 1;

ISR(INT2_vect, ISR_NAKED) { // rising edge, output joystick 3
    asm volatile(
    "    push r0                \n"  // save register r0
    "    lds r0, output1        \n"
    "    out %[pin], r0         \n"
    "    pop r0                 \n"  // restore previous r0
    "    rjmp INT2_vect_part_2  \n"  // go to part 2 for required prologue and epilogue
    :: [pin] "I" (_SFR_IO_ADDR(PORTB)));
}

ISR(INT2_vect_part_2) { mode = 1; last_joystick = 1; last_interrupt = TCNT1; }

ISR(INT3_vect, ISR_NAKED) { // falling edge, output joystick 4
    asm volatile(
    "    push r0                \n"  // save register r0
    "    lds r0, output2        \n"
    "    out %[pin], r0         \n"
    "    pop r0                 \n"  // restore previous r0
    "    rjmp INT3_vect_part_2  \n"  // go to part 2 for required prologue and epilogue
    :: [pin] "I" (_SFR_IO_ADDR(PORTB)));
}

ISR(INT3_vect_part_2) { mode = 1; last_joystick = 2; last_interrupt = TCNT1; }

void setup() {
  pinMode(5, INPUT_PULLUP); // pin5 (PC6) is input
  pinMode(7, INPUT_PULLUP); // pin7 (PE6) is input
  DDRB = 0xff; //all PB-ports are outputs
  DDRF = 0; PORTF = 0xff; // all PF-ports are inputs with pullups
  DDRD = B00100000; PORTD = B11110011; // all PD-ports are inputs (except PD5) with pullups (PD2,PD3 without pullup)
  
  TIMSK0 = 0; // disable timer0 interrupts (Arduino Uno/Pro Micro millis()/micros() update ISR)
  
  EICRA = B10110000;  // INT2 – rising edge on RXD (Bxx11xxxx), INT3 - falling edge on TXD (B10xxxxxx)
  EIMSK = B1100;  // enable INT2 (Bx1xx) and INT3 (B1xxx)

  //Serial.begin(115200);
  //PORTD &= ~_BV(5);

  // We can't use millis() or micros() because Timer0 interrupts are disabled. We use 16-bit Timer1 with 1024 prescaler as "clock".
  TIMSK1 = 0; // disable timer1 interrupts
  TCCR1A = 0;
  TCCR1B = B00000101; // Timer1, normal mode, prescaler 1024. One tick is 64us.
  TCNT1 = 0; // reset Timer1 counter
}

volatile uint8_t joy1, joy2;

void loop() {
  uint8_t PF, PD, PC, PE;
  PF = PINF; PD = PIND; PC = PINC; PE = PINE;
  joy1 = 0xff; joy2 = 0xff; // all signals are inverted (also LED)
  if (up1) bitClear(joy1,upC);
  if (down1) bitClear(joy1,downC);
  if (left1) bitClear(joy1,leftC);
  if (right1) bitClear(joy1,rightC);
  if (up2) bitClear(joy2,upC);
  if (down2) bitClear(joy2,downC);
  if (left2) bitClear(joy2,leftC);
  if (right2) bitClear(joy2,rightC);
  if (fire1) { bitClear(joy1,fire1C); bitClear(joy2,fire1C); }
  if (fire2) { bitClear(joy1,fire2C); bitClear(joy2,fire2C); }
  //mode = 1;
  //if (mode) { bitClear(joy1,0); bitClear(joy2,0); } // RX LED is in PB0
  if (mode) { PORTD &= ~_BV(5); } else { PORTD |= _BV(5); } // TX LED is in PD5
//bitClear(joy1,0); bitClear(joy2,0);
//joy1 = 0; joy2 = 0;
//PORTD |= _BV(5);
  output1 = joy1; output2 = joy2;

  if (mode == 1) {
    uint16_t now = TCNT1; // TCNT1 is special 16-bit register, so it must be copied to variable before it can be used
    if ((now - last_interrupt) > 15625) mode = 0; // if there is no select-signal in 1s, fallback to 3-joystick mode 
  } else {
    if (last_joystick == 1) PORTB = output1;  // 3-joystick mode, update repeatedly and only joystick 3
    if (last_joystick == 2) PORTB = output2;  // 3-joystick mode, update repeatedly and only joystick 4
  }
  //PORTB = output1;  // 3-joystick mode, update repeatedly and only joystick 3
  //Serial.print(joy1, BIN); Serial.print(" "); Serial.println(joy2, BIN); delay(100);
  //Serial.print(mode, BIN); Serial.print(" "); Serial.println(last_interrupt);
  //delayMicroseconds(50);
  delayMicroseconds(10);
}