import zmq,time
from serial import Serial

ctx = zmq.Context()
sub = ctx.socket(zmq.SUB)
sub.bind('tcp://*:8080')
sub.setsockopt(zmq.SUBSCRIBE,'')
pol = zmq.Poller()
pol.register(sub,zmq.POLLIN)

PS = Serial(port="/dev/ttyACM0", baudrate=115200, timeout=0, writeTimeout=0)

while True:
    while PS.inWaiting()>0:
        print PS.read()
    socks = dict(pol.poll(0))
    if socks.get(sub) == zmq.POLLIN:
        t = sub.recv_json()
        for msg in t:
            #print msg
            if "ordre" in msg:
                if msg["ordre"] == "vit":
                    PS.write("A\n")
                    PS.write(str(msg["id"]+1)+"\n")
                    PS.write("2\n")
                    PS.write(str(int(msg["param"]))+"\n")
                if msg["ordre"] == "raz":
                    PS.write("A\n")
                    PS.write(str(msg["id"]+1)+"\n")
                    PS.write("1\n")
                    PS.write(str(int(msg["param"]))+"\n")
                if msg["ordre"] == "pos":
                    PS.write("A\n")
                    PS.write(str(msg["id"]+1)+"\n")
                    PS.write("3\n")
                    PS.write(str(int(msg["param"]))+"\n")
            if 'Vmax' in msg:
                PS.write("A\n")
                PS.write("5\n")
                PS.write("1\n")
                PS.write(str(int(msg["Vmax"]))+"\n")
            if 'K' in msg:
                PS.write("A\n")
                PS.write("6\n")
                PS.write("1\n")
                PS.write(str(int(msg["K"]*10))+"\n")
            if 'Ki' in msg:
                PS.write("A\n")
                PS.write("7\n")
                PS.write("1\n")
                PS.write(str(int(msg["Ki"]*100))+"\n")
    time.sleep(0.01)