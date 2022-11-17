/*
 * File:   Init.c
 * Author: Javier
 *
 * Created on September 22, 2022, 6:08 PM
 */


#include <xc.h>
#include "Sensores.h"
#define _XTAL_FREQ 32000000  

void start_signal(unsigned char port, unsigned char tris)
{ 
   if(tris==0){ //La configuracion depende el conector en algunos casos es el A o el C 
   TRISA &= (~port);     // Se coloca el puerto como salida 
   PORTA &= (~port);    // Se envia un 0 para que el sensor responda
   __delay_ms(18);    // Se debe esperar un intervalo de 18ms a que responda el sensor
   TRISA |= port;     // Se devuelve el puerto a entrada
   __delay_us(30);    // Espera 20-40us a que responda el sensor
}else{
   TRISC &= (~port);     // Data port is output
   PORTC &= (~port);
   __delay_ms(18);
   TRISC |= port;     // Data port is input
   __delay_us(30); 
}
}
unsigned char check_response(unsigned char port, unsigned char tris, unsigned char TOUT)
 { 
   TOUT = 0; //Señal que indica si hubo un error en la lectura
   TMR2 = 0; //Reinicio del contador
   TMR2ON = 1; // Inicia el timer 2
   if(tris==0){
   while (!(PORTA & port) && !TOUT);//Espera a que haya un valor en el puerto o haya un error
   if (TOUT) 
      return 0;// Si encuentra un error retorna un 0
   else 
    { TMR2 = 0; //Si no encuentra un error reinicia el timer para que no haya conflicto
      while (((PORTA & port)>0) && !TOUT); //Espera a que acabe la señal en alto
      if (TOUT) //Si hay error nuevamente retorna un 0
         return 0;
      else 
      { TMR2ON = 0;//Si no hay error apaga el timer y retorna un 1 indicando que esta listo para leer
         return 1;
       }
    }
   }else{
   while (!(PORTC & port) && !TOUT);
   if (TOUT) 
      return 0;
   else 
    { TMR2 = 0;
      while (((PORTC & port)>0) && !TOUT);
      if (TOUT) 
         return 0;
      else 
       { TMR2ON = 0;
         return 1;
       }
    }  
   }
 }
unsigned char read_byte(unsigned char port, unsigned char tris)
 { unsigned char num = 0, i=0;
   TRISA |= port; //En caso de necesitar vuelve a colocar el puerto como entrada
   for (i=0; i<8; i++)//El For para llenar los valores del sensor 
   {
      if(tris==0){
      while (!(PORTA & port));//Espera a que pase el 0
      TMR2 = 0;
      TMR2ON = 1;
      while (((PORTA & port)>0)); //Mientras espera a que pase el valor en alto el timer cuenta cada 0.5us
      TMR2ON = 0;
      if (TMR2 > 80) 
         num |= 1<<(7-i);  // Si el valor del timer es mayor a 40us indica que es un 1
   }else{
       while (!(PORTC & port));
      TMR2 = 0;
      TMR2ON = 1;
      while (((PORTC & port)>0));
      TMR2ON = 0;
      if (TMR2 > 80) 
         num |= 1<<(7-i);  // If time > 40us, Data is 1
   }
    }
   i=0;
   return num;
 }
void DHT22READ (unsigned char port, unsigned char tris, unsigned char entry, unsigned char TOUT, unsigned char dhtresult[], unsigned char COM[])
{
    static unsigned char check = 0;
    start_signal(port, tris);
    check = check_response(port, tris, TOUT);
    if (!check) 
       {
          PIE1bits.TXIE = 1;//Si hay un error imprime lo que haya en el TX
       }
      else
       {
         dhtresult[0] = read_byte(port, tris); //lee el valor alto de la humedad
         dhtresult[1]  = read_byte(port, tris); //lee el valor bajo de la humedad
         dhtresult[2]=  read_byte(port, tris); //Valor alto de temperatura
         dhtresult[3] = read_byte(port, tris); //Valor bajo de temperatura
         COM[1]=entry + 48; //Toma el valor del puerto y le suma 48 para convertirlo en un numero ascii
      }
}
