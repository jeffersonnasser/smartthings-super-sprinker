#include "Sprinkler.h"

Sprinkler::Sprinkler( uint8_t first_pin, uint8_t zone_count ) {
    // TODO: error if first_pin or zone_count out of bounds
    // TODO: shouldn't use pins > 12, uno flashes pin 13 at startup
    // TODO: shouldn't use pins < 3, uno uses for serial comm
    _first_pin = first_pin;
    _zone_count = zone_count;

    Serial.print( "constructor: _first_pin = " );
    Serial.print( _first_pin );
    Serial.print( ", _zone_count = " );
    Serial.println(  _zone_count );

    for( uint8_t i = 0; i < _zone_count; i++ ) {
        // Serial << "init: pin " << (  _first_pin + i ) << "\n";
        // Serial.print( "init: pin " );
        // Serial.println(  _first_pin + i );
        pinMode( _first_pin + i, OUTPUT );
        digitalWrite( _first_pin + i, ZONE_OFF );
    }

    _queue_head = NULL;
    _queue_tail = NULL;

    for( uint8_t i = 0; i < MAX_ZONES; i++ ) {
        _zones[i].zone = i;
        _zones[i].on = false;
        _zones[i].queued = false;
        _zones[i].duration = 0;
        _zones[i].start_time = 0;
        _zones[i].next = NULL;
    }
}

// If zone is already queued or on, this will update the duration
bool Sprinkler::on( uint8_t zone_id, unsigned int duration_in_mins ) {
    if( zone_id < 0 || zone_id > MAX_ZONES ) return false;
    if( ! ( duration_in_mins > 0 ) ) return false;

    Zone *zone = &_zones[zone_id];

    // If we are already in the queue, then just update the duration.
    // Otherwise add to the end of the queue.
    if( ! zone->queued ) enqueue( zone );

    if( duration_in_mins > MAX_DURATION ) duration_in_mins = MAX_DURATION;
    zone->duration = duration_in_mins * 60L * 1000L; // stored in milliseconds
    // fprintf( stderr, "# duration = %d mins = %lu millis\n", duration_in_mins,
    //          zone->duration );

    return true;
}

bool Sprinkler::off( uint8_t zone_id ) {
    if( zone_id < 0 || zone_id > MAX_ZONES )
        return false;

    Zone *zone = &_zones[zone_id];

    if( ! zone->queued ) return false;

    if( zone->on ) { // On, must be head of queue
        assert( _queue_head == zone );
        stopFlow( zone->zone );
        zone->on = false;
    }

    dequeue( zone );

    return true;
}

void Sprinkler::allOn( unsigned int *durations_in_mins, unsigned int size ) {
    assert( size >= 0 && size < MAX_ZONES );
    for( uint8_t i = 0; i < size; i++ ) on( i, durations_in_mins[i] );
}

void Sprinkler::allOff( void ) {
    for( uint8_t i = 0; i < _zone_count; i++ ) off( i );
}

// bool Sprinkler::pump( uint8_t zone_id ){}

void Sprinkler::advance( void ) {
    if( _queue_head != NULL ) off( _queue_head->zone );
}

#ifdef _TEST_
// KLUDGE: This is available under Arduino, but not during testing.
// This is not a good implementation.
void itoa( unsigned int num, char *str, int radix ) {
    assert( num < 1001 );
    assert( radix == 10 );

    unsigned int size;
    // math.h produces an error in the definition of round, can't use log()
    // size = ( num == 0  ? 0 : log( num ) / log( radix ) ) + 1;
    size = num < 10 ? 1 : num < 100 ? 2 : 3; // we know it won't be this big
    // fprintf( stderr, "# %d size is %d\n", num, size );

    str[ size-- ] = '\0';
    do {
        str[ size ] = int( num % radix ) + char( '0' );
        num = int( num / radix );
        // fprintf( stderr, "# str: %s\n", str );
    } while( size-- );
}
#endif

// status should start with ok
bool Sprinkler::status( uint8_t zone_id, ZoneStatus *status ) {
    if( zone_id < 0 || zone_id >= _zone_count ) return false;

    unsigned long now = millis();

    Zone *zone = &_zones[zone_id];
    status->zone = zone->zone;
    status->on = zone->on;
    status->queued = zone->queued;
    status->duration = zone->duration / ( 1000L * 60L );
    status->secs_left = zone->on
                        ?  (  ( zone->duration - ( now - zone->start_time ) )
                              / 1000L )
                        : 0;

    return true;
}

