import sys
import zlib

f = open(sys.argv[1], 'rb')
decompressed_data = zlib.decompress(f.read())
print type(decompressed_data)
print len(decompressed_data)
