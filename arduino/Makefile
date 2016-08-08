SOURCES = t/test-sprinkler.cpp \
					src/Sprinkler.cpp \
					lib/ArduinoTap/*.cpp \
					lib/Timer/*.cpp \
					lib/MockWProgram/MockWProgram.cpp \
					lib/MockWProgram/MockSerial.cpp

OBJECTS := $(addsuffix .o, $(addprefix .build/, $(basename $(SOURCES))))
DEPFILES := $(subst .o,.dep, $(subst .build/,.deps/, $(OBJECTS)))
TESTCPPFLAGS = -D_TEST_ -It -Iarduino -Isrc -Ilib  
		# -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/avr/cores/arduino/
		# -I/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/avr/include \
		# --sysroot=/tmp -isystem=/tmp
		# -I/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/avr/include \
		# -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/avr/cores/arduino/
CPPDEPFLAGS = -MMD -MP -MF .deps/$(basename $<).dep
RUNTEST := $(if $(COMSPEC), runtest.exe, runtest)
# CPPC = /opt/local/bin/c++-mp-4.7 -c
# CPPC = /opt/local/bin/g++-mp-4.7 -c
# CPPC = /usr/bin/c++ -c
# $(COMPILE.cpp) $(TESTCPPFLAGS) $(CPPDEPFLAGS) -o $@ $<

all: runtests

test: runtests
	prove -v ./runtests

.build/%.o: %.cpp
	mkdir -p .deps/$(dir $<)
	mkdir -p .build/$(dir $<)
	$(COMPILE.cpp) $(TESTCPPFLAGS) $(CPPDEPFLAGS) -o $@ $<
	# $(CPPC) $(TESTCPPFLAGS) $(CPPDEPFLAGS) -o $@ $<

runtests: $(OBJECTS)
	$(CC) $(OBJECTS) -lstdc++ -o $@

clean:
	@rm -rf .deps/ .build/ $(RUNTEST)

-include $(DEPFILES)
