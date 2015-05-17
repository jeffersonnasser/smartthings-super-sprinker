/**
 *  Smart Sprinkler
 *
 *  Copyright 2015 Mark Grimes
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 */

metadata {
    def theZoneCount = 8;

    definition (name: "Sprinkler Controller", namespace: "mvgrimes", author: "Mark Grimes") {
        capability "Switch"  // on, off
        capability "Refresh" // refresh
        capability "Polling" // poll

        for (int i = 1; i <= theZoneCount; i++ ) {
            command "RelayOn${i}"
            command "RelayOff${i}"
        }
        command "OnWithZoneTimes"

        attribute "effect", "string"
    }

    simulator {
        // TODO: define status and reply messages here
    }

    preferences {
        section("Sprinkler Network Address... ") {
            input("confIpAddr", "string", title:"Sprinkler IP Address", defaultValue:"192.168.1.210", required:true, displayDuringSetup: true )
            input("confTcpPort", "number", title:"Sprinkler TCP Port", defaultValue:"80", required:true, displayDuringSetup: true )
        }

        section("Zone Setup... ") {
            for (int i = 1; i <= theZoneCount; i++ ) {
                def zone = NumeralToWord(i)
                input("${zone}Timer", "number", title: "Zone ${zone}", description: "Zone ${zone} Time", required: false)
            }
        }
    }

    tiles {
        // Information only icon for main screen
        standardTile("statusTile", "device.switch", width: 1, height: 1, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Start', icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff"
            state "on", label: 'Running', icon: "st.Health & Wellness.health7", backgroundColor: "#53a7c0"
            state "rainDelayed", label: 'Rain Delay', action: "switch.off", icon: "st.Weather.weather10", backgroundColor: "#fff000"
        }

        // Toggle for entire system
        standardTile("allZonesTile", "device.switch", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Start', action: "switch.on", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff", nextState: "starting"
            state "on", label: 'Running', action: "switch.off", icon: "st.Health & Wellness.health7", backgroundColor: "#53a7c0", nextState: "stopping"
            state "starting", label: 'Starting...', action: "switch.on", icon: "st.Health & Wellness.health7", backgroundColor: "#53a7c0"
            state "stopping", label: 'Stopping...', action: "switch.off", icon: "st.Health & Wellness.health7", backgroundColor: "#53a7c0"
            state "rainDelayed", label: 'Rain Delay', action: "switch.off", icon: "st.Weather.weather10", backgroundColor: "#fff000", nextState: "off"
        }

        // Individual zone tiles
        for (int i = 1; i <= theZoneCount; i++ ) {
            def zone = NumeralToWord(i)
            standardTile("zone${zone}Tile", "device.zone${zone}", width: 1, height: 1, canChangeIcon: true, canChangeBackground: true) {
                state "off${i}", label: "${zone}", action: "RelayOn${i}", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff", nextState: "starting${i}"
                state "starting${i}", label: 'Starting...', action: "RelayOff${i}", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
                state "queued${i}", label: "${zone}", action: "RelayOff${i}", icon: "st.Outdoor.outdoor12", backgroundColor: "#c0a353", nextState: "stopping${i}"
                state "on${i}", label: "${zone}", action: "RelayOff${i}", icon: "st.Outdoor.outdoor12", backgroundColor: "#53a7c0", nextState: "stopping${i}"
                state "stopping${i}", label: 'Stopping...', action: "RelayOff${i}", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            }
        }

        standardTile("scheduleEffect", "device.effect", width: 1, height: 1) {
            state("noEffect", label: "Normal", action: "skip", icon: "st.Office.office7", backgroundColor: "#ffffff")
            state("skip", label: "Skip 1X", action: "expedite", icon: "st.Office.office7", backgroundColor: "#c0a353")
            state("expedite", label: "Expedite", action: "onHold", icon: "st.Office.office7", backgroundColor: "#53a7c0")
            state("onHold", label: "Pause", action: "noEffect", icon: "st.Office.office7", backgroundColor: "#bc2323")
        }

        standardTile("refreshTile", "device.refresh", width: 1, height: 1, canChangeIcon: true, canChangeBackground: true, decoration: "flat") {
            state "ok", label: "", action: "refresh", icon: "st.secondary.refresh", backgroundColor: "#ffffff"
        }

        main "statusTile"
        details(["allZonesTile","zoneOneTile","zoneTwoTile","zoneThreeTile","zoneFourTile","zoneFiveTile","zoneSixTile","zoneSevenTile","zoneEightTile","scheduleEffect","refreshTile"])
    }
}

