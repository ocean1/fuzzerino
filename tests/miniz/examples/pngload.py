from PIL import Image
import sys

try:
    i = Image.open(sys.argv[1])
    i.rotate(45)
except:
    pass
