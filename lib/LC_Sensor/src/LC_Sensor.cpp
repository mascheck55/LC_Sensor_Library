// c++ for Arduino UNO NANO MINI
/*
  LC_Sensor uses D6 for compare, A7 is readen as AnalogInput with limited accuracy 0...255
  Arduino Nano 16MHz
  Timer2 and interrupt are used, D6 used,
  A7 analog input (0...255), AIN0(D6)as comparator
  Pull Up for the whole controller disabled

  MIT licensed: machine code InitialStroke
  Made by using:
  Arduino Sketch "Schrankensteuerung Zustandsautomat Schrankenbewegung"
  Beschreibung siehe www.coole-basteleien.de/schranke.html
  MIT (c) Copyright Autor: A. Messmer 2025 for 45lines - marked below

  Substancial changed by A.Mascheck 19.01.2026, 24.03.2026, 01.04.2026
  MIT (c) Copyright A.Mascheck 2026 - see LICENSE file.


*/
#include <Arduino.h>
#include <LC_Sensor.h>

#if defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_UNO)
// PRR already defined
#elif defined(ARDUINO_AVR_ATmega328PB) || defined(__AVR_ATmega328PB__)
#define PRR PRR0
#else
#error "Unsupported board! Add ARDUINO_AVR_* check"
#endif

#define LC_SENSOR_CHANNELS 7 // A0-A5 für LC Sensoren
#define ADC_CHANNEL 7        // A7 für Potentiometer
#define MAX_SENSORS 8        // total including ADC

// define Const_8_channel 8               // for running over 8 pins
#define SIGNAL_HOLD 180                 // time extension of detected occupancy in 1/400th of seconds
volatile uint8_t _nr_sensors = 0;       // Actually used analog inputs from 1 to 7
volatile uint8_t _result[MAX_SENSORS];  // Result vector with values 0... SIGNAL_HOLD
volatile uint8_t _count_m[MAX_SENSORS]; // Result vector previous measurement
volatile uint8_t _trigger = 37;         // Trigger threshold of the number of counted free oscillations
volatile uint8_t _repeat = 5;           // Sequential continious signals to trigger
volatile uint8_t _comp_count;           // Counter incremented by comparator interrupt
volatile uint8_t _channel_count = 0;    // 0...7
volatile boolean _workingAC = false;    // Flag for the Analog Comparator
volatile boolean _workingAD = false;    // Flag for the Analog Digital Converter
void ReadADC7();                        // Read ADC7
void SetAC();                           // Switch Analog Compare
void InitialStroke(uint8_t _channel);   // initial Stroke
void StoreChannel(uint8_t _channel);    // store the result of the counter
void StartTimer2_8x400Hz();             // Timer for 3,2kHz
#define FIVE_TIMES_NOP asm("nop\n nop\n nop\n nop\n nop\n ");
#define TEN_TIMES_NOP FIVE_TIMES_NOP FIVE_TIMES_NOP
#define TWENTY_TIMES_NOP TEN_TIMES_NOP TEN_TIMES_NOP
struct ChannelConfig {
  volatile uint8_t *port;
  volatile uint8_t *ddr;
  uint8_t bitmask;
};
const ChannelConfig channels[] PROGMEM = {
    {&PORTC, &DDRC, (1 << 0)}, // Channel 0 = A0 = PC0
    {&PORTC, &DDRC, (1 << 1)}, // Channel 1 = A1 = PC1
    {&PORTC, &DDRC, (1 << 2)}, // Channel 2 = A2 = PC2
    {&PORTC, &DDRC, (1 << 3)}, // Channel 3 = A3 = PC3
    {&PORTC, &DDRC, (1 << 4)}, // Channel 4 = A4 = PC4
    {&PORTC, &DDRC, (1 << 5)}, // Channel 5 = A5 = PC5
    {&PORTD, &DDRD, (1 << 4)}, // Channel 6 = A6 = PD4 make stroke with PD4
    {&PORTD, &DDRD, (1 << 3)}, // Channel 7 = A7 = PD3 make stroke with PD3
};
//==============================================================================================
//==============================================================================================

LC_Sensor::LC_Sensor(void)
{
}

//==============================================================================================
bool LC_Sensor::end(void)
{
  cli();                // disable Interrupt
  PORTC = 0x00;         // no Pullups
  DDRC = 0x00;          // all Analoge pins as Input
  bitClear(MCUCR, PUD); // clear PUD bit in MCUCR enables the pull-up function for all pins
  bitSet(PRR, PRADC);   // powersaving for ADC
  sei();
  return 1;
}
//==============================================================================================