// parse events into attributes
def parse(String description) {
    TRACE( "Parsing '${description}'" )

    def msg = parseLanMessage(description)
    log.debug "Body: ${msg.body}"

    if( msg.json ){
        log.debug "Status: (${msg.json.status}) ${msg.json.message}"

        if( msg.json.zones ){ // Updating statuses
            for( int i=0; i<msg.json.zones.size(); i++){
                updateZoneState(msg.json.zones[i]);
            }
        }

        updateSwitchState();

    } else {
        log.warn( "Recvd msg w/o valid json" )
    }

}

private def updateZoneState( zone ){
    def num = zone.zone + 1
    def name = "zone" + NumeralToWord( num )
    def state = zone.on ? "on${num}" : ( zone.queued ? "queued${num}" : "off${num}" )

    def ev = createEvent(name: name, value: state,
    isStateChange: true, displayed: true, isPhysical: true)
    log.debug "Parse returned ${ev?.descriptionText}"

    sendEvent(ev)
}

private updateSwitchState(){
    if(anyZoneOn()) {
        def ev = createEvent(name: "switch", value: "on", displayed: true)
        log.debug "Parse returned ${ev?.descriptionText}"
        return ev
    } else if (device.currentValue("switch") != "rainDelayed") {
        def ev = createEvent(name: "switch", value: "off", displayed: true)
        log.debug "Parse returned ${ev?.descriptionText}"
        return ev;
    }
}

// handle commands
def on() {
    TRACE( "Executing 'on'" )

    def zones = []
    def durations = []

    for (int i = 1; i <= 8; i++ ) {
        def zone = NumeralToWord(i)
        def duration

        try {
            duration = settings["${zone}Timer"].toInteger()
        } catch(ClassCastException e) {
            log.warn "Duration for ${zone} is not an integer '${duration}'"
            continue
        }

        // log.debug( "Checking settings[${zone}Timer] = '${duration}'" )
        if( duration > 0 ){
            log.info("Zone ${zone} on for ${duration} minutes")

            zones.push( i - 1 )
            durations.push( duration )
        }
    }

    log.debug( "on putting -> ${zones} / ${durations}" )
    return restPUT( "/zones", [ on: true, "zone[]": zones, "duration[]": durations ] )
}

def OnWithZoneTimes(value) {
    TRACE( "Executing 'allOn' with zone times [$value]" )

    def zones = []
    def durations = []

    for(z in value.split(",")) {
        def parts = z.split(":")
        def zone = parts[0].toInteger()
        def duration = parts[1].toInteger()

        log.info("Zone ${zone} on for ${duration} minutes")

        zones.push( zone - 1 )
        durations.push( duration )
    }

    log.debug( "putting -> ${query}" )
    return restPUT( "/zones", [ on: true, "zone[]": zones, "duration[]": durations ] )
}

def off() {
    TRACE( "Executing 'off'" )
    return restPUT( "/zones", [ on: false ] )
}

def RelayOn1() { return RelayOn(1) }
def RelayOff1() { return RelayOff(1) }
def RelayOn2() { return RelayOn(2) }
def RelayOff2() { return RelayOff(2) }
def RelayOn3() { return RelayOn(3) }
def RelayOff3() { return RelayOff(3) }
def RelayOn4() { return RelayOn(4) }
def RelayOff4() { return RelayOff(4) }
def RelayOn5() { return RelayOn(5) }
def RelayOff5() { return RelayOff(5) }
def RelayOn6() { return RelayOn(6) }
def RelayOff6() { return RelayOff(6) }
def RelayOn7() { return RelayOn(7) }
def RelayOff7() { return RelayOff(7) }
def RelayOn8() { return RelayOn(8) }
def RelayOff8() { return RelayOff(8) }

def RelayOn(Integer zone) {
    def name = NumeralToWord(zone)
    def duration

    try {
        log.debug( "getting duration for ${name}Timer" )
        duration = settings["${name}Timer"].toInteger()
    } catch( ClassCastException e ){
        log.warn "Duration for ${zone} is not an integer '${duration}'"
        return
    }

    return restPUT( "/zone/${zone - 1}", [ on: true, duration: duration ] )
}

def RelayOff(Integer zone) {
    return restPUT( "/zone/${zone - 1}", [ on: false ] )
}

def rainDelayed() {
    log.info "rain delayed"
    if(device.currentValue("switch") != "on") {
        sendEvent(name:"switch", value:"rainDelayed", displayed: true)
    }
}

def refresh() {
    TRACE( "Executing 'refresh()'" )
    return update()
}

def poll() {
    TRACE( "Executing 'poll()'" )
    return update()
}

// commands that over-ride the SmartApp ----------------------------------------------------

