import mido
import time
import serial
from mido import MidiFile
song_name = 'alla-turca.mid'
#port_number = '9600'
BAUDRATE = 38400
ser = None
piano_in = None
piano_out = None
mode_is_normal = True # middle c is 39 
song_id = 0
songs = [["midis/alla-turca.mid", [[0, 87], [27, 31], [46, 46]]]] # array of [file path: str, led_coloring_scheme]
header = [100, 101, 102, 103] 
# message format if about changing divider: [100, 101, 102, 103, 1, new_divider)]
# message format if about key press/release: [100, 101, 102, 103, 0, key_num, press_or_release)]
# led_coloring_scheme: array of [[time, position]]. time represents at what time this position should be set while position represents which key divides the piano in left and right hand 

def setup():
    global ser, piano_in, piano_out
    ser = serial.Serial()  # open serial port
    ser.baudrate = BAUDRATE 
    #ser.port = "COM7" 
    ser.port = '/dev/cu.usbserial-14210'
    ser.open()

    #piano_in = mido.open_input('CASIO USB-MIDI 0')
    #piano_out = mido.open_output('CASIO USB-MIDI 1')

    piano_in = mido.open_input('CASIO USB-MIDI')
    piano_out = mido.open_output('CASIO USB-MIDI')

def play_midi(song_id):
    song_data = songs[song_id]
    song_name = song_data[0]
    left_right_timings = song_data[1]
    curr_divider_index = -1 # index of divider
    curr_time = 0 # this is in seconds
    arr = []
    for msg in MidiFile(song_name):
        if mode_is_normal is True: 
            break 
        curr_seq = msg.dict()
        if not arr and curr_seq["type"] in ["note_on", "note_off"]:
            arr = [msg]
            continue
        # if there are more dividers to go through and if we should update the divider now
        if curr_divider_index + 1 < len(left_right_timings) and left_right_timings[curr_divider_index + 1][0] <= curr_time:
            curr_divider_index += 1
        # if this is about playing a note
        if curr_seq["type"] in ["note_on", "note_off"]:
            if curr_seq["note"] - 21 >= left_right_timings[curr_divider_index][1]:
                if curr_seq["type"] == "note_on":
                    # find all notes that will be played as this note gets played
                    # if right hand, add to some array that tracks if it's pressed at a given time to make sure that they're all played
                    # display ALL LEDS (left and right hand) and then pause until all notes in array is pressed at the same time
                    # once they are all pressed, turn off ONLY the right hand ones
                        # this will make it so that if the left hand is being held by the computer,
                        # it will still display that on the LED
                        # also the computer will handle turning it off since it iwll be expecting a note_off in the future for it
                    # wait for correct input
                    if curr_seq["time"] == 0: 
                        arr.append(msg)
                        continue
                    time.sleep(arr[0].time)
                    on_off_dictionary = {}
                    for arr_msg in arr:
                        change_led(arr_msg, 0, 255, 0)
                        on_off_dictionary[arr_msg.dict()["note"]] = 0
                    print("dic: " + str(on_off_dictionary))

                    while not all(value == 1 for value in on_off_dictionary.values()):
                        key_press_user = piano_in.receive().dict() # can't use this since it's blocking and we have to wait for user input from the makerboard to see if they want to stop playing
                        if mode_is_normal is True: break
                        key_type = key_press_user["type"]
                        if key_type in ["note_on", "note_off"]: 
                            note = key_press_user["note"]
                            if note in on_off_dictionary:
                                if key_type == "note_on":
                                    on_off_dictionary[note] = 1
                                elif key_type == "note_off":
                                    on_off_dictionary[note] = 0
                            #print("Wrong Key Pressed")
                            print(key_press_user["note"])
                    # time now moves on since the notes were pressed
                    curr_time += arr[0].time 
                    # send turn off message by modifying msg and then passing into change_led
                    for arr_msg in arr:
                        modified_msg = mido.Message('note_on', note=arr_msg.dict()["note"])
                        change_led(modified_msg, 0, 0, 0)
                    arr = [msg]
                else:
                    # we actually don't want ot send a right hand turn off because we already told the green LED to turn off
                    # manually and so sending another message would make the on/off array led in the arduino -1 (unintdned behavior)
                    time.sleep(msg.time)
            else:
                # is left hand note
                # just play it as usual
                # the rgb value doesn't matter in note_off since it will ignore
                time.sleep(msg.time)
                change_led(msg, 0, 0, 255)
                piano_out.send(msg)
        else:
            # this could be any message but we know it has a time so we're gonna delay by it
            


def change_led(msg, r, g, b): 
    m_dict = msg.dict()
    if m_dict['type'] in ['note_on', 'note_off']:
        # notes start at 21, so shift down by 21
        data = header.copy()
        note_number = m_dict['note'] - 21
        on_or_off = 1 if m_dict['type'] == 'note_on' else 2
        # first byte represents note number, second byte represents if it was on or off
        data.extend([on_or_off,note_number,r,g,b])
        ser.write(data) 

def normal_mode(): # mode for when it should just light all LEDs
    global mode_is_normal
    if ser.in_waiting:
        incoming = ser.read(6)
        if incoming[4] == 0:
            song_id = incoming[5]
            mode_is_normal = False
    for msg in piano_in.iter_pending(): # will only run when a message is received
        change_led(msg, 0, 0, 0)

def game_mode(song_id): # mode for when a song is currently being played, takes in chosen song as arg 
    # TODO figure out pseudo code for this later
    play_midi(song_id)


def loop():
    # TODO need to listen for input from makerboard in case they are changing the color/picking a song to play
    if mode_is_normal:
        normal_mode()
    else:
        game_mode(song_id)

setup()
while True:
    loop()
