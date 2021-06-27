/*******************************************************************************
 * Copyright (c) 2021 Dilshan R Jayakody. [jayakody2000lk@gmail.com]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 ******************************************************************************/

#include <xc.h>

#include "config.h"
#include "main.h"

// Maximum voltage level is set to 16V, (16V * 10K)/(10K + 22K) = 5V
// 160 => 16V x 10 to avoid floating point calculations.
#define MAX_VOLTAGE 160

// Number of samples to normalize the ADC voltage readings.
#define ADC_SAMPLES 10

// Startup delay in 0.5 seconds unit. (3 => 3 x 0.5 = 1.5 seconds.)
#define STARTUP_DELAY   3

void main(void) 
{
    CLRWDT();
    WDTCON = 0x00;
    
    unsigned short retVal, normVal;
    unsigned long chrgVoltage;
    unsigned char normPos, blinkState;
    ChargeMode chargeMode;
    
    // Initialize variables to known state.
    normPos = 0;
    startupDelay = 0;
    isRelayActive = 0;
    blinkState = 0;
    chargeMode = CMODE_NONE;
    
    // Initialize MCU peripherals.
    initSystem();
    
    sendLog("Start\r\n");
    
    // Check line status and confirm AC is online.
    while((PORTA & 0x04) != 0x00)
    {
        // AC line status is offline, may be PSU is in middle of shutdown?
        __delay_ms(5);
    } 
    
    sendLog("AC Online\r\n");
    
    // Initiate TIMER1 with 0.5sec triggers.
    T1CON = 0x3D;
    
    // Wait until startup delay is expired...
    while(isRelayActive == 0)
    {
        __delay_ms(20);
    }
    
    // Startup delay is now expired, shutdown the TIMER1 and continue.
    sendLog("Delay Expired\r\n");    
    T1CON = 0x00;
    PIE1 &= 0xFE;
        
    // Cut-off battery and activate 12V supply from PSU.
    PORTC |= 0x04;
    
    // Enable edge interrupt on RA2 to detect AC power failures.
    INTCON &= 0xFD;
    INTCON |= 0x10; 
    
    sendLog("EXT-INT Enable\r\n");
    
    // Wait until charger get normalize.
    __delay_ms(100);
    
    sendLog("Service Start\r\n");
    
    // Main service loop.
    while(1)
    {                      
        normVal = 0;
        
        if(isRelayActive == 0)
        {
            // Power failure detected, reset the MCU.
            PORTC &= 0xFC; 
                    
            INTCON &= 0x7F; 
            T1CON = 0x00;
            PIE1 = 0x00;
            PIR1 = 0x00;
            
            WDTCON = 0x01;            
            while(1);
        }
        
        for(normPos = 0; normPos < ADC_SAMPLES; normPos++)
        {        
            // Get ADC reading from RA4(AN3) pin.
            retVal = getADC();

            // Convert ADC value to 10 x volts.
            chrgVoltage = ((unsigned long)MAX_VOLTAGE * (unsigned long)retVal)/(unsigned long)1023;      
            normVal += (unsigned short)chrgVoltage;
            
            // If battery is weak or dead, blink charge status LEDs.
            if(chargeMode == CMODE_DEAD)
            {
                if((++blinkState) >= 2)
                {
                    blinkState = 0;
                    PORTC ^= 0x03;
                }
            }
          
            __delay_ms(200);
        }
        
        // Get normalized ADC value.
        retVal = normVal / ADC_SAMPLES;        
        logVoltage(retVal);        
        
        if(retVal > 130)
        {
            // Battery is fully charged and switch on floating (slow) charge mode.
            PORTC &= 0xFE;
            PORTC |= 0x0A;
            chargeMode = CMODE_SLOW;
            sendLog("MODE: Float Charging\r\n");
        }
        else if((chargeMode == CMODE_SLOW) && (retVal >= 125))
        {
            // Battery is almost charged. Keep it in slow charge mode to avoid 
            // fluctuations in charge mode.
            PORTC &= 0xFE;
            PORTC |= 0x0A;
            sendLog("MODE: Float Charging [*]\r\n");
        }        
        else if(retVal < 80)
        {
            // May be the battery is bad? Try to slow charge the battery...            
            PORTC |= 0x08;
            chargeMode = CMODE_DEAD;
            sendLog("MODE: Dead Battery\r\n");
        }
        else
        {
            // Battery charge level is low and switch on fast charging mode.
            PORTC &= 0xF5;
            PORTC |= 0x01;
            chargeMode = CMODE_FAST;
            sendLog("MODE: Fast Charging\r\n");
        }
    }
    
    return;
}

void __interrupt() isrMain(void)
{
    // TIMER1 interrupt handler.
    if(PIR1 & 0x01)
    {                      
        if((++startupDelay) >= STARTUP_DELAY)
        {
            // Startup delay is expired. Disable the TIMER1.
            PIE1 &= 0xFE;
            isRelayActive = 1;
        }
        
        // Reset TIMER1 flags and counter registers.
        TMR1H = 0;
        TMR1L = 0;
        PIR1 &= 0xFE;        
    }
    
    // External interrupt handler.
    if(INTCON & 0x02)
    {
        // Power failure (Raising edge) is detected on AC line status input.
        // Cut-off 12V supply and set battery output.
        PORTC &= 0xFB;
        isRelayActive = 0;
        INTCON &= 0xFD;
    }
}

void initSystem()
{
    // RA2 - AC mains status input.
    // RA4(AN3) - Battery voltage sense (ADC) input. 
    // RA3(MCLR) - Reset (ICP).
    // RC0 - Charge status LED - Fast charge / Dead.
    // RC1 - Charge status LED - Float charge / Dead.
    // RC2 - Mains / Battery selector relay.
    // RC3 - Standby charge control (LM350T - ADJ).    
    
    CMCON0 = 0x07;
    ANSEL = 0x08;
    ADCON0 = 0x8D;
    ADCON1 = 0x00;
    VRCON = 0x00;
    
    // System starts with slow charging mode.
    TRISC = 0x30;
    PORTC = 0x38;
    
    TRISA = 0x14;
    PORTA = 0x00;
    
    // baud rate is set to 1200; Let (4000000 - 9600*64)/(1200*64) = 51
    SPBRG = 51;   
    RCSTA = 0x90;
    TXSTA = 0x22;
        
    // Reset TIMER1 counters.
    TMR1H = 0;
    TMR1L = 0;
    
    // Enable TIMER1 and external interrupts.
    OPTION_REG |= 0x40;
    INTCON = 0xC0;
    PIE1 = 0x01;
    PIR1 = 0x00;
}

unsigned short getADC()
{
    ADCON0 |= 0x02;    
    while(ADCON0 & 0x02);
    return (ADRESL + (ADRESH << 8));
}

void logVoltage(unsigned short voltage)
{
    char voltageOut[5];
    
    // Convert specified voltage into 5 digit string.
    voltageOut[4] = 0x00;
    voltageOut[3] = (voltage % 10) + 48;
    voltageOut[2] = ((voltage % 100)/10) + 48;
    voltageOut[1] = ((voltage % 1000)/100) + 48;
    voltageOut[0] = (voltage / 1000) + 48; 
    
    sendLog(voltageOut);
    sendLog("\r\n");
}

void sendLog(char* logData)
{
    while(*logData != 0)
    {
        // Wait for any pending transmissions.
        while(!TRMT);
        
        // Send next byte to the host terminal.
        TXREG = *logData;        
        logData++;
    }
}