// skip one scheduled watering
def skip() {
    def evt = createEvent(name: "effect", value: "skip", displayed: true)
    log.info("Sending: $evt")
    sendEvent(evt)
}
// over-ride rain delay and water even if it rains
def expedite() {
    def evt = createEvent(name: "effect", value: "expedite", displayed: true)
    log.info("Sending: $evt")
    sendEvent(evt)
}

// schedule operates normally
def noEffect() {
    def evt = createEvent(name: "effect", value: "noEffect", displayed: true)
    log.info("Sending: $evt")
    sendEvent(evt)
}

// turn schedule off indefinitely
def onHold() {
    def evt = createEvent(name: "effect", value: "onHold", displayed: true)
    log.info("Sending: $evt")
    sendEvent(evt)
}


// Support routines ----------------------------------------------------
private restGET( path, query ){
    return restRequest( "GET", path, query )
}
private restPUT( path, query ){
    return restRequest( "PUT", path, query )
}
private restPOST( path, query ){
    return restRequest( "POST", path, query )
}

private restRequest( method, path, query ){
    TRACE( "Executing 'restRequest()'" )

    // TODO: only if changed...
    setNetworkId(settings.confIpAddr, settings.confTcpPort)

    def body = ""
    def contentType = "application/json"

    // If POST or PUT, convert query to form-urlencoded body
    if( method != "GET" ){
        contentType = "application/x-www-form-urlencoded"

        query.each{ k, v ->
            // URLEncoder.encode() only takes strings
            def key = URLEncoder.encode( "${k}"   ).replaceAll('\\+','%20')
            if( v instanceof List ){
                v.each{ item ->
                    def val = URLEncoder.encode( "${item}" ).replaceAll('\\+','%20')
                    body = body + "${key}=${val}&"
                }
            } else {
                def val = URLEncoder.encode( "${v}" ).replaceAll('\\+','%20')
                body = body + "${key}=${val}&"
            }
        }

        query = []
    }

    def request = new physicalgraph.device.HubAction(
        method: method,
        path: path,
        headers: [
        HOST: "${settings.confIpAddr}:${settings.confTcpPort}",
        "Content-Type": contentType
        ],
        query: query,
        body: body
    )
    log.debug( request )
    return request
}

private update() {
    TRACE( "Executing 'update()'" )
    return restGET( "/zones", [] )
}

private anyZoneOn() {
    if(device.currentValue("zoneOne") in ["on1","queued1"]) return true;
    if(device.currentValue("zoneTwo") in ["on2","queued2"]) return true;
    if(device.currentValue("zoneThree") in ["on3","queued3"]) return true;
    if(device.currentValue("zoneFour") in ["on4","queued4"]) return true;
    if(device.currentValue("zoneFive") in ["on5","queued5"]) return true;
    if(device.currentValue("zoneSix") in ["on6","queued6"]) return true;
    if(device.currentValue("zoneSeven") in ["on7","queued7"]) return true;
    if(device.currentValue("zoneEight") in ["on8","queued8"]) return true;
    return false;
}

// Sets device Network ID in 'AAAAAAAA:PPPP' format
private String setNetworkId(ipaddr, port) {
    TRACE("setNetworkId(${ipaddr}, ${port})")

    def hexIp = ipaddr.tokenize('.').collect {
        String.format('%02X', it.toInteger())
    }.join()

    def hexPort = String.format('%04X', port.toInteger())
    device.deviceNetworkId = "${hexIp}:${hexPort}"
    log.debug "device.deviceNetworkId = ${device.deviceNetworkId}"
}

// gets the address of the device
private getHostAddress() {
    def ip = getDataValue("ip")
    def port = getDataValue("port")

    if (!ip || !port) {
        def parts = device.deviceNetworkId.split(":")
        if (parts.length == 2) {
            ip = parts[0]
            port = parts[1]
        } else {
            log.warn "Can't figure out ip and port for device: ${device.id}"
        }
    }

    log.debug "Using IP: $ip and port: $port for device: ${device.id}"
    return convertHexToIP(ip) + ":" + convertHexToInt(port)
}

private Integer convertHexToInt(hex) {
    return Integer.parseInt(hex,16)
}

private String convertHexToIP(hex) {
    return [convertHexToInt(hex[0..1]),convertHexToInt(hex[2..3]),convertHexToInt(hex[4..5]),convertHexToInt(hex[6..7])].join(".")
}

// gets the address of the hub
private getCallBackAddress() {
    return device.hub.getDataValue("localIP") + ":" + device.hub.getDataValue("localSrvPortTCP")
}

private def TRACE(message) {
    log.debug message
}

private def NumeralToWord( Integer num ){
    String[] s = ["One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight"] as String[]
    if( num > 0 && num <= s.size() ) return s[num-1]
    else return num
}
