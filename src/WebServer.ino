/* Web_Demo.pde -- sample code for Webduino server library */

/*
 * To use this demo,  enter one of the following USLs into your browser.
 * Replace "host" with the IP address assigned to the Arduino.
 *
 * http://host/
 * http://host/json
 *
 * This URL brings up a display of the values READ on digital pins 0-9
 * and analog pins 0-5.  This is done with a call to defaultCmd.
 *
 *
 * http://host/form
 *
 * This URL also brings up a display of the values READ on digital pins 0-9
 * and analog pins 0-5.  But it's done as a form,  by the "formCmd" function,
 * and the digital pins are shown as radio buttons you can change.
 * When you click the "Submit" button,  it does a POST that sets the
 * digital pins,  re-reads them,  and re-displays the form.
 *
 */

#define WEBDUINO_FAIL_MESSAGE "<h1>Request Failed</h1>"
#include "SPI.h" // new include
#include "avr/pgmspace.h" // new include
#include "Ethernet.h"
#include "WebServer.h"
#include "Sprinkler.h"

#define VERSION_STRING "0.1"
#define FIRST_ZONE_PIN 5
#define ZONE_COUNT     8
#define MAX_DURATION   60
#define ZONE_ON        LOW
#define ZONE_OFF       HIGH
#define PARAM_SIZE     16
#define VALUE_SIZE     16

// no-cost stream operator as described at
// http://sundial.org/arduino/?page_id=119
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

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server

void zoneStatus( WebServer &server, int zone_id ) {
    ZoneStatus zone;

    if( ! sprinkler.status( zone_id, &zone ) ) {
        // TODO: too late
        server.httpNoContent();
        return;
    }

    server << "{";
    server << " 'zone': " << zone.zone << ",";
    server << " 'on': " << ( zone.on ? "true" : "false" ) << ",";
    server << " 'queued': " << ( zone.queued ? "true" : "false" ) << ",";
    server << " 'duration': " << zone.duration << ",";
    server << " 'time_left': " << zone.time_left;
    server << " }\n";
}

void zonesStatus( WebServer &server ) {
    server << "[\n";
    for( int i = 0; i < ZONE_COUNT; i++ ) {
        server << "    ";
        zoneStatus( server, i );
    }
    server << "]\n";
}

void zoneUpdate( WebServer &server, int zone_id ) {
    bool repeat;
    char name[PARAM_SIZE], value[VALUE_SIZE];

    unsigned long duration = 0;
    long zone = -1;
    bool on;
    bool on_set = false;

    while( server.readPOSTparam( name, PARAM_SIZE, value, VALUE_SIZE ) ) {
        if( strcmp( name, "zone" ) == 0 )
            zone = strtoul( value, NULL, 10 );
        if( strcmp( name, "duration" ) == 0 )
            duration = strtoul( value, NULL, 10 );
        if( strcmp( name, "on" ) == 0 ) {
            if( strcasecmp( value, "true" ) == 0  ) {
                on = true;
                on_set = true;
            } else if( strcasecmp( value, "false" ) == 0  ) {
                on = false;
                on_set = true;
            }
        }
        Serial << name << " = " << value << "\n";
    }

    if( ! on_set ) {
        server.httpSuccess( "application/json" );
        server << "{ error: 'on must be set to true or false' }\n";
        return;
    }
    if( zone < 0 ) zone = zone_id;
    if( zone != zone_id ) {
        server.httpSuccess( "application/json" );
        server << "{ error: 'zone in URL and param differ' }\n";
        return;
    }
    if( zone > 0 && zone < ZONE_COUNT ) {
        server.httpSuccess( "application/json" );
        server << "{ error: 'zone out of range' }\n";
        return;
    }
    if( on && duration > 0 ) {
        if( sprinkler.on( zone, duration ) ) {
            // server.httpSeeOther(PREFIX "/form");
            server.httpSuccess( "application/json" );
            server << "{ success: 'zone queued for " << duration << "' }\n";
            return;
        } else {
            server.httpSuccess( "application/json" );
            server << "{ errror: 'error running sprinkler.on()' }\n";
            return;
        }
    }
    if( !on ) {
        if( sprinkler.off( zone ) ) {
            // server.httpSeeOther(PREFIX "/form");
            server.httpSuccess( "application/json" );
            server << "{ success: 'zone turned off' }\n";
            return;
        } else {
            server.httpSuccess( "application/json" );
            server << "{ errror: 'error running sprinkler.off()' }\n";
            return;
        }
    }

    server.httpSuccess( "application/json" );
    server << "{ message: 'nothing to do' }\n";
}