bool LC_Sensor::begin(uint8_t nr_sensors, uint8_t trigger, uint8_t repeat)
{
  if (nr_sensors < 1 || nr_sensors > LC_SENSOR_CHANNELS)
    return 0;
  else
    _nr_sensors = nr_sensors;
  if (trigger > 20 && trigger < 46)
    _trigger = trigger; // default 37
  if (repeat > 2 && repeat < 7)
    _repeat = repeat;   // default 5
  bitSet(MCUCR, PUD);   // PUD bit in MCUCR disables the pull-up function for all pins
  bitClear(PRR, PRADC); // no powersaving for ADC
  for (int i = 0; i < _nr_sensors; i++)
  { // loop over all sensors
    _count_m[i] = _result[i] = 0;
  }
  StartTimer2_8x400Hz();
  sei(); // enable interrupt
  return 1;
}
//==============================================================================================
uint8_t LC_Sensor::read(uint8_t A_pin)
{
  return _result[A_pin];
}
bool LC_Sensor::activ(uint8_t A_pin)
{
  if (LC_Sensor::read(A_pin) == 0)
    return 0;
  else
    return 1;
}
//==============================================================================================
uint8_t LC_Sensor::pins() { return _nr_sensors; }

//========================================private===============================================
//==============================================================================================

void StartTimer2_8x400Hz()
{
  /***************** setup 3,3kHz timer ****************/
  bitClear(ASSR, AS2);             // ensure system clock, only necessary for timer 2
  TCCR2A = TCCR2B = TIMSK2 = 0x00; // rest registers disables also timer compare interrupt
  bitSet(TCCR2A, WGM21);           // turn on CTC mode
  TCCR2B |= B00000011;             // Prescaler = 32
                                   // Prescaler B0010=>8; B0011=>62; B100=>64; B0101=>128; B0110=>256; B0111=>1024
  OCR2A = 150;                     // 150 = 3,3kHz
                                   // 150 yields 3,3kHz/8 = 413Hz request on sensors
  TCNT2 = 0;                       // initialize timer counter value to 0
  bitSet(TIMSK2, OCIE2A);          // enable timer compare interrupt
                                   // https://deepbluembedded.com/arduino-timer-calculator-code-generator/
                                   // https://wolles-elektronikkiste.de/timer-und-pwm-teil-1
}

//===============================================================================

ISR(TIMER2_COMPA_vect, ISR_NOBLOCK)
{
  /*
   * die Kanäle 0 bis 7 werden nacheinander durchlaufen mit 400Hz
   * nur 0 bis 5 sind für die LC Sensoren das Ergebnis steht in _result
   * Wenn dieser größer 0 ist, dann ist der Sensor aktiv.
   * _channel 7 wird für die schnelle Auslesung des analogen Eingangs AD7 verwendet
   * dabei ergeben sich Werte von 0-255
   *
   * channels 0 to 7 are run through one after the other at 400Hz
   * only 0 to 5 are for the LC sensors, the result is in _result
   * If this is greater than 0, then the sensor is active.
   * _channel 7 is used for fast reading of analog input AD7
   * this results in values from 0-255
   */
  uint8_t _channel;

  _channel = ((_channel_count++) % MAX_SENSORS); // runs from 0 to 7
  if (_workingAC && _channel > 0)
    StoreChannel(_channel - 1); // store previous channel
  if (_channel < _nr_sensors)
    InitialStroke(_channel);
  if (!(_workingAC) && (_channel == ADC_CHANNEL))
    ReadADC7(); // on _channel 7 is an analog value 0...255
}
//===============================================================================
ISR(ANALOG_COMP_vect)
{

  _comp_count++;
}
//===============================================================================
void ReadADC7()
{
  /*
   *  wird nur einmal verwendet um als letzte Messung einen Poti an A7 einzulesen
   *  die Sequenz beinhaltet den Start und Stop, der Messwert ist 8bit
   *
   * is only used once to read a potentiometer at A7 as the last measurement
   * the sequence includes the start and stop, the measured value is 8bit
   */
  cli();
  _workingAD = true;
  bitClear(ADCSRB, ACME);
  ADCSRA = 0x00;         // AnalogDigitalWandler ServiceRegister A zurückgesetzt
  bitSet(ADCSRA, ADEN);  // ADC einschalten
  bitSet(ADCSRA, ADPS2); // Prescaler für ADC
  bitSet(ADCSRA, ADPS0); // auf 32 - geht deutlich schneller
  ADMUX = (ADC_CHANNEL); // zurücksetzen und A7 als Eingang verwenden
  bitSet(ADMUX, REFS0);  // AVcc als Referenz
  bitSet(ADMUX, ADLAR);  // linksbündig nur 256 Schritte für ADC zur Ausgabe auf PWM
  ADCSRB = 0x00;         // ACME Vergleicher Ausgeschaltet und free running mode
  DDRC = 0x00;           // A0...7 als input
  PORTC = 0x00;          // A0...7 auf low
  bitSet(ADCSRA, ADSC);  // Startbefehl
  sei();
  while (!(bit_is_clear(ADCSRA, ADSC)))
  {
  }; // Warten auf Fertigmeldung
  cli();
  bitClear(ADCSRA, ADEN); // StopADC7
  //_result[7] = ADCL;      // drop the two lowest bits
  _result[7] = ADCH; // only read ADCH for 8-bit result
  _workingAD = false;
  sei();
}
//===============================================================================
void SetAC()
{ 
  bitSet(DIDR1, AIN1D);
  bitSet(DIDR1, AIN0D);   // input Puffer disable
  ACSR = 0x00;            // Analog Vergleich ServiceRegister zurückgesetzt
  bitSet(ACSR, ACIS1);    // Interrupt auf fallende Flanke
  bitClear(ACSR, ACIE);   // Vergleicher interrupt disable
  bitSet(ACSR, ACI);      // Interrupt Flag gelöscht
  ADMUX = 0x00;           // Multiplexer auf A0 und AREF Extern
  bitClear(ADCSRA, ADEN); // AnalogDigitalWandler ausgeschaltet
  DDRC = 0x00;            // A0...7 als input
  PORTC = 0x00;           // A0...7 auf low
                          // wird vorbereitet mit ADCSRA &= ~(bit(ADEN));
                          // und ADMUX |= (_channel & 0x06) A0...6
  _workingAC = false;     // software flag ...
                          // Messung startet mit ADCSRB |= bit(ACME);
}

