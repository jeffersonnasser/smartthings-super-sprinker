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
#define PARAM_SIZE     16
#define VALUE_SIZE     16

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

#define ZONE_COUNT     8
#define MAX_DURATION   60
#define TIMER_PERIOD   5000
uint8_t zone_pins[ZONE_COUNT] = { 2, 3, 5, 6, 7, 8, 9, A0 };
Sprinkler sprinkler( zone_pins, ZONE_COUNT );  // constructor

Timer t;

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server
const __FlashStringHelper * _okMsg() {
    return F( "\"status\": \"success\"" );
}
const __FlashStringHelper * _errMsg() {
    return F( "\"status\": \"failure\"" );
}

bool _zoneStatus( WebServer &server, uint8_t zone_id ) {
    ZoneStatus zone;

    server << F( "{" );

    if( ! sprinkler.status( zone_id, &zone ) ) {
        server << F( " \"zone\": " ) << zone_id;
        server << F( "}" );
        return false;
    }

    server << F( " \"zone\": " ) << zone.zone << F( "," );
    server << F( " \"on\": " ) << ( zone.on ? F( "true" ) : F( "false" ) ) <<
           F( "," );
    server << F( " \"queued\": " ) << ( zone.queued ? F( "true" ) : F( "false" ) )
           << F( "," );
    server << F( " \"duration\": " ) << zone.duration << F( "," );
    server << F( " \"secs_left\": " ) << zone.secs_left;
    server << F( "}" );

    return true;
}

void zoneStatus( WebServer &server, uint8_t zone_id ) {
    server << F( "{ \"zones\": [ " );
    if( _zoneStatus( server, zone_id ) ) {
        server << F( " ],\n" ) << _okMsg() << F( ", " );
        server << F( "\"message\":\"status returned ok\"\n" );
    } else {
        server << F( " ],\n" ) << _errMsg() << F( ", " );
        server << F( "\"message\":\"error returned by zone.status()\"\n" );
    }
    server << F( " }\n" );
}

bool _zonesStatus( WebServer &server ) {
    bool ok = true;

    server << F( " \"zones\": [\n" );
    for( uint8_t i = 0; i < ZONE_COUNT; i++ ) {
        if( _zoneStatus( server, i ) ) ok = false;
        if( i < ZONE_COUNT - 1 ) server << F( ",\n" );
    }
    server << F( "],\n" );

    return ok;
}

void zonesStatus( WebServer &server ) {
    server << F( "{\n" );
    if( !_zonesStatus( server ) ) {
        server << _okMsg() << F( ", " );
        server << F( "\"message\": \"statuses returned ok\"\n" );
    } else {
        server << _errMsg() << F( ",\n" );
        server << F( "\"message\": \"an error returned by on of the zone.status()\n" );
    }
    server << F( " }\n" );
}

