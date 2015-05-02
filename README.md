# Arduino/Smart Things Sprinkler Controller

Based on https://github.com/d8adrvn/smart_sprinkler. Awesome project! Thanks.

## Reasons for modifications:

* Want to interface via Internet (Ethernet Shield vs. Xbee). The Smart Things
  Shield is currently out of stock. Might try to implement Xbee over Xbee
  Shield, but don't really need this to be wireless. Turning into an
  object should facilitate access via numerous interfaces.

* The order of watering for zones should be kept. Currently, the lowest
  numbered zone is always watered next.

* Consolidate and simplify the 8 and 24 zone Arduino code.

* Testing

## Developing

Using `ano` to compile and upload. Python package installed via virtualenv
in `~/src/arduino/venv`. Use `smartcd` to source `venv/bin/activate`. Since
`~/src/arduino` is a link to ~/Documents/Arduino`, needed to hand edit some
paths in `venv/bin/*`.

    $ ano build
    $ ano upload
    $ ano serial

## Testing

Want to unit test on development box (not Arduino). Looked into numerous
solutions. Liked approach of: https://github.com/IronSavior/dsm2_tx.

Uses his Makefile which compiles using system (not Arduino) compiler, headers
and libraries for the local development architecture. It also uses MockWProgram
to provide some missing functionality. This works well, but there are some issues:

    * Not using the same header/libraries as Arduino can lead to
      inconsistencies. For example, Arduino has the `log` function, local
      system doesn't without including `math.h`. Same for itoa() .

    * Serial.print

Have tried to use the Arduino headers, etc, but so far without luck. Switched
to g++ as it has the --sysroot and --isystem parameters which allow us to
effectively hide the local files, but we don't have any way to create output.


## Not Implemented

    * Pump function


## Resources:

* Assert while on Arduino:
    https://gist.github.com/jlesech/3089916

http://stackoverflow.com/questions/780819/how-can-i-unit-test-arduino-code
