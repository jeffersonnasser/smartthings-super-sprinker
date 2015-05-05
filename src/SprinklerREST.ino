/* RESTSprinkler.ino -- REST API for Sprinkler */

/*
 *
 * EthernetShield
 *   communicates over SPI bus:
 *     uno: pins 11,12,13
 *     mega: pins 50,51,52,53
 *   selects the W5100 via pin 10
 *   selects the sdcard via pin 4

 * Move string into CODE using F():
    Added in version 1.0
    http://playground.arduino.cc/Learning/Memory
 */

#define WEBDUINO_FAIL_MESSAGE "<h1>Request Failed</h1>"
// #define WEBDUINO_SERIAL_DEBUGGING true
#include "SPI.h" // new include
#include "avr/pgmspace.h" // new include
#include "Ethernet.h"
#include "WebServer.h"
#include "Sprinkler.h"
#include "Timer.h"

#define VERSION_STRING "0.2"
#define FIRST_ZONE_PIN 5
#define ZONE_COUNT     5
#define MAX_DURATION   60
#define ZONE_ON        LOW
#define ZONE_OFF       HIGH
#define PARAM_SIZE     16
#define VALUE_SIZE     16
#define TIMER_PERIOD   2000

// no-cost stream operator as described at
// http://arduiniana.org/libraries/streaming/
// http://playground.arduino.cc/Main/StreamingOutput
template<class T>
inline Print &operator <<( Print &obj, T arg ) {
    obj.print( arg );
    return obj;
}

// CHANGE THIS TO YOUR OWN UNIQUE VALUE
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xAE, 0xED };

// CHANGE THIS TO MATCH YOUR HOST NETWORK
static uint8_t ip[] = { 192, 168, 1, 210 };

#define PREFIX ""

WebServer webserver( PREFIX, 80 );
Sprinkler sprinkler( FIRST_ZONE_PIN, ZONE_COUNT );  // constructor
Timer t;

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server
const __FlashStringHelper * _okMsg() {
    return F( "status: 'success', " );
}
const __FlashStringHelper * _errMsg() {
    return F( "status: 'failure', " );
}

void zoneStatus( WebServer &server, uint8_t zone_id ) {
    ZoneStatus zone;

    if( ! sprinkler.status( zone_id, &zone ) ) {
        server << "{" << _errMsg() << ", ";
        server << F( " 'zone': " ) << zone.zone;
        server << F( " 'message': 'error returned by status()' " );
        server << "}\n";
        return;
    }

    server << "{" << _okMsg() << ", ";
    server << " 'zone': " << zone.zone << ",";
    server << " 'on': " << ( zone.on ? "true" : "false" ) << ",";
    server << " 'queued': " << ( zone.queued ? "true" : "false" ) << ",";
    server << " 'duration': " << zone.duration << ",";
    server << " 'secs_left': " << zone.secs_left;
    server << " }\n";
}

void zonesStatus( WebServer &server ) {
    server << "[\n";
    for( uint8_t i = 0; i < ZONE_COUNT; i++ ) {
        server << "    ";
        zoneStatus( server, i );
    }
    server << "]\n";
}


