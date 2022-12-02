from datetime import datetime
import paho.mqtt.client as mqtt
import threading
import time
import json
import node_class
import myuart as uart

#Variables importantes
host = 'demo.thingsboard.io'
#host = 'localhost'

resetFlag = False
nary = []

next_reading = time.time() 
sens = ['h', 't', 'w', 'l'] #Sensor Type
units = {'t':'°C', 'T':'°C', 'h':'%', 'w':'kg', 'l': 'lx'} #Units per measure
batTime = {"st": time.time(), "t0": 0, "tf": 0, "rt": 60} #Setting times for battery requests

#Nodes
lib = []
for i in range(1,31): #Aqui va un 31
    node = {'ID': i, 'ports': 10, 'token': 'NODE_' + str(i) + '_WSN', 'telemetry': {'ts': 0, 'values': {}}, 'battery': {'Battery': 0}}
    lib.append(node)

#Active nodes
activeNodes = []
nodes = []

# MQTT objects
clients = []

queue = [] #Queue to store messages from the cloud
conf_queue = [] #Queue to store messages to send UART
telemetry_queue = [] #Queue to store messages to send cloud

INTERVAL = 2
on = True
fst_flag = []

deviceName = "Dispositivo serie USB"
deviceName = "Silicon Labs CP210x USB to UART Bridge"

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, rc, *extra_params):
    if rc['session present'] == 0:
        client.connected_flag = True #set flag
        id = clients.index(client)
        print("Node {} Connected With Result Code {}".format(nodes[id]['ID'],rc))
        # Subscribing to receive RPC requests
        client.subscribe('v1/devices/me/rpc/request/+')
        # Subscribing to sheared attributes
        client.subscribe('v1/devices/me/attributes')
        # Subscribe to attributtes respones
        client.subscribe('v1/devices/me/attributes/response/+')
        # Publish active state
        client.publish('v1/devices/me/telemetry', json.dumps({'active':True}), 1)
        client.publish('v1/devices/me/telemetry', json.dumps({'state':"Conectado"}), 1)
    else:
        print("Bad connection Returned code=",rc)
        client.connected_flag = False #set flag
        client.loop_stop()

# The callback for when a PUBLISH message is received from the server.     
def on_message(client, userdata, msg): 
    # print ('Topic: ' + msg.topic + '\nMessage: ' + str(msg.payload))
    to = clients.index(client)
    queue.append([to,msg])

def client_connections():
    print('Creating Connections for ', len(nodes), ' Clients')
    #Create clients
    for i  in range(len(nodes)):
        # Create new instance 
        client = mqtt.Client('Client'+str(nodes[i]['ID']))
        nodes[i]['client'] = client
        clients.append(client)
        # Register connect callback
        client.on_connect = on_connect
        # Registed publish message callback
        client.on_message = on_message
        # Set access token
        client.username_pw_set(nodes[i]['token'])
        try:
            # Connect to ThingsBoard with 60 seconds keepalive interval
            print('Connecting client: ', nodes[i]['ID'])
            client.connect(host, 1883, 60)
        except:
            print('Connection Fialed in node',nodes[i]['ID'])
            continue
        while not client.connected_flag:
            client.loop(0.01) #check for messages
            time.sleep(0.05)
        print('Client {} created'.format(nodes[i]['ID']))
        client.loop_start()
    print("All Clients Connected ")
    

def createNodes():
    #Create nodes
    print('Creating Nodes for ', len(nodes), ' Devices')
    for key in nodes:
        key['node'] = (node_class.Node(key['ID'], key['ports'], sens))
    print("All Nodes Created ")

def attendRPC(msg, dta, id):
    global conf_queue
    # Check request method
    if dta['method'] == 'setPortSens':
        print('Setting Port Sensor')
        nodes[id]['node'].setPortAttr(dta['params'], nodes[id]['ID'])
        nodes[id]['node'].addPortAttr(nodes[id]['ID'])
        portSens = nodes[id]['node'].getPortAttr(nodes[id])
        client = clients[id]
        print('Publishing: ', portSens)
        client.publish(msg.topic.replace('request', 'response'), json.dumps(portSens), 1)
        client.publish('v1/devices/me/attributes', json.dumps(portSens), 1)
        conf_queue.append({'ID':nodes[id]['ID']})
        client.publish('v1/devices/me/telemetry', json.dumps({"P" + [*dta['params']][0][-1:] + "_measr": ""}), 1)
    elif dta['method'] == 'reset':
        conf_queue.clear()
        uart.UART_tx(serialUART, "CG")
    else:
        print('Not recognized method')

