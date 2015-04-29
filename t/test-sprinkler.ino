#include <ArduinoTap.h>
#include <MockWProgram.hpp>
#include <Sprinkler.h>

#define FIRST_ZONE_PIN 6
#define ZONE_COUNT     8
#define ZONE_ON        HIGH
#define ZONE_OFF       LOW

Sprinkler sprinkler( FIRST_ZONE_PIN, ZONE_COUNT );  // constructor

void setup() {
    Serial.begin(9600);
}

void loop() {
    no_plan();

    // for( uint8_t i=0; i<=ZONE_COUNT; i++ )
    //     is(pinState(FIRST_ZONE_PIN+i,OUTPUT), "Pins are set to OUTPUT");

    ok( sprinkler.on(0,10), "Turn zone 1 on for 10 minutes" );
    is( digital_pins[ 0 ], ZONE_ON, "... and zone 1 is on");
    for( uint8_t i=1; i<=ZONE_COUNT; i++ )
        is( digital_pins[ FIRST_ZONE_PIN+i ], ZONE_OFF, "... other zones are off");

    // ok( sprinkler.on(2,10), "Turn zone 2 on for 10 minutes" );
    // ok( sprinkler.off(1), "Turn zone 1 off" );
    // ok( sprinkler.on(null,10), "Turn all zones on for 10 minutes" );
    // ok( sprinkler.off(null), "Turn all zones off" );
    // ok( sprinkler.update(), "???" );
    // // ok( sprinkler.pump(1), "Do stuff with pump");
    // ok( sprinkler.advance(), "Advance to next zone");

    done_testing();
}






