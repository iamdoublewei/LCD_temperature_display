#include <msp430.h> 


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	PM5CTL0 &= ~LOCKLPM5;
	P1DIR |= BIT0;
	P9DIR |= BIT7;
	P1OUT = ~BIT0;

	int P, i, j, k;
    P = 5;
    int start_time[5] = {0, 3, 6, 11, 17};
    int duration[5] = {5, 3, 6, 1, 4};

    int current = 0;
    for(i=0; i<P; i++)
    {
        if(current>=start_time[i])
        {
            P9OUT = BIT7;
            for(j=0; j<duration[i]; j++)
            {
                __delay_cycles(1000000);
                current++;
                for(k=i; k<P; k++)
                {
                    if(current == start_time[k])
                    {
                        P1OUT = BIT0;
                        __delay_cycles(320000);
                        P1OUT = ~BIT0;
                    }
                }
            }
            P9OUT = ~BIT7;
        }
        else
        {
            int def = start_time[i] - current;
            current = start_time[i];
            for(j=0; j<def; j++)
            {
                __delay_cycles(1000000);
            }

            P9OUT = BIT7;
            for(j=0; j<duration[i]; j++)
            {
                __delay_cycles(1000000);
                current++;
                for(k=i; k<P; k++)
                {
                    if(current == start_time[k])
                    {
                        P1OUT = BIT0;
                        __delay_cycles(320000);
                        P1OUT = ~BIT0;
                    }
                }
            }
            P9OUT = ~BIT7;

        }
        //P1OUT = BIT0;
        __delay_cycles(100000);
        //P1OUT = ~BIT0;
        // keep the light off for half second
    }
	
	return 0;
}
