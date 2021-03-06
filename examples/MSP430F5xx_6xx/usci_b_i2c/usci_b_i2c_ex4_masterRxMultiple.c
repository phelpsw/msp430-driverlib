/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#include "driverlib.h"

//*****************************************************************************
//! This example shows how to configure the I2C module as a master for
//! multi-byte reception with help of interrupts. The address of the slave module
//! that the master communicating with also set in this example. This example uses
//! the interrupt driven mode to receive data.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - I2C peripheral
//! - GPIO Port peripheral (for I2C pins)
//! - SCL2
//! - SDA
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - USCI_B0_VECTOR
//!
//
//*****************************************************************************

//*****************************************************************************
//
//Set the address for slave module. This is a 7-bit address sent in the
//following format:
//[A6:A5:A4:A3:A2:A1:A0:RS]
//
//A zero in the "RS" position of the first byte means that the master
//transmits (sends) data to the selected slave, and a one in this position
//means that the master receives data from the slave.
//
//*****************************************************************************
#define SLAVE_ADDRESS 0x48
//*****************************************************************************
//
//Specify Expected Receive data count.
//
//*****************************************************************************
#define RXCOUNT 0x05

unsigned char receiveBuffer[10] = { 0x01, 0x01, 0x01, 0x01, 0x01,
                                    0x01, 0x01, 0x01, 0x01, 0x01};
unsigned char *receiveBufferPointer;
unsigned char receiveCount = 0;

void main ()
{
    //Stop WDT
    WDT_A_hold(WDT_A_BASE);

    //Assign I2C pins to USCI_B0
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P3,
        GPIO_PIN1 + GPIO_PIN2
        );

    //Initialize Master
    USCI_B_I2C_initMasterParam param = {0};
    param.selectClockSource = USCI_B_I2C_CLOCKSOURCE_SMCLK;
    param.i2cClk = UCS_getSMCLK();
    param.dataRate = USCI_B_I2C_SET_DATA_RATE_100KBPS;
    USCI_B_I2C_initMaster(USCI_B0_BASE, &param);

    //Specify slave address
    USCI_B_I2C_setSlaveAddress(USCI_B0_BASE,
        SLAVE_ADDRESS
        );

    //Set receive mode
    USCI_B_I2C_setMode(USCI_B0_BASE,
        USCI_B_I2C_RECEIVE_MODE
        );

    //Enable I2C Module to start operations
    USCI_B_I2C_enable(USCI_B0_BASE);

    //Enable master Receive interrupt
    USCI_B_I2C_enableInterrupt(USCI_B0_BASE,
    		USCI_B_I2C_RECEIVE_INTERRUPT
        );

    //wait for bus to be free
    while (USCI_B_I2C_isBusBusy(USCI_B0_BASE )) ;

    while (1)
    {
        receiveBufferPointer = (unsigned char *)receiveBuffer;
        receiveCount = RXCOUNT;

        //Initialize multi reception
        USCI_B_I2C_masterReceiveMultiByteStart(USCI_B0_BASE);

        //Enter low power mode 0 with interrupts enabled.
        __bis_SR_register(LPM0_bits + GIE);
        __no_operation();
    }
}

//******************************************************************************
//
//This is the USCI_B0 interrupt vector service routine.
//
//******************************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_B0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(USCI_B0_VECTOR)))
#endif
void USCI_B0_ISR (void)
{
    switch (__even_in_range(UCB0IV,12)){
        case USCI_I2C_UCRXIFG:
        {
            //Decrement RX byte counter
            receiveCount--;

            if (receiveCount){
                if (receiveCount == 1) {
                    //Initiate end of reception -> Receive byte with NAK
                    *receiveBufferPointer++ =
                        USCI_B_I2C_masterReceiveMultiByteFinish(
                        		USCI_B0_BASE
                            );
                }
                else {
                    //Keep receiving one byte at a time
                    *receiveBufferPointer++ = USCI_B_I2C_masterReceiveMultiByteNext(
                    		USCI_B0_BASE
                        );
                }
            }
            else {
                //Receive last byte
                *receiveBufferPointer = USCI_B_I2C_masterReceiveMultiByteNext(
                		USCI_B0_BASE
                    );
                __bic_SR_register_on_exit(LPM0_bits);
            }
            break;
        }
    }
}

