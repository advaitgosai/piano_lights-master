import mido
import serial
from mido import MidiFile
song_name = 'alla-turca.mid'
#port_number = '9600'
BAUDRATE = 38400
ser = None
piano_in = None
piano_out = None
mode_is_normal = False # middle c is 39 
song_id: 0
songs = [["midis/alla-turca.mid", [[0, 47], [27, 31], [46, 46]]]] # array of [file path: str, led_coloring_scheme]
header = [100, 101, 102, 103] 
# message format if about changing divider: [100, 101, 102, 103, 1, new_divider)]
# message format if about key press/release: [100, 101, 102, 103, 0, key_num, press_or_release)]
# led_coloring_scheme: array of [[time, position]]. time represents at what time this position should be set while position represents which key divides the piano in left and right hand 

def setup():
    global ser, piano_in, piano_out
    ser = serial.Serial()  # open serial port
    ser.baudrate = BAUDRATE 
    ser.port = '/dev/cu.usbserial-14210'
    # ser.port = "COM7" 
    ser.open()

    piano_in = mido.open_input('CASIO USB-MIDI')
    piano_out = mido.open_output('CASIO USB-MIDI')
    # piano_in = mido.open_input('CASIO USB-MIDI 0')
    # piano_out = mido.open_output('CASIO USB-MIDI 1')


def play_midi(song_id):
    song_data = songs[song_id]
    song_name = song_data[0]
    left_right_timings = song_data[1]
    curr_divider_index = -1 # index of divider
    curr_time = 0 # this is in seconds
    song_gen = MidiFile(song_name).play() # generator is like stream in javascript, you call next(song_gen) to get the next msg
    while True:
        msg = next(song_gen)
        if mode_is_normal is True: break 
        curr_seq = msg.dict()
        print(curr_seq)
        # update curr_time because if we're in the loop, the note will play now
        curr_time += msg.time 
        # if there are more dividers to go through and if we should update the divider now
        if curr_divider_index + 1 < len(left_right_timings) and left_right_timings[curr_divider_index + 1][0] <= curr_time:
            curr_divider_index += 1
            # TODO send message to the esp32 here to change the divider
            data = header.copy()
            data.extend([1, left_right_timings[curr_divider_index][1], 0])
            print(data)
            ser.write(data)
        if curr_seq["type"] == "note_on" and (curr_seq["note"]) > left_right_timings[curr_divider_index+1][1]:
            #wait for correct input
            key_press_user = piano_in.receive().dict()
            while str(key_press_user["note"]) != str(curr_seq["note"]):
                if mode_is_normal is True: break
                if key_press_user["type"] == "note_on": 
                    print("Wrong Key Pressed")
                    print(key_press_user["note"])
                key_press_user = piano_in.receive().dict()
        change_led(msg)
        piano_out.send(msg)


def change_led(msg, r, g, b): 
    m_dict = msg.dict()
    if m_dict['type'] in ['note_on', 'note_off']:
        # notes start at 21, so shift down by 21
        data = header.copy()
        note_number = m_dict['note'] - 21
        on_or_off = 1 if m_dict['type'] == 'note_on' else 2
        # first byte represents note number, second byte represents if it was on or off
        data.extend([on_or_off,note_number,r,g,b])
        print(data)
        ser.write(data) 

def normal_mode(): # mode for when it should just light all LEDs
    #incoming = [3]#ser.read(6)
    #if incoming[4] == 0:
    #    song_id = incoming[5]
    #    mode_is_normal = False
    for msg in piano_in.iter_pending(): # will only run when a message is received
        print(msg)
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
    normal_mode()
