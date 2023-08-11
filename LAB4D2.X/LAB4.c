/*	MASTER//MASTER//MASTER//MASTER 
 * File:   main.c
 * Author: Bryan
 * Pre I2C Master
 * Created on 8 de agosto de 2023, 10:32 AM
 */
//*****************************************************************************
// Palabra de configuración
//*****************************************************************************
// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (RCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, RC on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

//*****************************************************************************
// Definición e importación de librerías
//*****************************************************************************
#include <stdint.h>
#include <pic16f887.h>
#include "I2C.h"
#include "LCD8B.h"
#include <xc.h>
#include <proc/pic16f887.h>
//*****************************************************************************
// Definición de variables
//*****************************************************************************
#define _XTAL_FREQ 8000000
uint8_t adc_val;
uint8_t seconds, secH, secL, minutes, minH, minL, hours, hrH, hrL;
uint8_t day, dH, dL, month, monH, monL, year, yrH, yrL;
uint8_t escritura=0, selector=0, contador=0;
//*****************************************************************************
// Definición de funciones para que se puedan colocar después del main de lo 
// contrario hay que colocarlos todas las funciones antes del main
//*****************************************************************************
void setup(void);
void writeTime(uint8_t dir, uint8_t d);
//*****************************************************************************
//INTERRUPCION
void __interrupt() isr(void){
    if(INTCONbits.RBIF){      //Interrupcion ON CHANGE botones
        if(PORTBbits.RB0==0){
            escritura = 1;      //poner en modo escritura de tiempo
            PORTEbits.RE0=1;
        }
        INTCONbits.RBIF = 0;      //Apagar bandera
    }
}
//*****************************************************************************
// Main
//*****************************************************************************
void main(void) {
    setup();
    Lcd_Clear();
    while(1){
        if(escritura==1){      //si estoy en modo escritura
            if(RB1==0){     //Si presiono selector
                __delay_ms(200);
                selector++;
            }
            if(RB2==0){      //Si presiono boton de incrementar
                __delay_ms(200);
                contador++;
            }
            if(RB3==0){      //Si presiono boton de decrementar
                __delay_ms(200);
                contador--;
            }
            
            if(selector==1){ //escritura de segundos
                if(contador>59){contador=0;}  //no hay segundos de 60, solo 59
                seconds=contador;
                secH=seconds/10;                //separacion decenas
                secL=seconds%10;                //separacion unidades
                seconds = secH<<4 | secL;       //unificacion preparada
                writeTime(0x00,seconds);        //escritura en RTC
            }
            if(selector==2){ //escritura de minutos
                if(contador>59){contador=0;}  
                minutes=contador;               
                minH=minutes/10;                //separacion
                minL=minutes%10;
                minutes = minH<<4 | minL;       //empaquetado
                writeTime(0x01,minutes);        //enviado
            }
            if(selector==3){ //escritura de horas
                if(contador>23){contador=0;}
                hours=contador;
                hrH=hours/10;
                hrL=hours%10;
                hours = hrH<<4 | hrL;
                writeTime(0x02,hours);
            }
            if(selector==4){ //escritura de dias
                if(contador>31){contador=0;}
                day=contador;
                dH=day/10;
                dL=day%10;
                day = dH<<4 | dL;
                writeTime(0x04,day);
            }
            if(selector==5){ //escritura de meses
                if(contador>12){contador=0;}
                month=contador;
                monH=month/10;
                monL=month%10;
                month = monH<<4 | monL;
                writeTime(0x05,month);
            }
            if(selector==6){ //escritura de anos
                if(contador>99){contador=0;}
                year=contador;
                yrH=year/10;
                yrL=year%10;
                year = yrH<<4 | yrL;
                writeTime(0x06,year);
            }
            if(selector==7){ //reseteo del selector
                selector=0;
                contador=0;
                escritura=0;
                PORTEbits.RE0=0;
            }
        }
        
        //Parte de lectura y escritura en LCD
        
        I2C_Master_Start();         //Conexion con PIC Slave
        I2C_Master_Write(0x51);     //Direccion y modo lectura
        adc_val = I2C_Master_Read(0);//lectura en variable
        I2C_Master_Stop();          //Fin conexion
        
        //Lectura de segundos
        I2C_Master_Start();         //Comienzo conexion RTC
        I2C_Master_Write(0xD0);     //Hablo a la direccion del RTC
        I2C_Master_Write(0x00);     //Hablo a la direccion de segundos
        I2C_Master_RepeatedStart();         //Retomo conexion
        I2C_Master_Write(0xD1);     //Hablo al RTC en modo lectura
        seconds = I2C_Master_Read(0);//Leo segundos
        I2C_Master_Stop();          //Termino conexion RTC
        secH= (seconds & 0xF0)>>4;  //Tomo bits altos para tener decenas de seg
        secL= seconds & 0x0F;       //Tomo bits bajos para unidades de seg
        //Lectura de minutos
        I2C_Master_Start();         //Comienzo conexion RTC
        I2C_Master_Write(0xD0);     //Hablo a la direccion del RTC
        I2C_Master_Write(0x01);     //Hablo a la direccion de minutos
        I2C_Master_RepeatedStart();         //Retomo conexion
        I2C_Master_Write(0xD1);     //Hablo al RTC en modo lectura
        minutes = I2C_Master_Read(0);//Leo segundos
        I2C_Master_Stop();          //Termino conexion RTC
        minH= (minutes & 0xF0)>>4;  //Tomo bits altos para tener decenas de min
        minL= minutes & 0x0F;       //Tomo bits bajos para unidades de min
        //Lectura de horas
        I2C_Master_Start();         //Comienzo conexion RTC
        I2C_Master_Write(0xD0);     //Hablo a la direccion del RTC
        I2C_Master_Write(0x02);     //Hablo a la direccion de horas
        I2C_Master_RepeatedStart();         //Retomo conexion
        I2C_Master_Write(0xD1);     //Hablo al RTC en modo horas
        hours = I2C_Master_Read(0);//Leo horas
        I2C_Master_Stop();          //Termino conexion RTC
        hrH= (hours & 0xF0)>>4;  //Tomo bits altos para tener decenas de horas
        hrL= hours & 0x0F;       //Tomo bits bajos para unidades de horas
        //Lectura de dias
        I2C_Master_Start();         //Comienzo conexion RTC
        I2C_Master_Write(0xD0);     //Hablo a la direccion del RTC
        I2C_Master_Write(0x04);     //Hablo a la direccion de dias
        I2C_Master_RepeatedStart();         //Retomo conexion
        I2C_Master_Write(0xD1);     //Hablo al RTC en modo lectura
        day = I2C_Master_Read(0);//Leo dias
        I2C_Master_Stop();          //Termino conexion RTC
        dH= (day & 0xF0)>>4;  //Tomo bits altos para tener decenas de dias
        dL= day & 0x0F;       //Tomo bits bajos para unidades de dias
        //Lectura de meses
        I2C_Master_Start();         //Comienzo conexion RTC
        I2C_Master_Write(0xD0);     //Hablo a la direccion del RTC
        I2C_Master_Write(0x05);     //Hablo a la direccion de meses
        I2C_Master_RepeatedStart();         //Retomo conexion
        I2C_Master_Write(0xD1);     //Hablo al RTC en modo lectura
        month = I2C_Master_Read(0);//Leo meses
        I2C_Master_Stop();          //Termino conexion RTC
        monH= (month & 0xF0)>>4;  //Tomo bits altos para tener decenas de meses
        monL= month & 0x0F;       //Tomo bits bajos para unidades de meses
        //Lectura de anos
        I2C_Master_Start();         //Comienzo conexion RTC
        I2C_Master_Write(0xD0);     //Hablo a la direccion del RTC
        I2C_Master_Write(0x06);     //Hablo a la direccion de anos
        I2C_Master_RepeatedStart();         //Retomo conexion
        I2C_Master_Write(0xD1);     //Hablo al RTC en modo lectura
        year = I2C_Master_Read(0);//Leo anos
        I2C_Master_Stop();          //Termino conexion RTC
        yrH= (year & 0xF0)>>4;  //Tomo bits altos para tener decenas de anos
        yrL= year & 0x0F;       //Tomo bits bajos para unidades de anos
        
        
        //Escritura en LCD de "Sensor1"
        char ADC[3];                
        ADC[0]=adc_val/100+48;
        ADC[1]=(adc_val/10)%10+48;
        ADC[2]=adc_val%10+48;
        //Lcd_Clear();
        Lcd_Set_Cursor(1,1);
        Lcd_Write_String("S1:");
        Lcd_Set_Cursor(2,1);
        Lcd_Write_Char(ADC[0]);
        Lcd_Write_Char(ADC[1]);
        Lcd_Write_Char(ADC[2]);
        
        //Separacion de variables numericas a caracteres.
        char segs[2],mins[2],hrs[2],days[2],months[2],years[2];
        segs[0]= secL + 48;   //unidades
        segs[1]= secH + '0';   //decenas
        mins[0]= minL + 48;   //unidades
        mins[1]= minH + '0';   //decenas
        hrs[0]= hrL + 48;   //unidades
        hrs[1]= hrH + '0';   //decenas
        days[0]= dL + 48;   //unidades
        days[1]= dH + '0';   //decenas
        months[0]= monL + 48;   //unidades
        months[1]= monH + '0';   //decenas
        years[0]= yrL + 48;   //unidades
        years[1]= yrH + '0';   //decenas
        Lcd_Set_Cursor(1,6);
        
        //Lcd_Write_String("Dia:");
        Lcd_Write_Char(days[1]);
        Lcd_Write_Char(days[0]);
        Lcd_Write_Char('/');
        Lcd_Write_Char(months[1]);
        Lcd_Write_Char(months[0]);
        Lcd_Write_Char('/');
        Lcd_Write_Char('2');
        Lcd_Write_Char('0');
        Lcd_Write_Char(years[1]);
        Lcd_Write_Char(years[0]);
        Lcd_Set_Cursor(2,6);
        Lcd_Write_Char(hrs[1]);
        Lcd_Write_Char(hrs[0]);
        Lcd_Write_Char(':');
        Lcd_Write_Char(mins[1]);
        Lcd_Write_Char(mins[0]);
        Lcd_Write_Char(':');
        Lcd_Write_Char(segs[1]);
        Lcd_Write_Char(segs[0]);
        //Lcd_Write_Char('S');
        __delay_ms(55);
        
        
    }
    return;
}
//*****************************************************************************
// Función de Inicialización
//*****************************************************************************
void setup(void){
    ANSEL = 0;
    ANSELH = 0;
    TRISB = 0x0F;
    TRISD = 0;
    PORTB = 0;
    PORTD = 0;
    TRISE = 0x00;
    PORTEbits.RE0=0;
    OSCCONbits.IRCF=0b111;
    OSCCONbits.SCS=1;
    Lcd_Init();
    I2C_Master_Init(100000);        // Inicializar Comuncación I2C
    
    
    //Configuracion para botones 
    INTCONbits.GIE = 1;         //Interrupcioes globales
    INTCONbits.RBIE = 1;        // Activo interrupción de botón
    INTCONbits.RBIF = 0;        // Apago bandera de interrupción
    OPTION_REGbits.nRBPU = 0;   // Activo pull ups generales
    
    //ESCRITOR
    IOCBbits.IOCB0 = 1;         // INTERRUPCIÓN PORTB activada
    WPUBbits.WPUB0 = 1;         // Activo pull up B0
    //SELECTOR
    IOCBbits.IOCB1 = 1;         // INTERRUPCIÓN PORTB activada
    WPUBbits.WPUB1 = 1;         // Activo pull up B1
    //INC
    IOCBbits.IOCB2 = 1;         // INTERRUPCIÓN PORTB activada
    WPUBbits.WPUB2 = 1;         // Activo pull up B2
    //DEC
    IOCBbits.IOCB3 = 1;         // INTERRUPCIÓN PORTB activada
    WPUBbits.WPUB3 = 1;         // Activo pull up B3
}

void writeTime(uint8_t dir, uint8_t d){
    I2C_Master_Start();
    I2C_Master_Write(0xD0);
    I2C_Master_Write(dir);
    I2C_Master_Write(d);
    I2C_Master_Stop();
    __delay_ms(10);
}