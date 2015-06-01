import serial
import time
import sys
import re

Print = sys.stdout.write

class Esp8266(object):
  
  class TimeoutError(Exception):
    pass
    
  def __init__(self, port):
    self.port = port

  def replaceNLCR(self, msg):
    return msg.replace('\n', "\\n").replace('\r', "\\r")
    
  def waitFor(self, expextedMessages, timeout=1.0):
    expextedMessages = [re.compile(i) for i in expextedMessages]
    Print("Wifi <- \"")
    start = time.time()
    reply = ""
    while (timeout == 0) or ((time.time()-start) < timeout):
      r = self.port.readline()
      Print(self.replaceNLCR(r))
      reply += r
      for expextedMsg in expextedMessages:
        searchRes = expextedMsg.search(reply) 
        if searchRes != None:
          Print("\" OK time=%.1fs\n" % (time.time()-start))
          return str(searchRes.group())
    Print("\" TIMEOUT\n")
    raise Esp8266.TimeoutError()
   
  def send(self, msg):
    self.port.write(msg)
    self.port.flush()
      
  def sendCmd(self, msg, expextedReplys = ["OK","no change"], timeout=1.0):
    Print("Wifi -> \""+ self.replaceNLCR(msg)+"\"\n")
    self.port.flushInput()
    self.send(msg)
    self.waitFor(expextedReplys, timeout)
       
  def waitForData(self):
    data = self.waitFor(["\+IPD.*\r\n"], timeout = 0)
    dataStart = data.index(":")
    return data[dataStart+1:-2] 
   
  def sendData(self, data):
    self.sendCmd("AT+CIPSEND=0,%d\r\n" % (len(data)+2), [">"] ) 
    self.sendCmd("%s\r\n" %(data)) 


def main():

  if len(sys.argv) != 4:
    print "pfodSever.py SerialPort ssid pwd"
    print " for example: pfodSever.py \\.\COM12 mywifi mywifipw"
    return   

  port = sys.argv[1]
  ssid = sys.argv[2]
  pwd = sys.argv[3]  
  
  
  ser=serial.Serial(port, baudrate=115200, timeout=.1)
  print "port open"
  
  wifi = Esp8266(ser)
  
  time.sleep(1)
  
  wifi.sendCmd("AT\r\n")
  wifi.sendCmd("AT+RST\r\n", ["ready"], timeout=3)
  wifi.sendCmd("AT+CWMODE=1\r\n")
  
  wifi.sendCmd("AT+CIPMODE=0\r\n")
  wifi.sendCmd("AT+CIPMUX=1\r\n")
  
  wifi.sendCmd("AT+CWJAP=\""+ssid+"\",\""+pwd+"\"\r\n", ["OK"], timeout=10)
  
  connected = False
  start = time.time()
  while not connected and (time.time()-start < 10):
    try:
      wifi.sendCmd("AT+CIFSR\r\n", ["\d*\.\d*.\d*\.\d*"])
      connected = True
    except Esp8266.TimeoutError:
      pass
  if not connected:
    print "FAILED TO CONNECT"
    return
      
  wifi.sendCmd("AT+CIPSERVER=1,8080\r\n")
  wifi.sendCmd("AT+CIPSTO=600\r\n")
  
  count = 1
          
  cmd = None
  while True:
    data = wifi.waitForData()
    for c in data:
      if c=='{':
        cmd = ""
      elif c=='}':
        print "pfod   Got CMD", cmd
        wifi.sendData("{.Counting: %d`250}" % count)
        count += 1
      else:
        if cmd != None:
          cmd += c
        
  
  
  
  



if __name__ == "__main__":
		main()