void zoneUpdate( WebServer &server, uint8_t zone_id ) {
    char name[PARAM_SIZE], value[VALUE_SIZE];

    unsigned long duration = 0;
    int8_t zone = -1;
    bool on;
    bool on_set = false;

    while( server.readPOSTparam( name, PARAM_SIZE, value, VALUE_SIZE ) ) {
        if( strcmp( name, "zone" ) == 0 )
            zone = atoi( value );
        if( strcmp( name, "duration" ) == 0 )
            duration = atoi( value );
        if( strcmp( name, "on" ) == 0 ) {
            if( strcasecmp( value, "true" ) == 0  ) {
                on = true;
                on_set = true;
            } else if( strcasecmp( value, "false" ) == 0  ) {
                on = false;
                on_set = true;
            }
        }
        // Serial << name << " = " << value << "\r\n";
        // Serial.print( name );
        // Serial.print( " = " );
        // Serial.println( value );
    }

    if( ! on_set ) {
        server.httpSuccess( "application/json" );
        server << "{" << _errMsg() << ", ";
        server << F( "message: 'on must be set to true or false' }\n" );
        return;
    }
    if( zone < 0 ) zone = zone_id;
    if( zone != zone_id ) {
        server.httpSuccess( "application/json" );
        server << "{" << _errMsg() << ", ";
        server << F( "message: 'zone in URL and param differ' }\n" );
        return;
    }
    if( zone < 0 || zone >= ZONE_COUNT ) {
        server.httpSuccess( "application/json" );
        server << "{" << _errMsg() << ", ";
        server << F( "message: 'zone out of range' }\n" );
        return;
    }
    if( on && duration > 0 ) {
        if( sprinkler.on( zone, duration ) ) {
            server.httpSuccess( "application/json" );
            server << "{ success: 'zone " << zone << " queued for " << duration << "' }\n";
            return;
        } else {
            server.httpSuccess( "application/json" );
            server << "{" << _errMsg() << ", ";
            server << F( "message: 'error running sprinkler.on()' }\n" );
            return;
        }
    }
    if( !on ) {
        if( sprinkler.off( zone ) ) {
            // server.httpSeeOther(PREFIX "/form");
            server.httpSuccess( "application/json" );
            server << "{" << _okMsg() << ", ";
            server << F( "message: 'zone turned off' }\n" );
            return;
        } else {
            server.httpSuccess( "application/json" );
            server << "{" << _errMsg() << ", ";
            server << F( "message: 'error running sprinkler.off()' }\n" );
            return;
        }
    }

    server.httpSuccess( "application/json" );
    server << F( "{ message: 'nothing to do' }\n" );
}

void do_advance( WebServer &server ) {
    sprinkler.advance();
    server.httpSuccess( "application/json" );
    server << "{" << _okMsg() << ", ";
    server << F( "message: 'sprinkler advanced' }\n" );
    return;
}

void do_multi_zone_on(  WebServer &server, uint8_t *zone, uint8_t zone_cnt,
                        uint8_t *duration, uint8_t duration_cnt ) {

    if( zone_cnt == 0 ) {
        server.httpSuccess( "application/json" );
        server << "{" << _errMsg() << ", ";
        server << F( "message: 'need to pass the zones to turn on in zone[]' }\n" );
        return;
    }
    if( zone_cnt != duration_cnt ) {
        server.httpSuccess( "application/json" );
        server << "{" << _errMsg() << ", ";
        server << F( "message: 'need to pass the durations for each zone in duration[]' }\n" );
        return;
    }
    for( uint8_t i = 0; i < zone_cnt; i++ ) {
        if( zone[i] < 0 || zone[i] >= ZONE_COUNT ) {
            server.httpSuccess( "application/json" );
            server << "{" << _errMsg() << ", ";
            server << F( "message: 'zone in zone[] out of range' }\n" );
            return;
        }
        if( duration[i] < 0 || duration[i] > MAX_DURATION ) {
            server.httpSuccess( "application/json" );
            server << "{" << _errMsg() << ", ";
            server << F( "message: 'duration out of range' }\n" );
            return;
        }
        sprinkler.on( zone[i], duration[i] );
    }
    server.httpSuccess( "application/json" );
    server << F( "{ message: 'zones successfully scheduled' }\n" );
    return;
}

void do_multi_zone_off( WebServer &server, uint8_t *zone, uint8_t zone_cnt ) {

    if( zone_cnt == 0 ) {
        sprinkler.allOff();
        server.httpSuccess( "application/json" );
        server << "{" << _okMsg() << ", ";
        server << F( "message: 'all zones turned off' }\n" );
        return;
    } else {
        for( uint8_t i = 0; i < zone_cnt; i++ )
            sprinkler.off( zone[i] );

        server.httpSuccess( "application/json" );
        server << "{" << _okMsg() << ", ";
        server << F( "message: 'zones turned off' }\n" );
        return;
    }

}


