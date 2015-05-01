#include "Sprinkler.h"

#define FIRST_ZONE_PIN 5
#define ZONE_COUNT     8

Sprinkler sprinkler( FIRST_ZONE_PIN, ZONE_COUNT );  // constructor

void setup() {
    Serial.begin( 9600 );

    for( int i = 0; i < ZONE_COUNT; i++ )
        sprinkler.on( i, 1 );
}

void loop() {
    Serial.println( "update" );
    sprinkler.update();

    delay( 1 * 1000 ); // milliseconds
}



