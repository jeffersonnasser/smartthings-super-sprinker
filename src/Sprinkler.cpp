#include "Sprinkler.h"

Sprinkler::Sprinkler( uint8_t first_pin, uint8_t zone_count ) {
    // TODO: error if first_pin or zone_count out of bounds
    // TODO: shouldn't use pins > 12, uno flashes pin 13 at startup
    _first_pin = first_pin;
    _zone_count = zone_count;

    for( uint8_t i = 0; i < _zone_count; i++ ) {
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
bool Sprinkler::on( int8_t zone_id, unsigned int duration_in_mins ) {
    if( zone_id < 0 || zone_id > MAX_ZONES )
        return false;

    Zone *zone;
    zone = &_zones[zone_id];

    // If we are already in the queue, then just update the duration.
    // Otherwise add to the end of the queue.
    if( ! zone->queued ) enqueue( zone );

    zone->duration = duration_in_mins * 60L * 1000L; // stored in milliseconds
    fprintf( stderr, "# duration = %d mins = %lu millis\n", duration_in_mins,
             zone->duration );

    return true;
}

bool Sprinkler::off( int8_t zone_id ) {
    if( zone_id < 0 || zone_id > MAX_ZONES )
        return false;

    Zone *zone;
    zone = &_zones[zone_id];

    if( ! zone->queued ) return false;

    if( zone->on ) { // On, must be head of queue
        assert( _queue_head == zone );
        stopFlow( zone->zone );
        zone->on = false;
    }

    // TODO: combine this and dequeue
    // Remove from the queue
    zone->queued = false;
    if( zone->prior != NULL ) zone->prior->next = zone->next;
    if( _queue_head == zone ) _queue_head = zone->next;
    if( _queue_tail == zone ) _queue_tail = zone->prior;
    zone->prior = zone->next = NULL;

    return true;
}

void Sprinkler::allOn( unsigned int *durations_in_mins, unsigned int size ) {
    assert( size >= 0 && size < MAX_ZONES );

    for( uint8_t i = 0; i < size; i++ ) {
        fprintf( stderr, "# allOn calling on( %d, %d )\n", i, durations_in_mins[i] );
        on( i, durations_in_mins[i] );
    }
}

void Sprinkler::allOff( void ) {
    for( uint8_t i = 0; i < _zone_count; i++ ) off( i );
}

// bool Sprinkler::pump( int8_t zone_id ){}

void Sprinkler::advance( void ) {
    if( _queue_head != NULL ) off( _queue_head->zone );
}

#ifdef _TEST_
void itoa( unsigned int num, char *str, int radix ) {
    unsigned int size;
    size = ( num == 0  ? 0 : log( num ) / log( radix ) ) + 1;
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
void Sprinkler::status( char *status ) {
    Zone *zone;

    strcat( status, "ok" );
    for( uint8_t i = 0; i < _zone_count; i++ ) {
        zone = &_zones[i];
        if( zone->on ) strcat( status, ",on" );
        else if( zone->queued ) strcat( status, ",queued" );
        else strcat( status, ",off" );

        itoa( i, &status[ strlen( status ) ], 10 );
    }
}

void Sprinkler::enqueue( Zone *zone ) {
    zone->queued = true;
    zone->prior = _queue_tail;

    if( _queue_tail != NULL ) _queue_tail->next = zone;
    if( _queue_head == NULL ) _queue_head = zone;

    _queue_tail = zone;
}

Zone* Sprinkler::dequeue() {
    Zone *head;
    head = _queue_head;

    if( head != NULL ) {
        head->queued = false;

        _queue_head = head->next;
        if( _queue_head != NULL ) _queue_head->prior = NULL;

        fprintf( stderr, "# dequeue: now head is %d\n",
                 _queue_head == NULL ? -1 : _queue_head->zone );
    }

    return head;
}

bool Sprinkler::update( void ) {
    unsigned long now;
    now = millis();

    return update( now );
}

bool Sprinkler::update( unsigned long now ) {
    Zone *zone;
    zone = _queue_head;

    if( zone == NULL )  return false;

    assert( zone->queued );

    if( ! zone->on ) { // Start if it hasn't been started yet
        startFlow( zone->zone );
        zone->on = true;
        zone->start_time = now;
    } else { // Stop if the time is up

        fprintf( stderr, "# %lu - %lu = %lu  >= %lu\n", now, zone->start_time,
                 ( now - zone->start_time ), zone->duration );

        // Wrappinig of millis/now is ok. See: http://www.gammon.com.au/millis
        if( now - zone->start_time >= zone->duration  ) {
            stopFlow( zone->zone );
            zone->on = false;
            zone->start_time = 0;
            dequeue();
        }
    }

    return true;
}

void Sprinkler::startFlow( uint8_t zone_id ) {
    fprintf( stderr, "# startFlow: %d\n", zone_id );
    // Serial.print( "startFlow" );
    // Serial.println( _first_pin + zone_id );
    digitalWrite( _first_pin + zone_id, ZONE_ON );
}

void Sprinkler::stopFlow( uint8_t zone_id ) {
    fprintf( stderr, "# stopFlow: %d\n", zone_id );
    // Serial.print( "stopFlow" );
    // Serial.println( _first_pin + zone_id );
    digitalWrite( _first_pin + zone_id, ZONE_OFF );
}
