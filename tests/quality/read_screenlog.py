from pylab import *
print argv[0]
SAMPLES=200
f = open("screenlog.0", "r")
data = []
done = False
for line in f:
    try:
        number, value =  [int(field) for field in line.split(":")]
    except ValueError:
        #discard the line if we don't get two ints
        pass
    else:
        if (not done):
            data.append(value)
            done |= (len(data) >= SAMPLES)

plot(data)
