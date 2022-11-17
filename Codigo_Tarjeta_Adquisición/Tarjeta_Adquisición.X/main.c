/*
 * File:   main.c
 * Author: Javier
 *
 * Created on September 22, 2022, 5:55 PM
 */
/*Pragmas*/
// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection->INTOSC oscillator: I/O function on CLKIN pin
#pragma config WDTE = ON    // Watchdog Timer Enable->WDT disabled
#pragma config PWRTE = OFF    // Power-up Timer Enable->PWRT disabled
#pragma config MCLRE = ON    // MCLR Pin Function Select->MCLR/VPP pin function is MCLR
#pragma config CP = OFF    // Flash Program Memory Code Protection->Program memory code protection is disabled
#pragma config CPD = OFF    // Data Memory Code Protection->Data memory code protection is disabled
#pragma config BOREN = OFF    // Brown-out Reset Enable->Brown-out Reset disabled
#pragma config CLKOUTEN = OFF    // Clock Out Enable->CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin
#pragma config IESO = ON    // Internal/External Switchover->Internal/External Switchover mode is enabled
#pragma config FCMEN = ON    // Fail-Safe Clock Monitor Enable->Fail-Safe Clock Monitor is enabled

// CONFIG2
#pragma config WRT = OFF    // Flash Memory Self-Write Protection->Write protection off
#pragma config VCAPEN = OFF    // Voltage Regulator Capacitor Enable->All VCAP pin functionality is disabled
#pragma config PLLEN = ON    // PLL Enable->4x PLL enabled
#pragma config STVREN = ON    // Stack Overflow/Underflow Reset Enable->Stack Overflow or Underflow will cause a Reset
#pragma config BORV = LO    // Brown-out Reset Voltage Selection->Brown-out Reset Voltage (Vbor), low trip point selected.
#pragma config LVP = ON    // Low-Voltage Programming Enable->Low-voltage programming enabled

/*Librerias*/
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <string.h>
#include "Init.h"
#include "Sensores.h" 
#include "1wire.h"
#include "Config.h"
/*Definiciones*/
#define _XTAL_FREQ 32000000
/*Variables Globales*/
unsigned char port_save=0;
unsigned char cs=0, TOUT=0, Lectura, med_sl, endi2c=0, i2cbgin=0, i2cfinish=0;
unsigned int Temp=0, Lum=0, meddone=0, debug=0;
unsigned long weight =0;
unsigned char i2cena =0, i2cqt=0, i2select=0, i2busy=0, act_com=0;
/*Arreglos para envio de datos*/
unsigned char dhtresult[4];
unsigned char tempar[4], humar[4];
unsigned char TEMPDS[4];
unsigned char LUX[5];
unsigned char KG[8];
unsigned char COM[]={39, '0', 39, ':'};
unsigned char DHTS[]={'{', 39, 'h', 39, ':', 39, 'N', 'N', 'N', 'N', 39, ',', 39, 't', 39, ':', 39, 'N', 'N', 'N', 'N', 39, '}'};
unsigned char DS18B[]={'{', 39, 'T', 39, ':', 39, 'N', 'N', 'N', 'N', 39, '}'};
unsigned char VEML7700[]={'{', 39, 'l', 39, ':', 39, 'N', 'N', 'N', 'N', 'N', 39, '}'};
unsigned char LOAD_CELL[]={'{', 39, 'w', 39, ':', 39, 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',39, '}'};
/*Arreglos de almacenamiento puertos*/
unsigned char sport[10];

void delay_1s (void)
 { for (short i=0; i<50; i++)
      __delay_ms(100);   
 }

__bit delay_s (unsigned char time)
 { for (short i=0; i<time; i++)
      __delay_ms(100);   
 return 1;
 }