void Sprinkler::dump( void ) {
    fprintf( stderr, "# %6s: ", "zone" );
    for( uint8_t i = 0; i < _zone_count; i++ )
        fprintf( stderr, "% 4d", _zones[i].zone );
    fprintf( stderr, "\n" );

    fprintf( stderr, "# %6s: ", "on" );
    for( uint8_t i = 0; i < _zone_count; i++ )
        fprintf( stderr, "% 4d", _zones[i].on );
    fprintf( stderr, "\n" );

    fprintf( stderr, "# %6s: ", "queued" );
    for( uint8_t i = 0; i < _zone_count; i++ )
        fprintf( stderr, "% 4d", _zones[i].queued );
    fprintf( stderr, "\n" );

    fprintf( stderr, "# %6s: ", "head" );
    for( uint8_t i = 0; i < _zone_count; i++ )
        fprintf( stderr, "%4s", _queue_head == &_zones[i] ? "^" : "" );
    fprintf( stderr, "\n" );

    fprintf( stderr, "# %6s: ", "tail" );
    for( uint8_t i = 0; i < _zone_count; i++ )
        fprintf( stderr, "%4s", _queue_tail == &_zones[i] ? "^" : "" );
    fprintf( stderr, "\n" );

    fprintf( stderr, "# %6s: ", "next" );
    for( uint8_t i = 0; i < _zone_count; i++ )
        fprintf( stderr, "% 4d", _zones[i].next == NULL ? -1 : _zones[i].next->zone );
    fprintf( stderr, "\n" );

    fprintf( stderr, "# %6s: ", "prior" );
    for( uint8_t i = 0; i < _zone_count; i++ )
        fprintf( stderr, "% 4d", _zones[i].prior == NULL ? -1 : _zones[i].prior->zone );
    fprintf( stderr, "\n" );
}

void Sprinkler::enqueue( Zone *zone ) {
    zone->queued = true;
    zone->prior = _queue_tail;
    if( _queue_tail != NULL ) _queue_tail->next = zone;
    if( _queue_head == NULL ) _queue_head = zone;
    _queue_tail = zone;
}

void Sprinkler::dequeue( Zone *zone ) {
    zone->queued = false;
    if( zone->prior != NULL ) zone->prior->next = zone->next;
    if( zone->next != NULL ) zone->next->prior = zone->prior;
    if( _queue_head == zone ) _queue_head = zone->next;
    if( _queue_tail == zone ) _queue_tail = zone->prior;
    zone->prior = zone->next = NULL;
}

bool Sprinkler::update( void ) {
    unsigned long now = millis();
    return update( now );
}

bool Sprinkler::update( unsigned long now ) {
    Zone *zone = _queue_head;

    if( zone == NULL )  return false;

    assert( zone->queued );

    if( ! zone->on ) { // Start if it hasn't been started yet
        startFlow( zone->zone );
        zone->on = true;
        zone->start_time = now;
    } else { // Stop if the time is up

        // fprintf( stderr, "# %lu - %lu = %lu  >= %lu\n", now, zone->start_time,
        //          ( now - zone->start_time ), zone->duration );

        // Wrappinig of millis/now is ok. See: http://www.gammon.com.au/millis
        if( now - zone->start_time >= zone->duration  ) {
            stopFlow( zone->zone );
            zone->on = false;
            zone->start_time = 0;
            dequeue( zone );
        }
    }

    return true;
}

void Sprinkler::startFlow( uint8_t zone_id ) {
    fprintf( stderr, "# startFlow: %d\n", zone_id );
    // Serial << "startFlow: pin " << (  _first_pin + zone_id ) << "\n";
    // Serial.print( "startFlow: pin " );
    // Serial.println(  _first_pin + zone_id );
    digitalWrite( _first_pin + zone_id, ZONE_ON );
}

void Sprinkler::stopFlow( uint8_t zone_id ) {
    fprintf( stderr, "# stopFlow: %d\n", zone_id );
    // Serial << "stopFlow: pin " << (  _first_pin + zone_id ) << "\n";
    // Serial.print( "stopFlow: pin " );
    // Serial.println(  _first_pin + zone_id );
    digitalWrite( _first_pin + zone_id, ZONE_OFF );
}

