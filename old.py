import mido
import serial
from mido import MidiFile
song_name = 'alla-turca.mid'
port_number = '14210'
BAUDRATE = 38400
ser = None
piano_in = None
piano_out = None
mode_is_normal = True # middle c is 39
songs = [
            ["alla-turca.mid", [[0, 47], [27, 31], [46, 46]]],
            ["midis/Meek03.mid", [[0, 0]]],
            ["midis/moonlight-sonata-first.mid", [[0, 0]]]
        ] # array of [file path: str, led_coloring_scheme]
header = [100, 101, 102, 103] 
# message format if about changing divider: [100, 101, 102, 103, 1, new_divider)]
# message format if about key press/release: [100, 101, 102, 103, 0, key_num, press_or_release)]
# led_coloring_scheme: array of [[time, position]]. time represents at what time this position should be set while position represents which key divides the piano in left and right hand 

def setup():
    global ser, piano_in, piano_out
    ser = serial.Serial()  # open serial port
    ser.baudrate = BAUDRATE 
    ser.port = f"/dev/cu.usbserial-{port_number}" 
    ser.open()

    piano_in = mido.open_input('CASIO USB-MIDI')
    piano_out = mido.open_output('CASIO USB-MIDI')


def play_midi(song_id):
    song_data = songs[song_id]
    song_name = song_data[0]
    left_right_timings = song_data[1]
    curr_divider_index = -1 # index of divider
    curr_time = 0 # this is in seconds
    for msg in MidiFile(song_name).play():
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
        
        # if right hand note
    #while True:

        change_led(msg)
        piano_out.send(msg)

def change_led(msg): 
    m_dict = msg.dict()
    if m_dict['type'] in ['note_on', 'note_off']:
        # notes start at 21, so shift down by 21
        data = header.copy()
        note_number = m_dict['note'] - 21
        on_or_off = 1 if m_dict['type'] == 'note_on' else 0
        # first byte represents note number, second byte represents if it was on or off
        data.extend([0, note_number, on_or_off])
        print(data)
        ser.write(data) 

def normal_mode(): # mode for when it should just light all LEDs
    for msg in piano_in.iter_pending(): # will only run when a message is received
        print(msg)
        change_led(msg)

def game_mode(): # mode for when a song is currently being played 
    # TODO figure out pseudo code for this later
    print()

def loop():
    # TODO need to listen for input from makerboard in case they are changing the color/picking a song to play
    if mode_is_normal:
        normal_mode()
    else:
        game_mode()

setup()
while True:
    loop()