void DHTRESULT(unsigned char port, unsigned char tris, unsigned char conector, unsigned char TOUT, unsigned char dht[], unsigned char COM[])
{
      static unsigned int temp=0, hum=0;  
      DHT22READ(port, tris, conector, TOUT, dht, COM);
      hum=(unsigned int)((dht[0]<<8) | dht[1]);
      temp=(unsigned int)((dht[2]<<8) | dht[3]);
      sprintf(tempar,"%u",temp);
      sprintf(humar,"%u",hum);
      for(int j=0; j<4; j++){
          if(humar[j]==0) humar[j]='N';
          if(tempar[j]==0) tempar[j]='N';
      }
      DHTS[17]=tempar[0];
      DHTS[18]=tempar[1];
      DHTS[19]=tempar[2];
      DHTS[20]=tempar[3];
      DHTS[6]=humar[0];
      DHTS[7]=humar[1];
      DHTS[8]=humar[2];
      DHTS[9]=humar[3];
      tempar[0]=0;
      tempar[1]=0;
      tempar[2]=0;
      tempar[3]=0;
      humar[0]=0;
      humar[1]=0;
      humar[2]=0;
      humar[3]=0;
      med_sl=1;
      PIE1bits.TXIE = 1;//Imprime los valores en el puerto serial
}

void DS18B20_Read(unsigned char *TRIS, unsigned char *PORT, unsigned char *LAT, unsigned char PIN, unsigned char entry){
          OW_reset_pulse(&TRISA, &PORTA, &LATA, PIN);  // Retornamos 0 si hay algun error.
          
          OW_write_byte(0xCC, &TRISA, &LATA, PIN);   // Enviamos el comando de salto ROM
          OW_write_byte(0x44, &TRISA, &LATA, PIN);   // Enviamos el comando de conversion de inicio.

          __delay_ms(800); //Esperamos la conversion de la temperatura
          
          OW_reset_pulse(&TRISA, &PORTA, &LATA, PIN); // Retornamos 0 si hay algun error.
          
          OW_write_byte(0xCC, &TRISA, &LATA, PIN);   // Enviamos el comando de salto ROM
          OW_write_byte(0xBE, &TRISA, &LATA, PIN);   // Enviamos el comando de lectura
          // Leemos el byte LSB de temperatura y lo guardamos en Temp
          Temp=0;
          Temp  = OW_read_byte(&TRISA, &PORTA, &LATA, PIN);
          // Leemos el byte MSB de temperatura y lo guardamos en Temp
          Temp |= (OW_read_byte(&TRISA, &PORTA, &LATA, PIN) << 8);
          OW_reset_pulse(&TRISA, &PORTA, &LATA, PIN); 
          sprintf(TEMPDS,"%u",Temp);
          for(int j=0; j<4; j++)
          if(TEMPDS[j]==0) TEMPDS[j]='N';
          COM[1]=entry + 48;
          DS18B[6]=TEMPDS[0];
          DS18B[7]=TEMPDS[1];
          DS18B[8]=TEMPDS[2];
          DS18B[9]=TEMPDS[3];
          TEMPDS[0]=0;
          TEMPDS[1]=0;
          TEMPDS[2]=0;
          TEMPDS[3]=0;
          med_sl=2;
          PIE1bits.TXIE = 1;
}

void pinlow(unsigned char *TRIS, unsigned char *PORT, unsigned char PIN)
{
    *TRIS &= (~PIN);
    *PORT &= (~PIN);
}
void pinhigh(unsigned char *TRIS, unsigned char *PORT, unsigned char PIN)
{
    *TRIS &= (~PIN);
    *PORT |= PIN;
}
void testports(unsigned char *TRIS, unsigned char *PORT, unsigned char PIN)
{
    pinlow(TRIS, PORT, PIN);
    __delay_ms(500);
    pinhigh(TRIS, PORT, PIN);
    __delay_ms(500);
}

void i2cwrite2byte (unsigned char add, unsigned char dir, unsigned int data){
   static char si3=0;// variable para cambiar de estado
    switch (si3){
           case 0:
        SSPBUF = add; //configuracion de escritura
        si3=1;
        break;
           case 1:
        SSPBUF = dir;//ingresa direccion alta   
        si3=2;
        break;
           case 2:
        SSPBUF = (unsigned char)(data & 0x00FF);//ingresa direccion baja  
        si3=3;
        break;
           case 3:
        SSPBUF = (unsigned char)(data >> 8);//ingresa dato a escribir
        si3=4;
        break;
            case 4:
        si3=5;
        SSPCON2bits.PEN = 1; //Genera condicion de Stop
        break;
        case 5:
        si3=0;
        PIE1bits.SSPIE = 0;//Deshabibilita las interrupciones de i2c
        endi2c=1;
        break;
        
       }
}