void zoneUpdate( WebServer &server, uint8_t zone_id ) {
    char name[PARAM_SIZE], value[VALUE_SIZE];

    unsigned long duration = 0;
    int8_t zone = -1;
    bool on = false;
    bool off = false;

    while( server.readPOSTparam( name, PARAM_SIZE, value, VALUE_SIZE ) ) {
        if( strcmp( name, "zone" ) == 0 )
            zone = atoi( value );
        if( strcmp( name, "duration" ) == 0 )
            duration = atoi( value );
        if( strcmp( name, "on" ) == 0 ) {
            if( isTruey( value ) ) on = true;
            else if( isFalsey( value ) ) off = true;
        }
        // Serial << name << " = " << value << "\r\n";
        Serial.print( name );
        Serial.print( F( " = " ) );
        Serial.println( value );
    }

    if( ! ( on || off ) ) {
        server.httpSuccess( "application/json" );
        server << F( " {" ) << _errMsg() << F( ", " );
        server << F( "\"message\": \"on must be set to true or false\" }\n" );
        return;
    }
    if( zone < 0 ) zone = zone_id;
    if( zone != zone_id ) {
        server.httpSuccess( "application/json" );
        server << F( "{" ) << _errMsg() << F( ", " );
        server << F( "\"message\": \"zone in URL and param differ\" }\n" );
        return;
    }
    if( zone < 0 || zone >= ZONE_COUNT ) {
        server.httpSuccess( "application/json" );
        server << F( "{" ) << _errMsg() << F( ", " );
        server << F( "\"message\": \"zone out of range\" }\n" );
        return;
    }
    if( on && duration > 0 ) {
        if( sprinkler.on( zone, duration ) ) {
            server.httpSuccess( "application/json" );
            server << F( "{\n" );
            _zonesStatus( server ); // Check return value?
            server << _okMsg() << ", ";
            server << F( " \"message\": " )
                   << F( "\"zone " ) << zone << F( " queued for " ) << duration
                   << F( "\" }\n" );
            return;
        } else {
            server.httpSuccess( "application/json" );
            server << F( "{" ) << _errMsg() << F( ", " );
            server << F( "\"message\": \"error running sprinkler.on()\" }\n" );
            return;
        }
    }
    if( off ) {
        if( sprinkler.off( zone ) ) {
            // server.httpSeeOther(PREFIX "/form");
            server.httpSuccess( "application/json" );
            server << F( "{\n" );
            _zonesStatus( server ); // Check return value?
            server << _okMsg() << F( ", " );
            server << F( "\"message\": \"zone turned off\" }\n" );
            return;
        } else {
            server.httpSuccess( "application/json" );
            server << F( "{" ) << _errMsg() << F( ", " );
            server << F( "\"message\": \"error running sprinkler.off()\" }\n" );
            return;
        }
    }

    server.httpSuccess( "application/json" );
    server << F( "{ \"message\": \"nothing to do\" }\n" );
}

void do_advance( WebServer &server ) {
    sprinkler.advance();
    server.httpSuccess( "application/json" );
    server << F( "{" ) << _okMsg() << F( ", " );
    server << F( "\"message\": \"sprinkler advanced\" }\n" );
    return;
}

void do_multi_zone_on(  WebServer &server, uint8_t *zone, uint8_t zone_cnt,
                        uint8_t *duration, uint8_t duration_cnt ) {

    if( zone_cnt == 0 ) {
        server.httpSuccess( "application/json" );
        server << F( "{" ) << _errMsg() << F( ", " );
        server << F( "\"message\": \"need to pass the zones to turn on in zone[]\" }\n" );
        return;
    }
    if( zone_cnt != duration_cnt ) {
        server.httpSuccess( "application/json" );
        server << F( "{" ) << _errMsg() << F( ", " );
        server << F( "\"message\": \"need to pass the durations for each zone in duration[]\" }\n" );
        return;
    }
    for( uint8_t i = 0; i < zone_cnt; i++ ) {
        if( zone[i] < 0 || zone[i] >= ZONE_COUNT ) {
            server.httpSuccess( "application/json" );
            server << F( "{" ) << _errMsg() << F( ", " );
            server << F( "\"message\": \"zone in zone[] out of range\" }\n" );
            return;
        }
        if( duration[i] < 0 || duration[i] > MAX_DURATION ) {
            server.httpSuccess( "application/json" );
            server << F( "{" ) << _errMsg() << F( ", " );
            server << F( "\"message\": \"duration out of range\" }\n" );
            return;
        }
        sprinkler.on( zone[i], duration[i] );
    }
    server.httpSuccess( "application/json" );

    server << F( "{\n" );
    _zonesStatus( server ); // Check return value?
    server << _okMsg() << F( ", " );
    server << F( "\"message\": \"zones successfully scheduled\" " );
    server << F( " }\n" );
    return;
}

