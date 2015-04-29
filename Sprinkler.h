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

// #ifdef _TESTING
#include <MockWProgram.hpp>
// #endif

#define MAX_ZONES 8
#define ZONE_ON   HIGH
#define ZONE_OFF  LOW


typedef struct ZoneNode {
    uint8_t id;
    uint8_t duration;
    bool    on;
    ZoneNode *next;
} ZoneNode;


class Sprinkler {

  public:
    Sprinkler(uint8_t first_pin, uint8_t zone_count);

    bool on(int8_t zone, unsigned long duration);
    bool off(int8_t zone);
    bool update(void);
    bool pump(int8_t zone);
    bool advance(void);

  protected:
    uint8_t _first_pin;
    uint8_t _zone_count;

    ZoneNode *_queue;
    // _zones[MAX_ZONES];
    // int8_t findFreeEventIndex(void);
};

#endif