void i2cwritebyte (unsigned char add, unsigned char dir, unsigned char data){
   static char si3=0;// variable para cambiar de estado
    switch (si3){
           case 0:
        SSPBUF = add; //configuracion de escritura
        si3=1;
        break;
           case 1:
        SSPBUF = dir;//ingresa direccion alta   
        si3=2;
        break;
           case 2:
        SSPBUF = data;//ingresa direccion baja  
        si3=3;
        break;
            case 3:
        si3=4;
        SSPCON2bits.PEN = 1; //Genera condicion de Stop
        break;
        case 4:
        si3=0;
        PIE1bits.SSPIE = 0;//Deshabibilita las interrupciones de i2c
        endi2c=1;
        break;
        
       }
}

unsigned int i2creadbyte (unsigned char add, unsigned char dir, unsigned int *value){
   static char si2=0;
    switch (si2){
           case 0:
        SSPBUF = add; //Configuracion de escritura
        si2=1;
        break;
           case 1:
        SSPBUF = dir;//ingresa direccion alta   
        si2=2;
        break;
            case 2:
        SSPCON2bits.RSEN = 1;//Genera la condicion de Restart 
        si2=3;
        break;
            case 3:
        SSPBUF = add | 0x01; //Vuelve a enviar el byte de control pero con configuración de lectura
        si2=4;
        break;
            case 4:
        si2=5;
        SSPCON2bits.RCEN = 1;//Porque se configura el maestro como receptor. Se baja automaticamente
        break;
            case 5:
        *value = SSPBUF; //Dato recibido
        si2=6;
        SSPCON2bits.ACKDT = 0; //NACK del maestro, se genera cuando se termina de leer
        SSPCON2bits.ACKEN = 1; 
        break;
        case 6:
        si2=7;
        SSPCON2bits.RCEN = 1;//Porque se configura el maestro como receptor. Se baja automaticamente
        break;
            case 7:
        *value |= SSPBUF<<8; //Dato recibido
        si2=8;
        SSPCON2bits.ACKDT = 1; //NACK del maestro, se genera cuando se termina de leer
        SSPCON2bits.ACKEN = 1; 
        break;
            case 8:
        si2=9;
        SSPCON2bits.PEN = 1; //Genera condicion de Stop
        break; 
            case 9:
        si2=0;
        PIE1bits.SSPIE = 0;//Deshabilita interrupción i2c
        endi2c=1;
        break; 
       }
}

unsigned long i2cread3byte (unsigned char add, unsigned char dir, unsigned long *value){
   static char si2=0;
    switch (si2){
           case 0:
        SSPBUF = add; //Configuracion de escritura
        si2=1;
        break;
           case 1:
        SSPBUF = dir;//ingresa direccion alta   
        si2=2;
        break;
            case 2:
        SSPCON2bits.RSEN = 1;//Genera la condicion de Restart 
        si2=3;
        break;
            case 3:
        SSPBUF = add + 1 ; //Vuelve a enviar el byte de control pero con configuración de lectura
        si2=4;
        break;
            case 4:
        si2=5;
        SSPCON2bits.RCEN = 1;//Porque se configura el maestro como receptor. Se baja automaticamente
        break;
            case 5:
        *value = SSPBUF <<16; //Dato recibido
        si2=6;
        SSPCON2bits.ACKDT = 0; //NACK del maestro, se genera cuando se termina de leer
        SSPCON2bits.ACKEN = 1; 
        break;
        case 6:
        si2=7;
        SSPCON2bits.RCEN = 1;//Porque se configura el maestro como receptor. Se baja automaticamente
        break;
            case 7:
        *value |= SSPBUF<<8; //Dato recibido
        si2=8;
        SSPCON2bits.ACKDT = 0; //NACK del maestro, se genera cuando se termina de leer
        SSPCON2bits.ACKEN = 1; 
        break;
            case 8:
        *value |= SSPBUF; //Dato recibido
        si2=9;
        SSPCON2bits.ACKDT = 1; //NACK del maestro, se genera cuando se termina de leer
        SSPCON2bits.ACKEN = 1; 
        break;
            case 9:
        si2=10;
        SSPCON2bits.PEN = 1; //Genera condicion de Stop
        break; 
            case 10:
                
        si2=0;
        endi2c=2;
        break; 
       }
}

