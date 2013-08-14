import sys
import unittest
import pylab
sys.path.append("../../tools/python_module/")
import nomech



class testQ(unittest.TestCase):
    def test1(self):
        input("make sure electrode 1 is not being touched and press [Enter]")
        data_untouched = nomech.read(samples = 1000, electrodes = 1)
        input("touch electrode 1 and press [Enter]")
        data_touched = nomech.read(samples = 1000, electrodes = 1)

        pylab.plot(data_untouched[0])
        pylab.plot(data_touched[0])
        pylab.show()

        
        #print(data)

