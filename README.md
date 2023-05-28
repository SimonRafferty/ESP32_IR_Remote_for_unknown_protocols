# ESP32_IR_Remote_for_unknown_protocols
Simple IR Remote which records and replays a signal from Flash/EEPROM

Developed to control an Airxcel MaxxFan extractor fan in my RV.
The MaxxFan seems to use some proprietary protocol which none of the IR libraries I tried could understand or replay.

Using a Scope, I found that is was using a 38kHz carrier - which makes things easier.

The sketch just listens to a GPIO pin, connected to a standard IR Receiver (which demodulates the 38kHz and outputs 1 or 0 for the presence or abcence of carrier).  The timings of the transitions from 0 to 1 and 1 to 0 are stored in an array, then transferred to EEPROM (Actually, Flash on an ESP32).  The recording can then be replayed.  It seems to work on everything I've tried it on so far.

Serial displays a trace of the signal received, for example:
```
______|--------|_______|-----------------|_______|--------|_______|-----------------|_______|--------|_______|----
```
to give you an idea of what it's actually doing.

Most references I've found on line suggest this approach doesn't work well (effectively a glorified tape recorder) but I've found it reliable and at the end of the day, it's better than nothing!

It uses 3 pushbuttons 
```
#define CAPTURE_PIN 5 // Button to trigger capturing
#define BUTTON_PIN_1 19 // Button 1 to trigger sending signal 1
#define BUTTON_PIN_2 18 // Button 2 to trigger sending signal 2
```
Press CAPTURE_PIN to record two button presses on the remote handset.  It's easy to increase the number - I only needed ON & OFF.
Then press BUTTON_PIN_1 or BUTTON_PIN_2 to replay the signal.

Very easy!