//===============================================================================
void InitialStroke(uint8_t _channel)
{ // MIT licensed by Albert Messmer 2025
  SetAC();
  _workingAC = true;
  ADMUX = _channel & 0x07;
  
  // read ports from table
  volatile uint8_t *port = (volatile uint8_t *)pgm_read_ptr(&channels[_channel].port);
  volatile uint8_t *ddr  = (volatile uint8_t *)pgm_read_ptr(&channels[_channel].ddr);
  uint8_t bitmask = pgm_read_byte(&channels[_channel].bitmask);
  
  //cli() only for time critical part
  cli();
  
  //******** start measuring sequence ********/
  /* initial pull down pulse */
  *port &= ~bitmask; // LOW
  *ddr |= bitmask;   // Output
  TWENTY_TIMES_NOP;     // 1.25 uS pulse duration

  *ddr &= ~bitmask;  // Input
  FIVE_TIMES_NOP;     // 0.313 uS pause

  /* pull up pulse */
  *port |= bitmask;  // HIGH
  *ddr |= bitmask;   // Output

  TWENTY_TIMES_NOP;     // 1.25 uS counter pulse duration

  *ddr &= ~bitmask;  // Input Tristate
  _comp_count = 0;
  
  sei();

  /* end initial stroke pulse */
  ADCSRB |= (1 << ACME);           // AC on
  ACSR |= (1 << ACIE) | (1 << ACI); // Interrupt + Flag löschen
}
//===============================================================================
//===============================================================================
void StoreChannel(uint8_t _channel)
{
  /*
   * der Messwert wird mit dem Trigger Wert verglichen <_repeat> hintereinander
   * kommende Signale werden gezählt
   *
   * the measured value is compared with the trigger value <_repeat> consecutive
   * Signals are counted, each nonconsecutive will reset the counter
   */
  bitClear(ADCSRB, ACME); // AC off
  bitClear(ACSR, ACIE);   // disable interrupt of analog comparator, _result is in "_comp_count"
  _workingAC = false;     // do not count anymore
  if (_result[_channel])
    _result[_channel]--;

  uint8_t count = _count_m[_channel];
  if (_comp_count >= _trigger)
  {
    _count_m[_channel] = 0;
  }
  else if (count < _repeat)
  {
    _count_m[_channel] = count + 1;
    if (count + 1 >= _repeat)
      _result[_channel] = SIGNAL_HOLD;
  }
  /*
  obove is optimesed code for:
  if (_comp_count < _trigger)
    _count_m[_channel]++;
  else
    _count_m[_channel] = 0;
  if (_count_m[_channel] >= _repeat)
  { // solange das Signal anliegt bleibt _result >0 zur Verhinderung von Prellen
    _result[_channel] = SIGNAL_HOLD;
  }
    */
}

//===============================================================================