void do_multi_zone_off( WebServer &server, uint8_t *zone, uint8_t zone_cnt ) {

    if( zone_cnt == 0 ) {
        sprinkler.allOff();
        server.httpSuccess( "application/json" );
        server << F( "{\n" );
        _zonesStatus( server ); // Check return value?
        server << _okMsg() << F( ", " );
        server << F( "\"message\": \"all zones turned off\" \n" );
        server << F( " }\n" );
        return;
    } else {
        for( uint8_t i = 0; i < zone_cnt; i++ )
            sprinkler.off( zone[i] );

        server.httpSuccess( "application/json" );
        server << F( "{\n" );
        _zonesStatus( server ); // Check return value?
        server << _okMsg() << F( ", " );
        server << F( "\"message\": \"zones turned off\" \n" );
        server << F( " }\n" );
        return;
    }
}

bool isTruey( char *value ) {
    return
        strcasecmp( value, "true" ) == 0
        || strcasecmp( value, "\"true\"" ) == 0
        || strcasecmp( value, "'true'" ) == 0;
}

bool isFalsey( char *value ) {
    return
        strcasecmp( value, "false" ) == 0
        || strcasecmp( value, "\"false\"" ) == 0
        || strcasecmp( value, "'false'" ) == 0;
}

// bool isString( char *value, char * ){
//     return
//         strcasecmp( value, "false" ) == 0
//         || strcasecmp( value, "\"false\"" ) == 0
//         || strcasecmp( value, "'false'" ) == 0;
// }

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
            if( isTruey( value ) ) on = true;
            else if( isFalsey( value ) ) off = true;
        }

        if( strcmp( name, "zone" ) == 0 && strcmp( value, "next" ) == 0 )
            advance = true;

        // Stop processing if too many
        if( zone_cnt > ZONE_COUNT || duration_cnt > ZONE_COUNT ) {
            server.httpSuccess( "application/json" );
            server << F( "{" ) << _errMsg() << F( ", " );
            server << F( "\"message\": \"too many zone[] or duration[] entries\" }\n" );
            return;
        }

        // Serial << name << " = " << value << "\r\n";
        Serial.print( name );
        Serial.print( F( " = " ) );
        Serial.println( value );
    }

    if( advance ) return do_advance( server );
    else if( on ) return do_multi_zone_on( server, zone, zone_cnt,
                                               duration, duration_cnt );
    else if( off ) return do_multi_zone_off( server, zone, zone_cnt );

    server.httpSuccess( "application/json" );
    server << F( "{" ) << _errMsg() << F( ", " );
    server << F( "\"message\": \"on needs to be set to true or false\" }\n" );
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

    server.httpSuccess();
    server << F(
               "<html><head>\n"
               "<title>Arduino Sprinkler Server</title>\n"
               "</head>"
               "<body>" );

    server << F( "<h1>Digital Pins</h1><p>\n" );

    // ignore the pins we use to talk to the Ethernet chip
    for ( uint8_t i = 2; i < 10; ++i ) {
        if( i == 4 ) continue;
        uint8_t val = digitalRead( i );
        server << F( "Digital " ) << i << F( ": " );
        server << ( val ? F( "HIGH" ) : F( "LOW" ) );
        server << F( "<br/>\n" );
    }

    server << F( "</body></html>" );
}

void defaultCmd( WebServer & server, WebServer::ConnectionType type,
                 char *url_tail, bool tail_complete ) {

    server.httpSuccess();
    server << F(
        "<html>"
        "<head>"
        "<title>Arduino Sprinkler System</title>"
        "</head>"
        "<body>" );
    server << F( "Welcome" );
    server << F( "</body></html>" );
}

void do_sprinkler_update( void ) {
    sprinkler.update();
}

void setup() {
    Serial.begin( 9600 );
    Serial.println( F( "beginning setup" ) );

    Ethernet.begin( mac, ip );
    webserver.begin();

    webserver.setDefaultCommand( &defaultCmd );
    webserver.addCommand( "zones", &zonesCmd );
    webserver.addCommand( "debug", &debugCmd );
    webserver.setUrlPathCommand( &zoneCmd );

    t.every( TIMER_PERIOD, do_sprinkler_update );
    Serial.println( F( "completed setup" ) );
}

void loop() {
    // process incoming connections one at a time forever
    webserver.processConnection();

    // do stuff!
    t.update();
}
