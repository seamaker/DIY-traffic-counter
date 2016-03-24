# DIY Traffic Counter
The Seattle traffic counter was developed by by Bob Edmiston between 2013 and 2016 in response to the need for an efficient and inexpensive way for citizen advocates to collect counts for bicycles, pedestrians and motor vehicle traffic for the purpose of advocating for safer streets.  


This Digital traffic counter is based on the Adafruit ESP8266 HUZZAH Feather + the SD-RTC Feather piggyback shield, but is readily adaptable to any Arduino device that has more than 2K of memory to work with due to library size.

This build works off of Digital GPIO-0 pulled high on startup and triggers when it is pulled low by a PBNO switch or relay as can be found in any reed switch, IR beam break relay or other switching mechanism.   This method was chosen so standard shielded 3 pin XLR microphone cables can be used to carry both power and signaling from the counter to the signaling device.

This project was inspired and assisted by many generous makers who have shared their knowledge, code, thoughts and talents. This code is hereby committed on March 23, 2016 to the Creative Commons.  Attribution is appreciated but not required. I release this into the public domain to help people around the world collect the data they need to make more informed decisions.  A list of sources, part numbers and attribution will be fleshed out over time.

The open source champions that made this project are:
    Ladyada (Engineer Limor Fried) and all of the awesome folks at https://www.adafruit.com/about who not only engineered, built and made all of this awesome hardware, but sell it at fair prices and with zero markup on shipping.  The Adafruit team publishes not only the essential tutorials I needed to learn how to use the hardware, they also provide sample code and libraries that work.  I can't say enough good about how Adafruit has made technology accessible to ordinary people who are not electronic engineers or professional coders.  I buy all that I can from Adafruit because being able to integrate and deploy the hardware is a huge part of the value system and I've not found any company yet who can deliver the entire value package at such a low price.
    
