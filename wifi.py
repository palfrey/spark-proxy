import serial, sys, time

def do_break(ser):
    ser.write("\x1Bstop")
    time.sleep(0.3) # let the Spark catchup
    ser.read(1000) # skip all the errors from the break command

ser = serial.Serial(sys.argv[1], 115200, timeout=1)
do_break(ser)
ser.write("w;%s;%s\n"%(sys.argv[2], sys.argv[3]))
print ser.read(1000)