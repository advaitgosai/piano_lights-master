# Piano Lights
## Summary
This is the code repository for our final project for Fall 2021 [CICS 256 - Make: Physical Computing](https://sites.google.com/view/cics256/home). The project needed to consist of both software and hardware components, and we decided to create LEDs which would illuminate according to the keys played on the piano. From this core idea, we built other features upon it including:

- The piano playing a song by itself OR the ability to "play along"
  -  "Play along" meant only having to play the right hand, and you would be guided by the LEDs.
- A menu to control LED color and song being played
- A script which would convert a MIDI file into a format understandable by our software.

## Setup
Our setup consisted of a Raspberry Pi Zero connected to both the piano and the custom Makerboard/Arduino. The [code](./final.py) for the Raspberry Pi Zero was the middleman between the piano and the Makerboard, while the [code](./arduino_code/arduino_code.ino) for the Makerboard allowed us to control the LEDs.

## Visuals
[Here are pictures and a video demonstration](https://sites.google.com/umass.edu/piano-lights/progress)
