
mask=0xdeedbeef

def decode(x):
    #print("before",hex(x))
    r = x&0xff
    r = ((r << 5) | (r >> 3)) & 0xff
    r = r^((mask>>24)&0xff)
    r = ((r << 4) | (r >> 4)) & 0xff
    r = r^((mask>>16)&0xff)
    r = ((r << 3) | (r >> 5)) & 0xff
    r = r^((mask>>8)&0xff)
    r = ((r << 2) | (r >> 6)) & 0xff
    r = r^(mask&0xff)
    r = ((r << 1) | (r >> 7)) & 0xff
    #print("after",hex(r), chr(r))
    return r

with open("hhhhh", 'rb') as f:
    s = f.read()
    for i in s:
        print(chr(decode(i)), end="")
        #decode(i)
        #print(hex(decode(i)), chr(decode(i)))
# print(hex(mask>>24))