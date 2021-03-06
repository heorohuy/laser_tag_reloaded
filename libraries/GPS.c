/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2011, 2012 LeafLabs, LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/


#include "GPS.h"

#define GPS_BAUD 9600
#define GPS_USART USART1
#define GPS_PORT_TX GPIOA
#define GPS_PORT_RX GPIOA
#define GPS_PIN_TX 9
#define GPS_PIN_RX 10

#define GPS_VDD 8

#define NMEA_MAX_MESSAGE_LENGTH 82

//To consider: Implement Cold/Hot start functions?
//Idea for cold start, start and wait for coords or 30 seconds, whichever comes first.
//Might be better to separate wait to avoid blocking and allow startup for other components simultaneously.
void gps_start(void){
    usart_config_gpios_async(GPS_USART, GPS_PORT_RX, GPS_PIN_RX, GPS_PORT_TX, GPS_PIN_TX, 0);
    usart_init(GPS_USART);
    usart_set_baud_rate(GPS_USART, USART_USE_PCLK, GPS_BAUD);

    //Enable BJT to turn on GPS
    //TODO: Confirm this isn't affected by being on UART CLK Pin (Currently unused)
    gpio_set_mode(GPIOA, 1, GPIO_OUTPUT_PP);
    gpio_write_bit(GPIOA, 8, 1);
    
    //Enable GPS USART Port
    usart_enable(GPS_USART);
}

void gps_end(void){
    usart_disable(GPS_USART);
    gpio_write_bit(GPIOA, 8, 0);
}

uint32 gps_available(void){
    return usart_data_available(GPS_USART);
}

uint8 gps_read(void){
    while(!gps_available());
    return usart_getc(GPS_USART);
}

//Read GPS string up to the specified length or the end of that string, whichever comes first.
//Returns the amount of characters read from the GPS String
int gps_readString(char *str, int length){
    char c = ' ';
    int i = 0;
    while(c != '\n' && i < length-1)
    {
        c = gps_read();
        str[i++] = c;
    }
    return i;
}

//Return 0 if string is invalid size
//Return 1 if valid string is sent
uint8 gps_readMessage(char *str, int length){
    if(length < NMEA_MAX_MESSAGE_LENGTH+1)
    {
        return 0;
    }
    char c = ' ';
    int i = 0;
    while(gps_read() != '$');
    while(c != '\n' && i < NMEA_MAX_MESSAGE_LENGTH-1)
    {
        c = gps_read();
        str[i++] = c;
    }
    return 1;
}

void gps_write(unsigned char ch){
    usart_putc(GPS_USART, ch);
}

//Get GPS coordinates
//Currently using an SW-side loop to determine timeout, may want to use timers in the future
//Returns 0 if location was not found or if the incoming string's length is of invalid size (Should I differentiate these?)
//Returns 1 if location was found
//The resulting string will be formatted as xxmm.dddd,<N|S>,yyymm.dddd,<E|W>
int gps_getLocation(char *str, int length){
    //If the string's length is not 12 (+1 for string end character), return 0
    if(length != 12+1)
        return 0;

    gps_start();
    //Wait for the GPS to get a fix if available, if it does not after x time, return 0
    //TODO: Make this a timer-based implementation
    waitCount = 0;
    while(gps_hasFix() == 0 && waitCount < 20)
        waitCount++;
    if(waitCount == 20)
        return 0;


    //Find the string that contains the coordinates from the GPS output
    char str[6] = "     ";
    while(1){
        if(gps_read() == '$'){
            gps_readString(str, 6);
            if(strcmp(str, "GPRMC") == 0){
                break;
            }
        }
    }

    //Scan the string until the beginning of the coordinates portion
    int i;
    for(i=0; i < 14; i++){
        gps_read();
    }

    //Read the coordinates, store the in str and return 1
    gps_readString(str, 12);
    gps_end();
    return 1;
}

uint8 gps_hasFix(void){
    char str[6] = "     ";
    while(1){
        if(gps_read() == '$'){
            gps_readString(str, 6);
            if(strcmp(str, "GPRMC") == 0){
                break;
            }
        }
    }
    //Check for new GPS line, then check for GPRMC, then check for valid fix flag, if valid true, if invalid false
    int i;
    for(i=0; i < 12; i++){
        gps_read();
    }
    char flag = gps_read();
    if(flag == 'V')
        return 0;
    else
        return 1;
    //TODO: Do I want to read the rest of the message to clear it from buffer?
}