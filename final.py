import mido
import ast
import time
import serial
from mido import MidiFile, Message, MetaMessage
song_name = 'alla-turca.mid'
#port_number = '9600'
BAUDRATE = 38400
ser = None
piano_in = None
piano_out = None
play_for_me = False
mode_is_normal = True # middle c is 39 
song_id = 0
songs = [
        ["midi_as_dict/alla-turca-dict.txt", [[0, 47], [44.6, 43], [58.48, 46], [104.157, 43], [119.360, 47], [162.75, 43], [177.86, 46]]], 
        ["midi_as_dict/fur_elise.txt", [[0, 39], [21.325, 40], [35, 39], [38.403, 40], [46.659, 39], [54.571, 41], [57.593, 46], [64.640, 47], [66.081, 46], [67,250, 47]]],    
        ["midi_as_dict/campanella.txt", [[0, 87]]],
        ["midi_as_dict/un_sospiro.txt", [[0, 87]]],
        ["midi_as_dict/twinkle.txt", [[0, 39], [1.021, 40], [8.343, 39], [9.75, 41], [19.619, 39], [21.360, 40], [25, 39]]],
        ["midi_as_dict/never-gonna.txt", [[0, 29]]],
] # array of [file path: str, led_coloring_scheme ]
header = [100, 101, 102, 103] 
# message format if about changing divider: [100, 101, 102, 103, 1, new_divider)]
# message format if about key press/release: [100, 101, 102, 103, 0, key_num, press_or_release)]
# led_coloring_scheme: array of [[time, position]]. time represents at what time this position should be set while position represents which key divides the piano in left and right hand 

def setup():
    global ser, piano_in, piano_out
    ser = serial.Serial()  # open serial port
    ser.baudrate = BAUDRATE 
    #ser.port = "COM7" 
    ser.port = '/dev/cu.usbserial-14220'
    #ser.port = '/dev/ttyUSB0'
    ser.open()

    # advait
    #piano_in = mido.open_input('CASIO USB-MIDI 0')
    #piano_out = mido.open_output('CASIO USB-MIDI 1')

    # will
    piano_in = mido.open_input('CASIO USB-MIDI')
    piano_out = mido.open_output('CASIO USB-MIDI')

    # raspberry
    #piano_out = mido.open_output('CASIO USB-MIDI:CASIO USB-MIDI MIDI 1 20:0')
    #piano_in = mido.open_input('CASIO USB-MIDI:CASIO USB-MIDI MIDI 1 20:0')