// on = true -> need zone[] and duration[]
// on = false -> zone[] or allOff
void zonesUpdate( WebServer & server ) {
    char name[PARAM_SIZE], value[VALUE_SIZE];

    bool on = false;
    bool off = false;
    bool advance = false;
    uint8_t zone[ZONE_COUNT];
    uint8_t duration[ZONE_COUNT];
    uint8_t zone_cnt = 0;
    uint8_t duration_cnt = 0;

    while( server.readPOSTparam( name, PARAM_SIZE, value, VALUE_SIZE ) ) {
        if( strcmp( name, "zone[]" ) == 0 )
            zone[zone_cnt++] = atoi( value );

        if( strcmp( name, "duration[]" ) == 0 )
            duration[duration_cnt++] = atoi( value );

        if( strcmp( name, "on" ) == 0 ) {
            if( strcasecmp( value, "true" ) == 0  ) on = true;
            else if( strcasecmp( value, "false" ) == 0  )  off = true;
        }

        if( strcmp( name, "zone" ) == 0 && strcmp( value, "next" ) == 0 )
            advance = true;

        // Stop processing if too many
        if( zone_cnt >= ZONE_COUNT || duration_cnt >= ZONE_COUNT ) {
            server.httpSuccess( "application/json" );
            server << "{" << _errMsg() << ", ";
            server << F( "message: 'too many zone[] or duration[] entries' }\n" );
            return;
        }
    }

    if( advance ) return do_advance( server );
    else if( on ) return do_multi_zone_on( server, zone, zone_cnt,
                                               duration, duration_cnt );
    else if( off ) return do_multi_zone_off( server, zone, zone_cnt );

    server.httpSuccess( "application/json" );
    server << "{" << _errMsg() << ", ";
    server << F( "message: 'on needs to be set to true or false' }\n" );
    return;
}
void zoneCmd( WebServer & server, WebServer::ConnectionType type,
              char **url_path, char *url_tail, bool tail_complete ) {

    if( !tail_complete ) {
        server.httpNoContent();
        return;
    }

    if( strcmp( url_path[0], "zone" ) != 0 ) {
        server.httpNoContent();
        return;
    }

    uint8_t zone_id = atoi( url_path[1] );

    // server << "Type = " << url_path[0] << "\n";
    // server << "Zone = " << url_path[1] << " == " << zone_id << "\n";

    if ( type == WebServer::POST || type == WebServer::PUT ) {
        zoneUpdate( server, zone_id );
        return;
    }

    server.httpSuccess( "application/json" );

    if ( type == WebServer::HEAD ) return;

    zoneStatus( server, zone_id );
}

void zonesCmd( WebServer & server, WebServer::ConnectionType type,
               char *url_tail, bool tail_complete ) {

    if ( type == WebServer::POST || type == WebServer::PUT ) {
        zonesUpdate( server );
        return;
    }

    server.httpSuccess( "application/json" );

    if ( type == WebServer::HEAD ) return;

    zonesStatus( server );
}

void debugCmd( WebServer & server, WebServer::ConnectionType type,
               char *url_tail, bool tail_complete ) {
    P( htmlHead ) =
        "<html><head>\n"
        "<title>Arduino Sprinkler Server</title>\n"
        "</head>"
        "<body>";

    server.httpSuccess();
    server.printP( htmlHead );

    server << F( "<h1>Digital Pins</h1><p>\n" );

    // ignore the pins we use to talk to the Ethernet chip
    for ( uint8_t i = 2; i < 10; ++i ) {
        if( i == 4 ) continue;
        uint8_t val = digitalRead( i );
        server << "Digital " << i << ": ";
        server << ( val ? "HIGH" : "LOW" );
        server << "<br/>\n";
    }

    server << F( "</body></html>" );
}

void defaultCmd( WebServer & server, WebServer::ConnectionType type,
                 char *url_tail, bool tail_complete ) {
    P( htmlHead ) =
        "<html>"
        "<head>"
        "<title>Arduino Sprinkler System</title>"
        "</head>"
        "<body>";

    server.httpSuccess();
    server.printP( htmlHead );
    server << F( "Welcome" );
    server << F( "</body></html>" );
}

void do_sprinkler_update( void ) {
    sprinkler.update();
}

unsigned long last_update;
void setup() {
    // Serial.begin( 9600 );
    // Serial.println( "beginning setup" );

    Ethernet.begin( mac, ip );
    webserver.begin();

    webserver.setDefaultCommand( &defaultCmd );
    webserver.addCommand( "zones", &zonesCmd );
    webserver.addCommand( "debug", &debugCmd );
    webserver.setUrlPathCommand( &zoneCmd );

    t.every( TIMER_PERIOD, do_sprinkler_update );
    // Serial.println( "completed setup" );
}

void loop() {
    // process incoming connections one at a time forever
    webserver.processConnection();

    // do stuff!
    t.update();
}
