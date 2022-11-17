/******* G E N E R I C   D E F I N I T I O N S ************************************************/
#define _XTAL_FREQ 32000000  
#define	HIGH	1
#define	LOW		0
#define	OUTPUT	0
#define	INPUT 	1
#define	SET		1
#define	CLEAR	0

//ONE WIRE PORT PIN DEFINITION
///****************************************************
// This Configuration is required to make any PIC MicroController
// I/O pin as Open drain to drive 1-wire.
///****************************************************
#define OW_PIN_DIRECTION 	LATAbits.LATA3
#define OW_WRITE_PIN  		TRISAbits.TRISA3
#define OW_READ_PIN			PORTAbits.RA3

