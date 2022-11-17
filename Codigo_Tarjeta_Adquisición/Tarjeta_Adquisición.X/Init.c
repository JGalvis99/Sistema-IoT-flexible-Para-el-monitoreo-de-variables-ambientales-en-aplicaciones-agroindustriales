/*
 * File:   Init.c
 * Author: Javier
 *
 * Created on September 22, 2022, 6:08 PM
 */


#include <xc.h>
#include "Init.h"
#define _XTAL_FREQ 32000000  
#define mbmsk 0xC0
#define mcmsk 0xD9
void Pin_init(void)//Inicialización de pines
{
    /**
    LATx registers
    */
    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;
    /**
    TRISx registers
    */
    TRISA = 0xFF;
    TRISB = 0x80;
    TRISC = 0xD9;
    /**
    ANSELx registers
    */
    ANSELB = 0x00;
    ANSELA = 0x00;
    /**
    WPUx registers
    */
    WPUE = 0x00;
    WPUB = 0x00;
    /**
    APFCONx registers
    */
    APFCON = 0x00;
    /*Inicializacion de pines en 0*/
    PORTA = 0x00;
    PORTB = 0x00;
    PORTC = 0x00;
}
void Reg_init(void)//Configuración de Registros
{   //Registros Definición Oscilador
    OSCCON = 0x70; // Oscilador Interno 8Mhz x 4PLL habilitado
    OSCTUNE = 0x00; //Oscilator tune disable
    BORCON = 0x00; //Brown-On Reset Disable    
    //Registros de Interrupciones
    INTCON = 0xE0; //Habilita Interrupciones globales, perifericas y del timer 1
    //Registros del UART y su interrupcion
    // Set the EUSART module to the options selected in the user interface.
    // ABDOVF no_overflow; SCKP Non-Inverted; BRG16 16bit_generator; WUE disabled; ABDEN disabled; 
    BAUDCON = 0x08;
    SPBRG = 68; //115900 baudios 0.6 de error, la mejor equivalencia a 115200 bd
    // SPEN enabled; RX9 8-bit; CREN enabled; ADDEN disabled; SREN disabled; 
    RCSTA = 0x90;
    // TX9 8-bit; TX9D 0; SENDB sync_break_complete; TXEN enabled; SYNC asynchronous; BRGH hi_speed; CSRC slave; 
    TXSTA = 0x24;
    PIE1bits.RCIE = 1;
    /*Registros de timer 1*/
    T1CON = 0x30; //FOSC/4, 1:8
    /*Registro timer 2*/
    T2CON = 0x01; // FOSC/4, 1:4, apagado
    PIE1bits.TMR2IE = 1;
    
    //I2C Regs
    SSPSTAT = 0x00;
    SSPCON1 = 0x08;
    SSPCON2 = 0x00;
    SSPADD  = 0x4F;
    SSPCON1bits.SSPEN = 0;
    __delay_ms(10);
    
    SSPCON1bits.SSPEN = 1;
    PIE1bits.SSPIE = 0;
    

}
void Sys_Init(void)
{
    Reg_init();
    Pin_init();
}
/*
void I2C_Initialize()
{
    SSPSTAT = 0x00;
    SSPCON1 = 0x08;
    SSPCON2 = 0x00;
    SSPADD  = 0x13;
    SSPCON1bits.SSPEN = 0;
    

}

  */