def attendATTR(msg, dta, id):
    # 1 sensor 2 sampletime
    if 'response' in msg.topic:
        if 'shared' in dta:
            print('In shared')
            nodes[id]['node'].updtSampleTime(dta['shared'], nodes[id]['ID'])
            conf_queue.append({'ID':nodes[id]['ID']})
        elif 'client' in dta:
            print('In client')
            nodes[id]['node'].updtPortAttr(dta['client'], nodes[id]['ID'])
            portSens = nodes[id]['node'].getPortAttr(nodes[id])
            conf_queue.append({'ID':nodes[id]['ID']})
        else:
            if '/1' in msg.topic: # Sensores
                print('In empty client')
                portSens = nodes[id]['node'].getPortAttr(nodes[id])
                print(json.dumps(portSens))
                clients[id].publish('v1/devices/me/attributes', json.dumps(portSens), 1)
                nodes[id]['node'].addPortAttr(nodes[id]['ID'])
            elif '/2' in msg.topic: # Tiempos de muestreo
                print('In empty shared')
                fst_flag.append("FT")
                ts = nodes[id]['node'].getSampleTime(nodes[id])
                print(json.dumps(ts))
                clients[id].publish('v1/devices/me/attributes', json.dumps(ts), 1)
    else:
        if len(fst_flag) > 0:
            print('Publishing shared')
            fst_flag.pop(0)
            nodes[id]['node'].updtSampleTime(dta, nodes[id]['ID'])
            conf_queue.append({'ID':nodes[id]['ID']})
        else:
            for key in dta:
                sampleTime = {key: dta[key]}
                nodes[id]['node'].setSampleTime(sampleTime, nodes[id]['ID'])
                conf_queue.append({'ID':nodes[id]['ID']})

def read_msg():
    for x in range(len(queue)):
        msg = queue.pop(0)
        id = msg[0] # Pos in the clients list
        msg = msg[1]
        # Decode JSON request
        print('Message for node ', nodes[id]['ID'])
        print(msg.topic)
        data = json.loads(msg.payload)
        print(data)
        if 'attributes' in msg.topic:
            attendATTR(msg, data, id)
        elif 'rpc' in msg.topic:
            attendRPC(msg, data, id)
        else:
            print('Unknown message')

def uart_send(entity):
    if len(conf_queue) > 0:     
        id = conf_queue.pop(0)
        key = activeNodes.index(id['ID'])
        node = nodes[key]['node']
        conf = str(node.getConf(nodes[key]))
        if key != 0:
            uart.UART_tx(entity, conf)
        print('Attended changes ', id)

def dataProcess(key, data):
    data = int(data)
    if key == 't':
        data = data/10
    elif key == 'h':
        data = data/10
    elif key == 'T':
        t = int(data)
        u = t>>4
        d = format(t, "b")[-4:]
        try:
            d = int(d[0])*0.5 + int(d[1])*0.25 + int(d[2])*0.125 + int(d[3])*0.0625
        except:
            d = 0
        data = u + d
    return data

def myPublish():
    # Check if there are messages to send
    if len(telemetry_queue) > 0:
        cloud_msg = telemetry_queue.pop(0)
        # Check if the message is valid
        if cloud_msg != 'none':
            # Node ID form the message
            id = activeNodes.index(cloud_msg[0])
            # Client assosiated to node ID
            client = clients[id]
            # Check the type of message to send
            if cloud_msg[1] == 'B':
                nodes[id]['battery']['Battery'] = cloud_msg[2]
                print('Publishing: {} to node {}'.format(nodes[id]['battery'],nodes[id]['ID']))
                client.publish('v1/devices/me/telemetry', json.dumps(nodes[id]['battery']), 1)
            else: 
                timeStamp = cloud_msg[1]
                measures = cloud_msg[2]
                key_port = [*measures][0]
                values = measures[key_port]
                key_values = [*values]
                lastValue = {}
                lastInt = {}
                for i in key_values:
                    values[i] = values[i].replace("N","")
                    values[i] = dataProcess(i, values[i])
                    values['P'+ key_port + '_' + i] = values[i]
                    del values[i]
                #nodes[id]['telemetry']['ts'] = int(time.time()*1000)
                nodes[id]['telemetry']['ts'] = int(timeStamp)
                nodes[id]['telemetry']['values'] = values
                print('Publishing: {} to node {}'.format(nodes[id]['telemetry'],nodes[id]['ID']))
                client.publish('v1/devices/me/telemetry', json.dumps(nodes[id]['telemetry']), 1)
                for key in values:
                    value = key[-1:].capitalize() + ": " + str(values[key]) + units[key[-1:]]
                    lastValue[key[:-2] + "_measr"] = lastValue[key[:2] + "_measr"] + " y " + value if (key[:2] + "_measr") in lastValue else value
                    if key[:-2] in lastInt:
                        lastInt[key[:-2] + "_2"] = values[key]
                    else:
                        lastInt[key[:-2]] = values[key]
                client.publish('v1/devices/me/telemetry', json.dumps(lastInt), 1)
                client.publish('v1/devices/me/telemetry', json.dumps(lastValue), 1)

def serialSetUp():
    # Configurating UART connection
    print('Configurating UART connection')
    return uart.uart_conf(deviceName)

