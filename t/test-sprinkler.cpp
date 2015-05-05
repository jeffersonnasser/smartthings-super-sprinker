#include "ArduinoTap/ArduinoTap.h"
#include "MockWProgram/MockWProgram.hpp"
#include "Sprinkler.h"

#define FIRST_ZONE_PIN 5
#define ZONE_COUNT     5
#define ZONE_ON        LOW
#define ZONE_OFF       HIGH

unsigned long current_time; // So we can mess with the clock
Sprinkler sprinkler( FIRST_ZONE_PIN, ZONE_COUNT );  // constructor

// Pre-declare helper/test functions
void advance_time_by_min( unsigned long minutes );
void advance_time_by_sec( unsigned long seconds );
void isZoneOn( uint8_t zone );
void areAllZonesOff( );

void setup() {
    Serial.begin( 9600 );
    current_time = millis();
}

void loop() {
    no_plan();

    // Can we turn a zone on?
    ok( sprinkler.on( 0, 10 ), "Turn zone 0 on for 10 minutes" );
    advance_time_by_min( 0 );
    isZoneOn( 0 );

    // Does it turn off?
    advance_time_by_min( 9 );
    isZoneOn( 0 );
    advance_time_by_min( 2 );
    areAllZonesOff();

    // Queue a couple zones
    ok( sprinkler.on( 1, 10 ), "Turn zone 1 on for 10 minutes" );
    ok( sprinkler.on( 3, 10 ), "Turn zone 3 on for 10 minutes" );
    ok( sprinkler.on( 5, 10 ), "Turn zone 5 on for 10 minutes" );
    advance_time_by_min( 1 );
    isZoneOn( 1 );
    // Do we advance?
    advance_time_by_min( 10 );
    advance_time_by_sec( 30 ); // stop/start on different updates
    isZoneOn( 3 );
    // What if we change duration
    ok( sprinkler.on( 3, 3 ), "Reduce zone 3 to 3 minutes" );
    advance_time_by_sec( 30 );
    isZoneOn( 3 );
    advance_time_by_min( 3 );
    advance_time_by_sec( 30 ); // stop/start on different updates
    isZoneOn( 5 );

    // Now turn off
    ok( sprinkler.off( 5 ), "Turn zone 5 off" );
    areAllZonesOff();
    advance_time_by_min( 1 );
    areAllZonesOff();

    // Turn all on at once
    diag( "Turn all zones on" );
    uint8_t durations[ZONE_COUNT];
    for( uint8_t i = 0; i < ZONE_COUNT; i++ ) durations[i] = i + 1;
    sprinkler.allOn( durations, ZONE_COUNT );
    advance_time_by_sec( 0 ); // 0 starts at 0
    isZoneOn( 0 );
    advance_time_by_sec( 90 ); // 0 stops at 1.5
    advance_time_by_sec( 0 ); // 1 starts at 1.5
    isZoneOn( 1 );
    advance_time_by_sec( 250 ); // 1 stops at 4
    advance_time_by_sec( 0 ); // 2 starts at 4
    isZoneOn( 2 );

    diag( "Advance to next zone" );
    sprinkler.advance();
    advance_time_by_sec( 10 ); // Give it a chances to stop
    advance_time_by_sec( 10 ); // Give the next a chance to start
    advance_time_by_sec( 10 ); // Give a change for something to go wrong
    isZoneOn( 3 );

    sprinkler.dump();

    diag( "Turn all zones off" );
    sprinkler.allOff();

    sprinkler.dump();

    advance_time_by_sec( 10 );
    areAllZonesOff();

    for( int i = 0; i < ZONE_COUNT; i++ ) {
        ZoneStatus status;
        sprinkler.status( 0, &status );
        is( status.on, false, "... zone is off" );
        is( status.queued, false, "... zone is not queued" );
    }

    // ok( !strcmp( status, "ok,off0,off1,off2,off3,off4,off5,off6,off7" ),
    // ok( sprinkler.pump(1), "Do stuff with pump");

    done_testing();
}

void advance_time_by_min( unsigned long minutes ) {
    advance_time_by_sec( minutes * 60L );
}

void advance_time_by_sec( unsigned long seconds ) {
    current_time = current_time + seconds * 1000L;
    fprintf( stderr, "# %d mintutes passed\n", int( current_time / 60 / 1000 ) );
    sprinkler.update( current_time );
}

void areAllZonesOff( ) {
    bool all_off = true;
    for( uint8_t i = 0; i < ZONE_COUNT; i++ ) {
        if( digital_pins[ FIRST_ZONE_PIN + i ] != ZONE_OFF ) {
            all_off = false;
            fprintf( stderr, "# pin %d is on\n", i );
        }
    }

    ok( all_off,  "... all zones are off" );
}

void isZoneOn( uint8_t zone ) {
    char desc[] = "... and zone 0x is on";
    desc[14] = zone % 9 + char( '0' );
    desc[13] = int( zone / 10 ) + char( '0' );

    is( digital_pins[ FIRST_ZONE_PIN + zone ], ZONE_ON, desc );

    bool all_off = true;
    for( uint8_t i = 0; i < ZONE_COUNT; i++ ) {
        if( i != zone && ( digital_pins[ FIRST_ZONE_PIN + i ] != ZONE_OFF ) ) {
            all_off = false;
            fprintf( stderr, "# pin %d is on\n", i );
        }
    }

    ok( all_off,  "... other zones are off" );
}

int main( int argc, char **argv ) {
    setup();
    loop();
}

