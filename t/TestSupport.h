#ifndef TestSupport_h
#define TestSupport_h

#include "ArduinoTap/ArduinoTap.h"
#include "Sprinkler.h"
#include <String>

void isZoneOn( uint8_t zone ) {
    char desc[] = "... and zone 0x is on";
    desc[14] = zone + char('0');

    is( digital_pins[ FIRST_ZONE_PIN + zone ], ZONE_ON, desc );

    bool all_off = true;
    for( uint8_t i = 0; i <= ZONE_COUNT && all_off; i++ ) {
        if( i <> zone ) all_off = digital_pins[ FIRST_ZONE_PIN + i ] == ZONE_OFF;
    }

    ok( all_off,  "... other zones are off" );
}

#endif



