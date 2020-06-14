from __future__ import print_function
import sys
import traceback
from os import listdir
from os.path import isfile

if __name__ == '__main__':
    try:
        onlybmps = [ f for f in listdir(".") if ( isfile(f) and (f.endswith(".bmp") or f.endswith(".png"))) ]
        for fname in onlybmps:
            print("Opening", fname)
            with open(fname, 'rb') as f:
                read_data = f.read()
            wrfilename = fname.split(".")[0] + '.h'
            if fname.endswith(".png"):
                suffix = "_png"
            else:
                suffix = "_bmp"
            wrfilename = fname.split(".")[0] + suffix + '.h'
            with open(wrfilename, 'w') as f:
                f.write("//Generated from file " + fname + "\n")
                count = 0
                f.write("const unsigned char " + fname.split(".")[0] + suffix + "[] =\n{\n    ")
                for byte in read_data:
                    if type(byte) is str: # This is for Python 2
                        f.write("0x%02X, " % ord(byte))
                    else: # This is for Python 3
                        f.write("0x%02X, " % byte)
                    count += 1
                    if (count % 16) == 0:
                        f.write("\n    ")
                f.write("\n};\n")
                f.write("const unsigned int " + fname.split(".")[0] + suffix + "_size = %d;\n" % count)
            print ("%d bytes from %s written into %s" % (count, fname, wrfilename))
    except:
        print(traceback.format_exc())

