# DIY Traffic Counter
This traffic counter sketch was written by Bob Edmiston, but was inspired by so many people who have also been generous in sharing their knowledge, code, thoughts and talents. This code is hereby committed on March 23, 2016 to the Creative Commons.  Attribution is appreciated but not required. I release this into the public domain to help people around the world collect the data they need to make more informed decisions.

This Digital traffic counter is based on the Adafruit ESP8266 HUZZAH Feather + the SD-RTC Feather piggyback shield, but is readily adaptable to any Arduino device that has more than 2K of memory to work with due to library size.

This build works off of Digital GPIO-0 pulled high on startup and triggers when it is pulled low by a PBNO switch or relay as can be found in any reed switch, IR beam break relay or other switching mechanism.   This method was chosen so standard shielded 3 pin XLR microphone cables can be used to carry both power and signaling from the counter to the signaling device.

