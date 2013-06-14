#Same as servicelistener.py but modified slightly for video transmission from Device A to Device B
import dbus, gobject, avahi
from dbus import DBusException
from dbus.mainloop.glib import DBusGMainLoop

device={}
device['IP']=[]
device['PORT']=[]

def listen():
    
    TYPE = "_piper._tcp"
    
    def store(devic):
        global device
        device=devic
        #print device
    
    def service_resolved(*args):
        global device
        print 'service resolved'
        print 'name:', args[2]
        #print IP, PORT
        print 'address:', args[7]
        print 'port:', args[8]
        device['IP'].append(str(args[7]))
        device['PORT'].append(int(args[8]))
        store(device)
        if ":" in device['IP'][-1] :
            del device['IP'][-1]
            del device['PORT'][-1]
        if len(device['IP'])==1:
            loop.quit()
    
    def print_error(*args):
        global device
        print 'error_handler'
        print args[0]
    
    def myhandler(interface, protocol, name, stype, domain, flags):
        global device
        print "Found service '%s' type '%s' domain '%s' " % (name, stype, domain)
    
        if flags & avahi.LOOKUP_RESULT_LOCAL:
                # local service, skip
                pass
    
        server.ResolveService(interface, protocol, name, stype, 
            domain, avahi.PROTO_UNSPEC, dbus.UInt32(0), 
            reply_handler=service_resolved, error_handler=print_error)
        
        
        #print IP, PORT
    
    
    loop = DBusGMainLoop()
    bus = dbus.SystemBus(mainloop=loop)
    server = dbus.Interface( bus.get_object(avahi.DBUS_NAME, '/'),
            'org.freedesktop.Avahi.Server')
    sbrowser = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
            server.ServiceBrowserNew(avahi.IF_UNSPEC,
                avahi.PROTO_UNSPEC, TYPE, 'local', dbus.UInt32(0))),
            avahi.DBUS_INTERFACE_SERVICE_BROWSER)
    
    sbrowser.connect_to_signal("ItemNew", myhandler)
    loop=gobject.MainLoop()
    loop.run()

if __name__ == "__main__":
    listen()

