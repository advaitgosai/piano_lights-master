import pygame.midi
import time
import random
import serial

#midi_data = [[[144, 86, 82, 0], 999], [[128, 86, 64, 0], 1372], [[144, 58, 95, 0], 2285], [[128, 58, 64, 0], 2471], [[144, 32, 102, 0], 3327], [[128, 32, 64, 0], 3475], [[144, 78, 107, 0], 4360], [[128, 78, 64, 0], 4486]]
curr_midi_data = [] # used to keep the midi data in lists size of 1024 since that is max size
total_midi_data = []
counter = 0
percent_threshold = 0.10
ser = serial.Serial()  # open serial port
ser.baudrate = 460800
ser.port = '/dev/cu.usbserial-14230' 
ser.open()
def print_devices():
    for n in range(pygame.midi.get_count()):
        # returns (interf, name, input, output, opened)
        print (n,pygame.midi.get_device_info(n))

def readInput(input_device):
    global curr_midi_data
    global counter
    print("Recording...")
    try:
        while True:
            if input_device.poll():
               event = input_device.read(1)[0]
               data = event[0]
               # timestamp = event[1]
               note_number = data[1]
               # velocity = data[2]
               # print (number_to_note(note_number), velocity)
               curr_midi_data.append(event) # 1 means we read the events one by one
               ser.write([note_number-21, 1 if data[0] == 144 else 0])
               counter += 1
               if counter == 1024:
                   total_midi_data.append(curr_midi_data)
                   curr_midi_data = []
                   counter = 0
    except KeyboardInterrupt:
        total_midi_data.append(curr_midi_data)
        ser.close()
        pass
        

def output(output_device):
    print("\nReplaying song...")
    # note varies from 21 to 108
    #while True:
    #    num = random.randint(21, 108)
    #    num2 = random.randint(1, 127)
    #j    output_device.note_on(num, num2)
    #    time.sleep(0.5)
    #    output_device.note_off(num, num2)
    print("Size: " + str((len(total_midi_data) - 1) * 1024 + counter ))
    adjust_timestamps()
    for midi_data in total_midi_data:
        output_device.write(midi_data)
    finish_ms = total_midi_data[-1][-1][1] # use this to determine when song is done
    while pygame.midi.time() <= finish_ms: # loop will end when piece is done
        global percent_threshold
        if (pygame.midi.time()/finish_ms >= percent_threshold):
            print(str(round(percent_threshold * 100)) + "% finished...")
            percent_threshold += .1
    print("song is finished")
    while True:
        continue

        
def adjust_timestamps():
    current_ms = pygame.midi.time()
    for midi_data in total_midi_data:
        for row in midi_data:
            row[1] += current_ms

if __name__ == '__main__':
    pygame.midi.init()
    my_input = pygame.midi.Input(0) 
    my_output = pygame.midi.Output(1, 1) # latency determines if to actually care about timestamp and adds latency to when the 
    # first note plays
    # readInput(my_input)
    # output(my_output)
    print('Listening')
    while True:
        if my_input.poll():
            event = my_input.read(1)[0]
            data = event[0] # gets whether it was press or release
            if (data[0] == 176 or data[0] == 177):
                continue # this corresponds to a petal press
            note_number = data[1] # gets the note number
            ser.write([note_number-21, 1 if data[0] == 144 else 0]) # notes start at 21, so shift down by 21

def play_song(data):    
