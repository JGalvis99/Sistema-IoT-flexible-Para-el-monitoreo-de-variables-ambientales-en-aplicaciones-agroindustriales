
#ifndef _1wire_H
#define _1wire_H

/** I N C L U D E S **********************************************************/
/******* G E N E R I C   D E F I N I T I O N S ************************************************/
#define _XTAL_FREQ 32000000  
/** P R O T O T Y P E S ******************************************************/
unsigned char read_OW (unsigned char *TRIS, unsigned char *PORT, unsigned char PIN);
unsigned char OW_reset_pulse(unsigned char *TRIS, unsigned char *PORT, unsigned char *LAT, unsigned char PIN);
void OW_write_bit (unsigned char write_bit, unsigned char *TRIS, unsigned char *LAT, unsigned char PIN);
unsigned char OW_read_bit (unsigned char *TRIS, unsigned char *PORT, unsigned char *LAT, unsigned char PIN);
void OW_write_byte (unsigned char write_data, unsigned char *TRIS, unsigned char *LAT, unsigned char PIN);
unsigned char OW_read_byte (unsigned char *TRIS, unsigned char *PORT, unsigned char *LAT, unsigned char PIN);

/*****************************************************************************
   V A R I A B L E S
******************************************************************************/

#endif