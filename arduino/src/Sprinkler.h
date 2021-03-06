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

#ifdef _TEST_
#define __ASSERT_USE_STDERR
#include "MockWProgram/MockWProgram.hpp"
#include <stdio.h>
#include <string.h>
// math.h produces an error in the definition of round
// would like to use to get def'n of log
// #include <math.h>
#else
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
// Compile out the print statements to save mem
#define fprintf(...)   true
#endif

#define SPRINKLER_MAX_ZONES    24
#define SPRINKLER_MAX_DURATION 60
#define ZONE_ON      LOW
#define ZONE_OFF     HIGH

typedef struct Zone {
    uint8_t zone;
    uint8_t pin;
    bool    queued;
    bool    on;
    unsigned long duration;
    unsigned long start_time;
    Zone *prior;
    Zone *next;
} Zone;

typedef struct ZoneStatus {
    uint8_t zone;
    uint8_t pin;
    bool    queued;
    bool    on;
    unsigned long duration;
    unsigned long secs_left;
} ZoneStatus;

class Sprinkler {

  public:
    Sprinkler( uint8_t *pins, uint8_t zone_count );

    bool on( uint8_t zone_id, uint8_t duration_in_mins );
    bool off( uint8_t zone_id );
    // bool pump( uint8_t zone_id );
    void allOn( uint8_t *durations_in_mins, uint8_t size );
    void allOff( void );
    void advance( void );
    bool status( uint8_t zone_id, ZoneStatus *status );
    void dump( void );

    bool update( void );
    bool update( unsigned long now );

  protected:
    uint8_t _zone_count;

    Zone _zones[SPRINKLER_MAX_ZONES];
    Zone *_queue_head;
    Zone *_queue_tail;

    void enqueue( Zone * );
    void dequeue( Zone * );

    void startFlow( uint8_t zone_id );
    void stopFlow( uint8_t zone_id );
};

#endif