void i2c_Read(unsigned char com){
      if(i2cbgin){
      __delay_ms(100);
      PIE1bits.SSPIE = 1;//Habilita la interrupcion del i2c
      SSPCON2bits.SEN = 1; //Genera la condicion de Start
      i2cbgin=0;
      }
      if(i2cfinish==1 && i2select==1){
          sprintf(LUX,"%u",Lum);
          for(int j=0; j<5; j++)
          if(LUX[j]==0) LUX[j]='N';
          COM[1]= com + 48;
          VEML7700[6]=LUX[0];
          VEML7700[7]=LUX[1];
          VEML7700[8]=LUX[2];
          VEML7700[9]=LUX[3];
          VEML7700[10]=LUX[4];
          LUX[0]=0;
          LUX[1]=0;
          LUX[2]=0;
          LUX[3]=0;
          LUX[4]=0;
          med_sl=3;
          i2cfinish=0;
          i2busy=0;
          PIE1bits.TXIE = 1;
      }if(i2cfinish==1 && i2select==2){
          sprintf(KG,"%u",weight);
          for(int j=0; j<8; j++)
          if(KG[j]==0) KG[j]='N';
          COM[1]= com + 48;
          LOAD_CELL[6]=KG[0];
          LOAD_CELL[7]=KG[1];
          LOAD_CELL[8]=KG[2];
          LOAD_CELL[9]=KG[3];
          LOAD_CELL[10]=KG[4];
          LOAD_CELL[11]=KG[5];
          LOAD_CELL[12]=KG[6];
          LOAD_CELL[13]=KG[7];
          med_sl=4;
          i2cfinish=0;
          i2busy=0;
          PIE1bits.TXIE = 1;
      }
}

void port_config(unsigned char stype[]){
    if(stype[0]<='2'){ //com1 config
        RA0=0;
        RC1=0;
        TRISA |= 0x01;
    }else{
        RA0=0;
        RC1=1;
        TRISA &= ~0x01;
    }
    if(stype[1]<='2'){ //com2 config
        RA1=1;
        RC2=0;
        TRISA |= 0x02;
    }else{
        RA1=0;
        RC2=1;
        TRISA &= ~0x02;
    }
    if(stype[2]<='2'){ //com3 config
        RA2=1;
        RC5=0;
        TRISA |= 0x04;
    }else{
        RA2=0;
        RC5=1;
        TRISA &= ~0x04;
    }
    if(stype[3]<='2'){ //com4 config
        RA3=1;
        RB2=0;
        TRISA |= 0x08;
    }else{
        RA3=0;
        RB2=1;
        TRISA &= ~0x08;
    }
    if(stype[4]<='2'){ //com5 config
        RA4=1;
        RB1=0;
        TRISA |= 0x10;
    }else{
        RA4=0;
        RB1=1;
        TRISA &= ~0x10;
    }
    if(stype[5]<='2'){ //com6 config
        RC0=1;
        RB5=0;
        TRISC |= 0x01;
    }else{
        RC0=0;
        RB5=1;
        TRISC &= ~0x01;
    }
    if(stype[6]<='2'){ //com7 config
        RA6=1;
        RB3=0;
        TRISA |= 0x40;
    }else{
        RA6=1;
        RB3=1;
        TRISA &= ~0x40;
    }
    if(stype[7]<='2'){ //com8 config
        RA7=1;
        RB4=0;
        TRISA |= 0x80;
    }else{
        RA7=0;
        RB4=1;
        TRISA &= ~0x80;
    }
    if(stype[8]<='2'){ //com9 config
        RA5=1;
        RB0=0;
        TRISA |= 0x20;
    }else{
        RA5=0;
        RB0=1;
        TRISA &= ~0x20;
    }
    if(stype[9]<='2'){ //com10 config
        RB7=1;
        TRISB |= 0x80;
    }
}

