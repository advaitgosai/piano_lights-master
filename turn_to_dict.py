import sys
import os
from mido import MidiFile

if len(sys.argv) == 0:
  sys.exit("usage: python turn_to_dict.py <midi_file.mid>")

path = sys.argv[1]
if os.path.isfile("dict.txt"):
  sys.exit("error: please delete/rename dict.txt first")
print(path)

with open("dict.txt", "w") as out:
  for msg in MidiFile(path):
    out.write(str(msg.dict()) + "\n")

sys.exit("done")