// on = true -> need zone[] and duration[]
// on = false -> zone[] or allOff
void zonesUpdate( WebServer & server ) {
    bool repeat;
    char name[PARAM_SIZE], value[VALUE_SIZE];

    bool on = false;
    bool on_set = false;
    long unsigned zone[ZONE_COUNT];
    int zone_idx = 0;
    long unsigned duration[ZONE_COUNT];
    int duration_idx = 0;

    while( server.readPOSTparam( name, PARAM_SIZE, value, VALUE_SIZE ) ) {
        if( strcmp( name, "zone[]" ) == 0 )
            zone[zone_idx++] = strtoul( value, NULL, 10 );

        if( strcmp( name, "duration[]" ) == 0 )
            duration[duration_idx++] = strtoul( value, NULL, 10 );

        if( strcmp( name, "on" ) == 0 ) {
            if( strcasecmp( value, "true" ) == 0  ) {
                on = true;
                on_set = true;
            } else if( strcasecmp( value, "false" ) == 0  ) {
                on = false;
                on_set = true;
            }
        }
        // Stop processing if too many
        if( zone_idx >= ZONE_COUNT || duration_idx >= ZONE_COUNT ) {
            server.httpSuccess( "application/json" );
            server << "{ errror: 'too many zone[] or duration[] entries' }\n";
            return;
        }
    }

    if( !on_set ) {
        server.httpSuccess( "application/json" );
        server << "{ errror: 'on needs to be set to true or false' }\n";
        return;
    }
    if( on ) {
        if( zone_idx == 0 ) {
            server.httpSuccess( "application/json" );
            server << "{ errror: 'need to pass the zones to turn on in zone[]' }\n";
            return;
        }
        if( zone_idx != duration_idx ) {
            server.httpSuccess( "application/json" );
            server << "{ errror: 'need to pass the durations for each zone in duration[]' }\n";
            return;
        }
        for( int i = 0; i < zone_idx; i++ ) {
            if( zone[i] < 0 || zone[i] >= ZONE_COUNT ) {
                server.httpSuccess( "application/json" );
                server << "{ errror: 'zone in zone[] out of range' }\n";
                return;
            }
            if( duration[i] <= 0 || duration[i] > MAX_DURATION ) {
                server.httpSuccess( "application/json" );
                server << "{ errror: 'duration out of range' }\n";
                return;
            }
            sprinkler.on( zone[i], duration[i] );
        }

    } else {
        if( zone_idx == 0 ) {
            sprinkler.allOff();
            server.httpSuccess( "application/json" );
            server << "{ success: 'all zones turned off' }\n";
            return;
        } else {
            for( int i = 0; i < zone_idx; i++ )
                sprinkler.off( zone[i] );
            server.httpSuccess( "application/json" );
            server << "{ success: 'zones turned off' }\n";
            return;
        }
    }
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

    int zone_id = atoi( url_path[1] );

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
        "<html>"
        "<head>"
        "<title>Arduino Sprinkler Server</title>"
        "</head>"
        "<body>";

    server.httpSuccess();
    server.printP( htmlHead );

    server << "<h1>Digital Pins</h1><p>";

    // ignore the pins we use to talk to the Ethernet chip
    for ( int i = 2; i < 10; ++i ) {
        int val = digitalRead( i );
        server << "Digital " << i << ": ";
        server << ( val ? "HIGH" : "LOW" );
        server << "<br/>";
    }

    server << "</body></html>";
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
    server << "Welcome";
    server << "</body></html>";
}

void setup() {
    Serial.begin( 9600 );

    // set pins 0-8 for digital input
    Ethernet.begin( mac, ip );
    webserver.begin();

    webserver.setDefaultCommand( &defaultCmd );
    // webserver.addCommand( "zone", &zoneCmd );
    webserver.addCommand( "zones", &zonesCmd );
    webserver.addCommand( "debug", &debugCmd );
    webserver.setUrlPathCommand( &zoneCmd );

    Serial.println( "completed setup" );
}

void loop() {
    // process incoming connections one at a time forever
    webserver.processConnection();

    // do stuff!
    sprinkler.update();
}