def sendDate(entity):
    date = datetime.now()
    for key in nodes:
        date = datetime.now()
        y = str(date.year)[2:]
        m = str(date.month) if date.month >= 10 else "0" + str(date.month)
        d = str(date.day) if date.day >= 10 else "0" + str(date.day)
        h = str(date.hour) if date.hour >= 10 else "0" + str(date.hour)
        mi = str(date.minute) if date.minute >= 10 else "0" + str(date.minute)
        s = str(date.second) if date.second >= 10 else "0" + str(date.second)
        string = str(key['ID']) + "/CT-" + str(h) + "," + str(mi) + "," + str(s) + "," + str(d) + "," + str(m) + "," + str(y)
        node = key['node']
        node.addDate(key, string)
        conf_queue.append({'ID':key['ID']})

def serialHandle(entity):
    global resetFlag
    global nary
    # Check for messagee
    if entity.in_waiting > 0:
        try:
            tly = uart.telemetry(entity)
            if len(tly) == 2:
                if tly[1][:2] == 'NT':
                    print("Nary tree ",tly)
                    resetFlag = True
                    nary = tly
            else:
                telemetry_queue.append(tly)
        except: 
            pass
    uart_send(entity)

def cloudHandle():
    while on: 
       myPublish()

def addBattery():
    for key in nodes:
        node = key['node']
        node.addBattery(key)
        conf_queue.append({'ID':key['ID']})

def batteryRequest(times):
    times["t0"] = time.time()
    times["tf"] = times["t0"] - times["st"]
    if times["tf"] >= times["rt"]: 
        addBattery()
        times["st"] = time.time()

def portSensorRequest():
    for id in range(len(clients)):
        client = clients[id]
        portSens = nodes[id]['node'].getPortSens(nodes[id])
        portSens = [*portSens]
        keys = ""
        for i in portSens:
            keys = keys + "," + i
        keys = keys[1:]
        portSens = {"clientKeys":str(keys), "sharedKeys":str(keys)}
        client.publish('v1/devices/me/attributes/request/1', json.dumps(portSens))

def sampleTimeRequest():
    for id in range(len(clients)):
        client = clients[id]
        ts = nodes[id]['node'].getSampleTime(nodes[id])
        ts = [*ts]
        keys = ""
        for i in ts:
            keys = keys + "," + i
        keys = keys[1:]
        string = {"clientKeys":str(keys), "sharedKeys":str(keys)}
        client.publish('v1/devices/me/attributes/request/2', json.dumps(string))

def naryDecode(nary):
    for i in nary[1][2:]:
        id = int(ord(i) - 48)
        if id > 0 and id <= 30: #Aqui va un 30
            activeNodes.append(id)
    for id in activeNodes:
        nodes.append(lib[id-1])

def waitTreeLoop(entity):
    uart.UART_tx(serialUART, "CG")
    print("Starting configuration loop")
    waiting = True
    # Waiting Loop untill nary tree
    while waiting:
        if entity.inWaiting:
            try:
                tly = uart.telemetry(entity)
                if len(tly) == 2:
                    if tly[1][:2] == 'NT':
                        print("Nary tree ", tly)
                        naryDecode(tly)
                        waiting = False
            except:
                continue

def configSetUp(entity):
    #Creating Nodes objects
    createNodes()
    # Creating Clients
    client_connections()
    # Request for Port Sensors
    portSensorRequest()
    # Request for Sample Times
    sampleTimeRequest()
    # Read the messages from the requests
    while (len(conf_queue)) < (len(nodes)*2):
        read_msg()
    # Add date to the UART queue
    sendDate(entity)
    # Add battery requests to UART queue
    addBattery()
    # Wait untill the configuration is sended 
    print("Starting Configuration Send Loop")
    while (len(conf_queue)) > 0:
        uart_send(entity)
        time.sleep(1)

def sysStop():
    print('Quitting')
    for i  in range(len(clients)):
        client = clients[i]
        client.publish('v1/devices/me/telemetry', json.dumps({'active': False}), 1)
        client.publish('v1/devices/me/telemetry', json.dumps({'state':"Desconectado"}), 1)
        for i in range(1,11):
            client.publish('v1/devices/me/telemetry', json.dumps({"P" + str(i) + "_measr": ""}), 1)
        client.loop_stop() # Stop loop 
        time.sleep(0.1)
        client.disconnect()
    n_threads = threading.active_count()
    print('Current threads: ', n_threads)

# Create flag in class
mqtt.Client.connected_flag = False

# Initial threads
n_threads = threading.active_count()
print('Current threads: ', n_threads)

try:
    # Configurating UART connection
    serialUART = serialSetUp()
    # Esperar por el Nary Tree
    waitTreeLoop(serialUART)
    # Configuracion
    configSetUp(serialUART)
    print('Starting main loop')
    while True:
        read_msg()
        batteryRequest(batTime)
        serialHandle(serialUART)
        myPublish()
        if resetFlag:
            print('In nary Loop')
            on = False
            sysStop()
            time.sleep(2)
            activeNodes.clear()
            nodes.clear()
            clients.clear()
            naryDecode(nary)
            configSetUp(serialUART)
            on = True
            resetFlag = False
        else:
            continue
except KeyboardInterrupt:
    print("Interrupted  by keyboard")
    on = False
    print('Closing UART port')
    serialUART.close()
    sysStop()

















