# -*- coding: cp1252 -*-
# <PythonProxy.py>
#
# Downloaded from https://python-proxy.googlecode.com/svn/trunk/PythonProxy.py, but then further modified
#
#Copyright (c) <2009> <F�bio Domingues - fnds3000 in gmail.com>
#
#Permission is hereby granted, free of charge, to any person
#obtaining a copy of this software and associated documentation
#files (the "Software"), to deal in the Software without
#restriction, including without limitation the rights to use,
#copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the
#Software is furnished to do so, subject to the following
#conditions:
#
#The above copyright notice and this permission notice shall be
#included in all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#OTHER DEALINGS IN THE SOFTWARE.

import socket, thread, select, serial, sys, time, re

__version__ = '0.1'
BUFLEN = 100
VERSION = 'Spark Proxy/'+__version__
HTTPVER = 'HTTP/1.1'

class ExtendedSerialPort(serial.Serial):
    def recv(self, *args):
        return self.read(*args)

    def send(self, *args):
        return self.write(*args)

class BufferedSocket:
    def __init__(self, socket):
        self.socket = socket
        self.buffer = ''

    def push_back(self, buffer):
        self.buffer += buffer

    def recv(self, buflen):
        if self.buffer != '':
            ret = self.buffer[:buflen]
            self.buffer = self.buffer[buflen:]
            return ret
        else:
            return self.socket.recv(buflen)

    def send(self, *args):
        return self.socket.send(*args)

    def close(self, *args):
        return self.socket.close(*args)

    def fileno(self):
        return self.socket.fileno()

class ConnectionHandler:
    def __init__(self, connection, address, timeout, ser):
        self.ser = ser
        self.client = BufferedSocket(connection)
        self.client_buffer = ''
        self.timeout = timeout
        self.method, self.path, self.protocol = self.get_base_header()
        try:
            if self.method=='CONNECT':
                self.method_CONNECT()
            elif self.method in ('OPTIONS', 'GET', 'HEAD', 'POST', 'PUT',
                                 'DELETE', 'TRACE'):
                self.method_others()
        finally:
            self.client.close()
            #self.target.close()

    def get_base_header(self):
        while 1:
            self.client_buffer += self.client.recv(BUFLEN)
            end = self.client_buffer.find('\n')
            if end!=-1:
                break
        print '%s'%self.client_buffer[:end]#debug
        data = (self.client_buffer[:end+1]).split()
        self.client_buffer = self.client_buffer[end+1:]
        return data

    def method_CONNECT(self):
        self._connect_target(self.path)
        self.client.send(HTTPVER+' 200 Connection established\n'+
                         'Proxy-agent: %s\n\n'%VERSION)
        self.client_buffer = ''
        self._read_write()

    def method_others(self):
        self.path = self.path[7:]
        i = self.path.find('/')
        host = self.path[:i]
        path = self.path[i:]
        self._connect_target(host)
        self.ser.write('%s %s %s\n'%(self.method, path, self.protocol))
        self.client.push_back(self.client_buffer)
        self.client_buffer = ''
        self._read_write()

    def _connect_target(self, host):
        i = host.find(':')
        if i!=-1:
            port = int(host[i+1:])
            host = host[:i]
        else:
            port = 80
        self.ser.write("c;%s;%s\n"% (host, port))
        response = self.ser.readline().strip()
        if response != "Connected to '%s' and '%s'"%(host, port):
            print "Tried to connect to %s %s"%(host,port)
            do_break(self.ser)
            raise Exception, response

    def _read_write(self):
        time_out_max = self.timeout/3
        socs = [self.client, self.ser]
        count = 0
        gettingHeaders = True
        headerData = ''
        endOfHeaders = re.compile("(?:\n\r|\n){2}")
        headerParse = re.compile("([^:]+): (.+)")
        while 1:
            count += 1
            (recv, _, error) = select.select(socs, [], socs, 3)
            if error:
                break
            if recv:
                for in_ in recv:
                    data = in_.recv(BUFLEN)
                    if in_ is self.client:
                        out = self.ser
                        if gettingHeaders:
                            headerData += data
                            if endOfHeaders.search(headerData)!=None:
                                lines = headerData.splitlines()
                                headers = dict([headerParse.match(x).groups() for x in lines if x!=''])
                                headers["Proxy-Connection"] = "close"
                                headerData = "\n".join(["%s: %s"%v for v in headers.items()])
                                data = headerData + "\n\n"
                                gettingHeaders = False
                            else:
                                continue
                    else:
                        out = self.client
                    if data:
                        out.send(data)
                        count = 0
            if count == time_out_max:
                break

def do_break(ser):
    ser.write("\x1Bstop")
    time.sleep(0.3) # let the Spark catchup
    print ser.read(1000) # skip all the errors from the break command

def start_server(host='localhost', port=8080, IPv6=False, timeout=60,
                  handler=ConnectionHandler, serial_port = '/dev/ttyS1'):
    ser = ExtendedSerialPort(serial_port, 115200, timeout=1)
    do_break(ser)
    ser.write("i")
    line = ser.readline()
    if line != "Ready\r\n":
        raise Exception, line
    if IPv6==True:
        soc_type=socket.AF_INET6
    else:
        soc_type=socket.AF_INET
    soc = socket.socket(soc_type)
    soc.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    soc.bind((host, port))
    print "Serving on %s:%d"%(host, port)#debug
    soc.listen(0)
    while True:
        args = soc.accept()+(timeout,ser)
        try:
            handler(*args)
        except Exception, e:
            print "Exception '%s'" % e
            do_break(ser)

if __name__ == '__main__':
    start_server(serial_port = sys.argv[1])
