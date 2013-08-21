import serial

def read(samples, electrodes):
    s = serial.Serial(port="/dev/ttyACM0")
    data = [[]] * electrodes
    done = [False]*electrodes
    while (False in done):
        #line format is <electrode_number>:<value>
        line = s.readline().rstrip().decode("ascii")
        try:
            number, value =  [int(field) for field in line.split(":")]
        except ValueError:
            #discard the line if we don't get two ints
            pass
        else:
            if (not done[number]):
                data[number].append(value)
                for d in data:
                    #done when length is bigger or equal to number of samples
                    done[number] |= (len(d) >= samples)
    s.close()
    del s #just making sure, probably not needed
    return data

def open():
    global s
    s = serial.Serial(port="/dev/ttyACM0")

def read_one():
    global s
    line = s.readline().rstrip().decode("ascii")
    try:
        number, value =  [int(field) for field in line.split(":")]
    except ValueError:
        return read_one()
    else:
        s.flushInput()
        return value

def close():
    global s
    s.close()
