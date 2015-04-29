#include "Sprinkler.h"

Sprinkler::Sprinkler(uint8_t first_pin, uint8_t zone_count) {

    // TODO: error if first_pin or zone_count out of bounds
    _first_pin = first_pin;
    _zone_count = zone_count;

    for( uint8_t i=0; i<_zone_count; i++ )
        pinMode( _first_pin+i, OUTPUT );

    _queue = NULL;
}

bool Sprinkler::on( int8_t zone, unsigned long duration) {

    digitalWrite( _first_pin + zone, ZONE_ON );
    return true;

    // int8_t i = findFreeEventIndex();
    // if (i == -1) return -1;
    //
    // _events[i].eventType = EVENT_EVERY;
    // _events[i].period = period;
    // _events[i].repeatCount = repeatCount;
    // _events[i].callback = callback;
    // _events[i].lastEventTime = millis();
    // _events[i].count = 0;
    // return i;
}



