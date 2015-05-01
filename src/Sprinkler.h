/*
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

/*  * * * * * * * * * * * * * * * * * * * * * * * * * * *
    Code by Mark Grimes
    http://www.github.com/mvgrimes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef Sprinkler_h
#define Sprinkler_h

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
// #include <math.h>
// #include <WString.h>

#ifdef _TEST_
#include "MockWProgram/MockWProgram.hpp"
#define __ASSERT_USE_STDERR
#include <stdio.h>
#include <string.h>
#else
// If version > 100?
#include "Arduino.h"
#endif

#define MAX_ZONES  24
#define ZONE_ON    LOW
#define ZONE_OFF   HIGH

typedef struct Zone {
    uint8_t zone;
    bool    queued;
    bool    on;
    unsigned long duration;
    unsigned long start_time;
    Zone *prior;
    Zone *next;
} Zone;

class Sprinkler {

  public:
    Sprinkler( uint8_t first_pin, uint8_t zone_count );

    bool on( int8_t zone_id, unsigned int duration_in_mins );
    bool off( int8_t zone_id );
    // bool pump( int8_t zone_id );
    void allOn( unsigned int *durations_in_mins, unsigned int size );
    void allOff( void );
    void advance( void );
    void status( char *str );

    bool update( void );
    bool update( unsigned long now );

  protected:
    uint8_t _first_pin;
    uint8_t _zone_count;

    Zone _zones[MAX_ZONES];
    Zone *_queue_head;
    Zone *_queue_tail;

    void enqueue( Zone * );
    Zone *dequeue();

    void startFlow( uint8_t zone_id );
    void stopFlow( uint8_t zone_id );
};

#endif