void main(void) {
    Sys_Init();
    WDTCON=0x1B;
    for(unsigned char i=0; i<10; i++) {
        sport[i]= EEPROM_READ(i);
        if(sport[i]>='3') i2cqt++;
    }
    port_config(sport);
    __delay_ms(100);
    //med_sl=5, COM[1]=48, PIE1bits.TXIE = 1;
    while(1){
    CLRWDT();
    if(port_save==1){
        for(unsigned char i=0; i<10; i++)
        EEPROM_WRITE(i,sport[i]);
        port_config(sport);
        __delay_ms(100);
        port_save=0;    
    }
        if(Lectura==1){
           // __delay_ms(100);
            i2cfinish=0;
            i2cbgin=1;
            Lectura=0;
        }
    
    i2c_Read(act_com);
/*
        if(Lectura==2){
      DHTRESULT(0x01, 1, 6, TOUT, dhtresult, COM);
      DHTRESULT(0x01, 0, 1, TOUT, dhtresult, COM);
      DHTRESULT(0x02, 0, 2, TOUT, dhtresult, COM);
      //DHTRESULT(0x04, 0, 3, TOUT, dhtresult, COM);
      Lectura=0;
      DS18B20_Read(&TRISA, &PORTA, &LATA, 0x10, 5);
      //DS18B20_Read(&TRISA, &PORTA, &LATA, 0x08, 4);
      //BAUDCON |= 0x02;
      //SLEEP();
      __delay_ms(10);
      RESET(); 
        }*/
        /*Selección de sensor y realización de medida*/
    if(((meddone & 0x01)>0)){ //com 1
        if(sport[0]=='1' && i2busy==0){
           __delay_ms(10);
           DHTRESULT(0x01, 0, 1, TOUT, dhtresult, COM);
           meddone &= ~0x01 ;
        }if(sport[0]=='2' && i2busy==0){
           DS18B20_Read(&TRISA, &PORTA, &LATA, 0x01, 1);
           meddone &= ~0x01 ; 
        }if(sport[0]=='3' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=1;
           i2busy=1;
           act_com=1;
           meddone &= ~0x01;
        }if(sport[0]=='4' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=2;
           i2busy=1;
           act_com=1;
           meddone &= ~0x01;
        }
    }if(((meddone & 0x02)>0)){ // com 2
        if(sport[1]=='1' && i2busy==0){
           DHTRESULT(0x02, 0, 2, TOUT, dhtresult, COM);
           meddone &= ~0x02;
        }if(sport[1]=='2' && i2busy==0){
           DS18B20_Read(&TRISA, &PORTA, &LATA, 0x02, 2);
           meddone &= ~0x02; 
        }if(sport[1]=='3' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=1;
           i2busy=1;
           act_com=2;
           meddone &= ~0x02;
        }if(sport[1]=='4' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=2;
           i2busy=1;
           act_com=2;
           meddone &= ~0x02;
        }     
    }if(((meddone & 0x04)>0)){ // cpm 3
        if(sport[2]=='1' && i2busy==0){
           DHTRESULT(0x04, 0, 3, TOUT, dhtresult, COM);
           meddone &= ~0x04;
        }if(sport[2]=='2' && i2busy==0){
           DS18B20_Read(&TRISA, &PORTA, &LATA, 0x04, 3);
           meddone &= ~0x04; 
        }if(sport[2]=='3' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=1;
           i2busy=1;
           act_com=3;
           meddone &= ~0x04;
        }if(sport[2]=='4' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=2;
           i2busy=1;
           act_com=3;
           meddone &= ~0x04;
        }     
    }if(((meddone & 0x08)>0)){ // com 4
        if(sport[3]=='1' && i2busy==0){
           DHTRESULT(0x08, 0, 4, TOUT, dhtresult, COM);
           meddone &= ~0x08;
        }if(sport[3]=='2' && i2busy==0){
           DS18B20_Read(&TRISA, &PORTA, &LATA, 0x08, 4);
           meddone &= ~0x08; 
        }if(sport[3]=='3' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=1;
           i2busy=1;
           act_com=4;
           meddone &= ~0x04;
        }if(sport[3]=='4' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=2;
           i2busy=1;
           act_com=4;
           meddone &= ~0x04;
        }     
    }if(((meddone & 0x10)>0)){ // com 5
        if(sport[4]=='1' && i2busy==0){
           DHTRESULT(0x10, 0, 5, TOUT, dhtresult, COM);
           meddone &= ~0x10 ;
        }if(sport[4]=='2' && i2busy==0){
           DS18B20_Read(&TRISA, &PORTA, &LATA, 0x10, 5);
           meddone &= ~0x10 ; 
        }if(sport[4]=='3' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=1;
           i2busy=1;
           act_com=5;
           meddone &= ~0x10;
        }if(sport[4]=='4' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=2;
           i2busy=1;
           act_com=5;
           meddone &= ~0x10;
        }   
    }if(((meddone & 0x20)>0)){ // com 6
        if(sport[5]=='1' && i2busy==0){
           DHTRESULT(0x01, 1, 6, TOUT, dhtresult, COM);
           meddone &= ~0x20 ;
        }if(sport[5]=='2' && i2busy==0){
           DS18B20_Read(&TRISC, &PORTC, &LATC, 0x01, 6);
           meddone &= ~0x20 ; 
        }if(sport[5]=='3' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=1;
           i2busy=1;
           act_com=6;
           meddone &= ~0x20;
        }if(sport[5]=='4' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=2;
           i2busy=1;
           act_com=6;
           meddone &= ~0x20;
        }   
    }if(((meddone & 0x40)>0)){ // com 7
        if(sport[6]=='1' && i2busy==0){
           DHTRESULT(0x40, 0, 7, TOUT, dhtresult, COM);
           meddone &= ~0x40 ;
        }if(sport[6]=='2' && i2busy==0){
           DS18B20_Read(&TRISA, &PORTA, &LATA, 0x40, 7);
           meddone &= ~0x40 ; 
        }if(sport[6]=='3' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=1;
           i2busy=1;
           act_com=7;
           meddone &= ~0x40;
        }if(sport[6]=='4' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=2;
           i2busy=1;
           act_com=7;
           meddone &= ~0x40;
        }      
    }if(((meddone & 0x80)>0)){ //com 8
        if(sport[7]=='1' && i2busy==0){
            //med_sl=5, COM[1]=48, PIE1bits.TXIE = 1;
           DHTRESULT(0x80, 0, 8, TOUT, dhtresult, COM);
           meddone &= ~0x80 ;
        }if(sport[7]=='2' && i2busy==0){
           DS18B20_Read(&TRISA, &PORTA, &LATA, 0x80, 7);
           meddone &= ~0x80 ; 
        }if(sport[7]=='3' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=1;
           act_com=8;
           meddone &= ~0x80;
        }if(sport[7]=='4' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=2;
           i2busy=1;
           act_com=8;
           meddone &= ~0x80;
        }     
    }if(((meddone & 0x100)>0)){
        if(sport[8]=='1' && i2busy==0){
           DHTRESULT(0x20, 0, 9, TOUT, dhtresult, COM);
           meddone &= ~0x100 ;
        }if(sport[8]=='2' && i2busy==0){
           DS18B20_Read(&TRISA, &PORTA, &LATA, 0x20, 9);
           meddone &= ~0x100 ; 
        }if(sport[8]=='3' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=1;
           act_com=9;
           meddone &= ~0x100;
        }if(sport[8]=='4' && i2busy==0){
           i2cfinish=0;
           i2cbgin=1;
           i2select=2;
           i2busy=1;
           act_com=9;
           meddone &= ~0x100;
        }     
    }/*if(((meddone & 0x200)>0)){
        if(sport[9]=='1' && i2busy==0){
           //DHTRESULT(0x20, 0, 9, TOUT, dhtresult, COM);
           meddone &= ~0x200 ;
        }if(sport[9]=='2' && i2busy==0){
           DS18B20_Read(&TRISB, &PORTB, &LATB, 0x80, 10);
           meddone &= ~0x200 ; 
        }  
    }*/
    
    
    }//fin del while 1

    
}//fin del main
void TXsend(unsigned char word[], unsigned char size){
    static unsigned char i=0;
    if(i==0){
       //size = strlen(word);
       TXREG=word[i];
       i++;
    }
    else if(i!=0 && i < size){
        TXREG=word[i];
        i++;
    }else if(i>=size){
        i=0;
        cs=cs+1;  
    }
}