def play_game(song_id):
    global mode_is_normal
    song_data = songs[song_id]
    song_location  = song_data[0]
    left_right_timings = song_data[1]
    curr_divider_index = -1 # index of divider
    curr_time = 0 # this is in seconds
    notes_to_play = [[], []]
    msg_array = []
    with open(song_location, "r") as i:
      for line in i:
          msg = None
          dic = ast.literal_eval(line)
          try:
            msg = Message.from_dict(dic)
          except:
            msg = MetaMessage.from_dict(dic)
          if mode_is_normal is True: 
              break 
          # time has passed, so make changes to the divider
          msg_dict = msg.dict()
          msg_type = msg_dict["type"]
          msg_time = msg_dict["time"]
          if msg_time == 0:
              msg_array.append(msg)
          else:
              time.sleep(msg_array[0].time)
              for play_msg in msg_array:
                  if play_msg.dict()["type"] == "note_on":
                      if play_msg.dict()["note"] - 21 >= left_right_timings[curr_divider_index][1]:
                          notes_to_play[1].append(play_msg)
                      else:
                          notes_to_play[0].append(play_msg)
                  elif play_msg.dict()["type"] == "note_off" and play_msg.dict()["note"] - 21 < left_right_timings[curr_divider_index][1]:
                      change_led(play_msg, 0, 0, 0)
                  elif not play_msg.is_meta:
                      piano_out.send(play_msg)

              for left in notes_to_play[0]:
                  change_led(left, 0, 0, 255)

              if notes_to_play[1]:
                  on_off_dictionary = {}
                  for right in notes_to_play[1]:
                      change_led(right, 0, 255, 0)
                      on_off_dictionary[right.dict()["note"]] = 0                    
                  while not all(value == 1 for value in on_off_dictionary.values()):
                      # non-blocking check for input from piano
                      for user_msg in piano_in.iter_pending():
                          key_press_user = user_msg.dict()
                          key_type = key_press_user["type"]
                          if key_type in ["note_on", "note_off"]: 
                              note = key_press_user["note"]
                              if note in on_off_dictionary:
                                  if key_type == "note_on":
                                      on_off_dictionary[note] = 1
                                  elif key_type == "note_off":
                                      on_off_dictionary[note] = 0
                              #print("note pressed: " + str(key_press_user["note"]))
                      # non-blocking call for input from arduino
                      if ser.in_waiting:
                          incoming = ser.read(7)
                          if incoming[4] == 1:
                              mode_is_normal = True
                              break
                  if mode_is_normal is True: break
                  # turns off the right hand
                  for arr_msg in notes_to_play[1]:
                    modified_msg = mido.Message('note_off', note=arr_msg.dict()["note"])
                    change_led(modified_msg, 0, 0, 0)

              for left in notes_to_play[0]:
                  # attempt to make left hand louder
                  left.velocity = (int) (left.velocity * 1.5) if left.velocity*1.5 <= 127 else 127
                  piano_out.send(left)
              curr_time += msg_array[0].time
              # if there are more dividers to go through and if we should update the divider now
              if curr_divider_index + 1 < len(left_right_timings) and left_right_timings[curr_divider_index + 1][0] <= curr_time:
                  curr_divider_index += 1
              msg_array = [msg]
              notes_to_play = [[], []]
    # if it gets here that means that it's done playing
    mode_is_normal = True
    data = header.copy()
    # only need first byte, but it reads by sets of 9
    data.extend([0, 0, 0, 0, 0])
    ser.write(data)
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
    global mode_is_normal, play_for_me, song_id
    if ser.in_waiting:
        incoming = ser.read(7)
        if incoming[4] == 0:
            song_id = incoming[5]
            play_for_me = incoming[6] == 0
            mode_is_normal = False
    for msg in piano_in.iter_pending(): # will only run when a message is received
        if msg.dict()["type"] == "note_on": print(msg.dict()["note"] - 21)
        change_led(msg, 0, 0, 0)

def play_whole_song(song_id):
    global mode_is_normal
    song_data = songs[song_id]
    song_location  = song_data[0]
    left_right_timings = song_data[1]
    curr_divider_index = -1 # index of divider
    curr_time = 0 # this is in seconds

    with open(song_location, "r") as i:
        for line in i:
            if mode_is_normal:
                break;
            msg = None
            dic = ast.literal_eval(line)
            try:
                msg = Message.from_dict(dic)
            except:
                msg = MetaMessage.from_dict(dic)

            time.sleep(msg.time)
            curr_time += msg.time
            if curr_divider_index + 1 < len(left_right_timings) and left_right_timings[curr_divider_index + 1][0] <= curr_time:
                curr_divider_index += 1
            
            # see if user wants to stop playing
            if ser.in_waiting:
                incoming = ser.read(7)
                if incoming[4] == 1:
                    mode_is_normal = True
                    break

            # we don't care if meta message 
            if msg.is_meta:
                continue
            dic = msg.dict()
            # send the message to the arduino, it will know what to do
            if dic["type"] in ["note_on", "note_off"]:
                if dic["note"] - 21 >= left_right_timings[curr_divider_index][1]:
                    change_led(msg, 0, 255, 0)
                else:
                    change_led(msg, 0, 0, 255)
            piano_out.send(msg)

    # if it gets here that means that it's done playing
    mode_is_normal = True
    piano_out.reset()

    # this is needed so that once the song stops, if the user pressed any keys,
    # it doesn't tell normal mode to light them all up at the same time
    for msg in piano_in.iter_pending(): # will only run when a message is received
       continue 

    data = header.copy()
    # only need first byte, but it reads by sets of 9
    data.extend([0, 0, 0, 0, 0])
    ser.write(data)


def loop():
    global play_for_me
    # TODO need to listen for input from makerboard in case they are changing the color/picking a song to play
    if mode_is_normal:
        normal_mode()
    else:
        if play_for_me:
            play_whole_song(song_id)
        else:
            play_game(song_id)


setup()
while True:
    loop()
