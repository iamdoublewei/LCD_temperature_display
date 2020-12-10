/* Requirements:
 * 1) The LCD displays the temperature in oF when button 1 is pressed.
 * 2) The LCD displays the temperature in oC when button 2 is pressed.
 * 3) Create an array of size 10 in FIFO structure to store the temperatures in oC
 *    sampled during past 10 seconds (one per second).
 * 4) When computer sends a command (You can design the command as you like) through
 *    UART to the board, the cpu on the board will calculate the average of the 10 temperatures in oC
collected in the past 10 seconds). The board will then send back the result to your computer.
 */

#include <msp430.h>
#include <math.h>
#include <stdlib.h>

// LED definition (use for testing)
#define RED BIT0  // Red LED
#define GREEN BIT7 // Green LED
// UART definition
#define UART_CLK_SEL 0x0080 // Specifies accurate SMCLK clock for UART
#define BR0_FOR_9600 0x34 // Value required to use 9600 baud
#define BR1_FOR_9600 0x00 // Value required to use 9600 baud
#define CLK_MOD 0x4911 // Microcontroller will "clean-up" clock signal
// LCD definition
#define POS0 LCDM8 // Segment 14,15; position in () => 00(0)
#define POS1 LCDM15 // Segment 28,29; position in () => 0(0)0
#define POS2 LCDM19 // Segment 36,37; position in () => (0)00
// Temperature sensor definition
#define CALADC12_12V_30C *((unsigned int *)0x1A1A) // Temperature Sensor Calibration-30 C
#define CALADC12_12V_85C *((unsigned int *)0x1A1C) // Temperature Sensor Calibration-85 C
#define SIZE 10 // temperature array size
// Button definition
#define BUT1 BIT1 // Button S1 at P1.1
#define BUT2 BIT2 // Button S2 at P1.2
// Timer definition
#define ACLK 0x0100 // Timer_A ACLK source
#define UP 0x0010 // Timer_A UP mode
#define TAIfg 0x0001 // Used to look at Timer A Interrupt Flag
#define TIMESPAN 16000 // serial communication changes the clock rate, define a timespan for 1s