void __interrupt() INTERRUPT_InterruptManager (void) //Función de Interrupción
{
    //TRISA &= (~0x10);
    //RA4!=RA4;
    static unsigned char i2cstep=0;
    static unsigned char rp=0, h=0, sspo=0;
    if(RCIF){
    switch(rp){
        case 0:
            h = RCREG;
            if(h=='C') rp=1, h=0, sspo=0;; //C=0x43, C de configuracion se envia una C en ascii junto a la configuracion de puertos.
            if(h=='R') meddone=0x3FF, h=0;
            if(h=='M') rp=3, h=0;
            if(h=='L') med_sl=5, COM[1]=48, h=0, PIE1bits.TXIE = 1;
            if(h=='0') meddone|= 0x01,h=0;
            if(h=='1') meddone|=0x02, h=0;
            if(h=='2') meddone|=0x04, h=0;
            if(h=='3') meddone|=0x08,  h=0;
            if(h=='4') meddone|=0x10,  h=0;
            if(h=='5') meddone|=0x20,  h=0;
            if(h=='6') meddone|=0x40, h=0;
            if(h=='7') meddone|=0x80, h=0;
            if(h=='8') meddone|=0x100, h=0;
            if(h=='9') meddone|=0x200, h=0;
            
            //if(h=='S') Lectura=2, h=0;
            break;
        case 1:
            if(sspo<9){
              sport[sspo]=RCREG; //Se recibe configuracion del mux con los puertos RB
              sspo++;
            }else{
              rp=2;  
            }
            break;
        case 2:
            sport[sspo]=RCREG;
            port_save=1;
            //med_sl=5;
            rp=0;
            //PIE1bits.TXIE = 1;
            break;
        case 3:
            sport[sspo]=RCREG;
            port_save=1;
            med_sl=5;
            rp=0;
            PIE1bits.TXIE = 1;
            break;
            
    }

    }
    static unsigned char plb[]="N";
    if(TXIF && PIE1bits.TXIE){
    /*
    switch(cs)
        {
            case 0:
            TXREG=weight>>16;
            cs=3;
            break;
            case 1:
            TXsend(plb, 1);
            break;
            case 2:
            PIE1bits.TXIE = 0;
            cs=0;
            break;
            case 3:
            TXREG=weight>>8;
            cs=4;
            break;
            case 4:
            TXREG=weight;
            cs=0;
            PIE1bits.TXIE = 0;
            break;
                
        }*/
        
        switch(cs)
        {
            case 0:
            TXREG='{';
            cs=1;
            break;
            case 1:
            TXsend(COM, 4);
            break;
            case 2:
            if(med_sl==1){
               TXsend(DHTS, 23); 
            }else if(med_sl==2){
               TXsend(DS18B, 12);
            }else if(med_sl==3){
               TXsend(VEML7700, 13);
            }else if(med_sl==4){
               TXsend(LOAD_CELL, 16);
            }else if(med_sl==5){
               TXsend(sport, 10);
            }
            break;
            case 3:
            TXREG='}'; 
            cs=4;
            break;
            case 4:
            TXREG=13; 
            cs=5;
            break;
            case 5:
            TXREG=10; 
            cs=0;
            PIE1bits.TXIE = 0;
            break;
                
        }
    }
    if(PIR1bits.SSPIF){
    if(i2select==1){
    switch(i2cstep)
     {
         case 0:
         i2cwrite2byte (0x20, 0x00,0x0800);
         break;
         case 1:
         i2creadbyte (0x20, 0x04, &Lum);
         break;

     }
     if(endi2c && (!i2cstep)) {
         i2cstep++;
         Lectura=1;
         endi2c=0;
     }else if(endi2c && (i2cstep)){
         i2cstep=0;
         endi2c=0;
         i2cfinish=1;
     }
    }
    if(i2select==2){
    switch(i2cstep)
     {
         case 0:
         i2cwrite2byte (0x2A<<1, 0x00,0x2586);
         break;
         case 1:
         i2cwritebyte (0x2A<<1, 0x02,0x30);
         break;
         case 2:
         i2cwritebyte (0x2A<<1, 0x11,0x51);
         break;
         case 9:
         i2cwritebyte (0x2A<<1, 0x15,0x30);
         break;
         case 3:
         i2cwritebyte (0x2A<<1, 0x1C,0x80);
         break;
         case 4:
         i2cread3byte (0x2A<<1, 0x12, &weight);
         break;
         

     }
     if(endi2c ==1) {
         i2cstep++;
         Lectura=1;
         endi2c=0;
     }if(endi2c ==2){
         i2cstep=0;
         endi2c=0;
         med_sl=5, COM[1]=48, PIE1bits.TXIE = 1;
         i2cfinish=1;
         PIE1bits.SSPIE = 0;//Deshabilita interrupción i2c
     }
    }
    PIR1bits.SSPIF = 0;//Debe ser bajada por software
   }
    if (TMR2IF || TMR2>100)
    { TOUT = 1; //Indica que hubo un error de lectura
      TMR2 = 0; //Reinicio del contador
      TMR2ON = 0;
      TMR2IF = 0; // Limpia la bandera de interrupcion
    }
}