int cnt = 0;
int index = 0; // point to the current insert position
// initailize temperature queue, default 0, in FIFO
float queue[SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int temp;
volatile float tempDegC;
volatile float tempDegF;
const unsigned char lcd_num[10] = {
    0xFC, // 0
    0x60, // 1
    0xDB, // 2
    0xF3, // 3
    0x67, // 4
    0xB7, // 5
    0xBF, // 6
    0xE4, // 7
    0xFF, // 8
    0xF7, // 9
};

//Select Clock Signals
void select_clock_signals(void)
{
    CSCTL0 = 0xA500; // "Password" to access clock calibration registers
    CSCTL1 = 0x0046; // Specifies frequency of main clock
    CSCTL2 = 0x0133; // Assigns additional clock signals
    CSCTL3 = 0x0000; // Use clocks at intended frequency, do not slow them down
}

// Used to Give UART Control of Appropriate Pins
void assign_pins_to_uart(void)
{
    P3SEL1 = 0x00; // 0000 0000
    P3SEL0 = BIT4 | BIT5; // 0001 1000
    //assigns P3.4 to UART Transmit (TXD) and assigns P3.5 to UART Receive (RXD)
}

//Specify UART Baud Rate
void use_9600_baud(void)
{
    UCA1CTLW0 = UCSWRST; // Put UART into SoftWare ReSeT1
    UCA1CTLW0 = UCA1CTLW0 | UART_CLK_SEL; // Specifies clock source for UART
    UCA1BR0 = BR0_FOR_9600; // Specifies bit rate (baud) of 9600
    UCA1BR1 = BR1_FOR_9600; // Specifies bit rate (baud) of 9600
    UCA1MCTLW = CLK_MOD; // "Cleans" clock signal
    UCA1CTLW0 = UCA1CTLW0 & (~UCSWRST); // Takes UART out of SoftWare ReSeT
}

void ini_lcd() {
    PJSEL0 = BIT4 | BIT5; // For LFXT

    // Initialize LCD segments 0 - 21; 26 - 43
    LCDCPCTL0 = 0xFFFF;
    LCDCPCTL1 = 0xFC3F;
    LCDCPCTL2 = 0x0FFF;

    // Configure LFXT 32kHz crystal
    CSCTL0_H = CSKEY >> 8; // Unlock CS registers
    CSCTL4 &= ~LFXTOFF; // Enable LFXT
    do
    {
        CSCTL5 &= ~LFXTOFFG; // Clear LFXT fault flag
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1 & OFIFG); // Test oscillator fault flag
    CSCTL0_H = 0; // Lock CS registers

    // Initialize LCD_C
    // ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
    LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;

    // VLCD generated internally,
    // V2-V4 generated internally, v5 to ground
    // Set VLCD voltage to 2.60v
    // Enable charge pump and select internal reference for it
    LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;

    LCDCCPCTL = LCDCPCLKSYNC; // Clock synchronization enabled
    LCDCMEMCTL = LCDCLRM; // Clear LCD memory
}

void ini_temp()
{
    // Initialize the shared reference module
    // By default, REFMSTR=1 => REFCTL is used to configure the internal reference
    while(REFCTL0 & REFGENBUSY); // If ref generator busy, WAIT
    REFCTL0 |= REFVSEL_0 + REFON; // Enable internal 1.2V reference

    /* Initialize ADC12_A */
    ADC12CTL0 &= ~ADC12ENC; // Disable ADC12 so that you could modify the values in registers
    ADC12CTL0 = ADC12SHT0_8 + ADC12ON; // Set sample time
    ADC12CTL1 = ADC12SHP; // Enable sample timer
    ADC12CTL3 = ADC12TCMAP; // Enable internal temperature sensor
    ADC12MCTL0 = ADC12VRSEL_1 + ADC12INCH_30; // ADC input ch A30 => temp sense
    ADC12IER0 = 0x001; // ADC_IFG upon conv result-ADCMEMO

    while(!(REFCTL0 & REFGENRDY)); // Wait for reference generator
                                   // to settle
    ADC12CTL0 |= ADC12ENC;
}

// Get average temperature for lastest 10 seconds.
void avg()
{
    // Calculate average
    float sum = 0;
    int i;
    for (i=0; i<SIZE; i++)
    {
        sum += queue[i];
    }
    int avg = round(sum / SIZE);

    // Convert to string
    char buffer[3];
    buffer[0] = '0' + avg / 100;
    avg %= 100;
    buffer[1] = '0' + avg / 10;
    avg %= 10;
    buffer[2] = '0' + avg;

    for (i=0; i<sizeof(buffer); i++)
    {
        while (!(UCA1IFG&UCTXIFG)); // USCI_A0 TX buffer ready for new data?
        UCA1TXBUF = buffer[i]; //Send the UART message out pin P3.4 by resign it to the buffer UCA1TXBUF
    }
}

// Display items in temperature queue
void show_queue()
{
    int i;
    while (!(UCA1IFG&UCTXIFG)); // USCI_A0 TX buffer ready for new data?
    UCA1TXBUF = '[';
    for (i=0; i<SIZE; i++)
    {
        // Convert to string
        char buffer[3];
        int t = round(queue[i]);
        buffer[0] = '0' + t / 100;
        t %= 100;
        buffer[1] = '0' + t / 10;
        t %= 10;
        buffer[2] = '0' + t;

        int j;
        for (j=0; j<sizeof(buffer); j++)
        {
            while (!(UCA1IFG&UCTXIFG)); // USCI_A0 TX buffer ready for new data?
            UCA1TXBUF = buffer[j]; //Send the UART message out pin P3.4 by resign it to the buffer UCA1TXBUF
        }

        while (!(UCA1IFG&UCTXIFG)); // USCI_A0 TX buffer ready for new data?
        if (i == SIZE - 1)
        {
            UCA1TXBUF = ']';
        }
        else
        {
            UCA1TXBUF = ',';
        }
    }
}

void main()
{
    WDTCTL = WDTPW | WDTHOLD; // Stop the watchdog timer
    PM5CTL0 &= ~LOCKLPM5; // Enable inputs and outputs

    // Configure GPIO
    P1DIR |= RED; // Direct pin as output
    P1OUT &= ~RED; // Initial red LED off;
    P9DIR |= GREEN; // Direct pin as output
    P9OUT &= ~GREEN; // Initial green LED off;

    // Configure buttons
    P1DIR &= ~(BUT1 | BUT2); // Direct pin as input
    P1REN |=  (BUT1 | BUT2); // Enable built-in resistor
    P1OUT |=  (BUT1 | BUT2); // Set resistor as pull-up
    P1IES |= (BUT1 | BUT2); // Have flag set on High to Low
    P1IE |= (BUT1 | BUT2); // Enable button 1 and button 2 interrupt
    P1IFG &= ~(BUT1 | BUT2); // Clear Pin 1.1, 1.2 flag so no immediate interrupt

    // Configure serial
    select_clock_signals(); // Assigns microcontroller clock signals
    assign_pins_to_uart(); // P4.2 is for TXD, P4.3 is for RXD
    use_9600_baud(); // UART operates at 9600 bits/second
    UCA1IE |= UCRXIE; // Enable UART receive interrupt

    // Configure LCD
    ini_lcd();
    LCDCCTL0 |= LCDON; //Turn LCD on
    POS2 = lcd_num[0];
    POS1 = lcd_num[0];
    POS0 = lcd_num[0];

    // Configure temperature sensor
    ini_temp();

    // Configure timer
    TA0CCR0 = TIMESPAN; // We will count up from 0 to TIMESPAN
    TA0CTL = ACLK | UP; // Use ACLK, for UP mode
    TA0CCTL0 = CCIE; // Enable interrupt for Timer_0

    _BIS_SR(GIE); // Activate interrupts previously enabled

    char const instruc[] = "This program helps you to get temperature data from MSP430FR6989 board.\n\r"
                           "Please enter the number to do the following: \n\r"
                           "1. Display temperature in queue.\n\r"
                           "2. Get average temperature for lastest 10 seconds.\n\r";
    int i;
    for (i=0; i<sizeof(instruc); i++)
    {
        while (!(UCA1IFG&UCTXIFG)); // USCI_A0 TX buffer ready for new data?
        UCA1TXBUF = instruc[i]; //Send the UART message out pin P3.4 by resign it to the buffer UCA1TXBUF
    }

    while(1)
    {
        ADC12CTL0 |= ADC12SC; // Sampling and conversion start
        __no_operation();
        // Temperature in Celsius. See the Device Descriptor Table section in the
        // System Resets, Interrupts, and Operating Modes, System Control Module
        // chapter in the device user's guide for background information on the
        // used formula.
        tempDegC = (float)(((long)temp - CALADC12_12V_30C) * (85 - 30)) / (CALADC12_12V_85C - CALADC12_12V_30C) + 30.0f;
        // Temperature in Fahrenheit Tf = (9/5)*Tc + 32
        tempDegF = tempDegC * 9.0f / 5.0f + 32.0f;
        __no_operation(); // SET BREAKPOINT HERE
    }
}

#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    if(UCA1RXBUF == '1')
    {
        show_queue();
    }
    else if(UCA1RXBUF == '2')
    {
        avg();
    }

    UCA0IFG = UCA0IFG & (~UCRXIFG); // Reset the UART receive flag
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
    if(ADC12IV & ADC12IV_ADC12IFG0)
    {
        // Vector 12: ADC12MEM0 Interrupt
        temp = ADC12MEM0; // Move results, IFG is cleared
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Port1_ISR (void)
{
    unsigned int t; // initialize temperature

    if (P1IFG & BUT1) // button 1 display F
    {
        t = round(tempDegF);
    }
    if (P1IFG & BUT2) // button 2 display C
    {
        t = round(tempDegC);
    }

    LCDCMEMCTL = LCDCLRM; // Clear LCD memory
    POS2 = lcd_num[t / 100];
    t %= 100;
    POS1 = lcd_num[t / 10];
    t %= 10;
    POS0 = lcd_num[t];

    P1IFG &= ~(BUT1 | BUT2); //Clear Pin 1.1, 1.2 flag
}

// Timer0 Interrupt Service Routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_ISR (void)
{
    P9OUT ^= GREEN; // Toggle green LED to indicate adding temp in queue

    queue[index] = tempDegC;
    index = (index + 1) % 10;
}
