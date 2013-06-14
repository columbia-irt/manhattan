#This is run on the Sink1 to discover Sink 2s that can play the media it has to forward/route
import wx
import pygst
pygst.require("0.10")
import gst
import pygtk
import gtk
import dbus, gobject, avahi
from dbus import DBusException
from dbus.mainloop.glib import DBusGMainLoop
import servicelistener #This is the module which discovers services around it using Avahi


toplay={} #This is a dictionary which gets the IP Address and Port of the Device to which we want to send our media(selected via the GUI)
class Main:

    def __init__(self):
        """
        This sets up the basic GUI which contains buttons corresponding to each device discovered in the network using Avahi
        """

        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.set_title("Choose where you want to send your media")
        
        self.window.set_default_size(600, 100)
        
        self.window.connect("destroy", gtk.main_quit, "WM destroy")
        
        self.create_table()
        self.window.show_all()
        

    def create_table(self):
        """
        Sets up the table layout of the GUI with each button (corresponding to a device discovered) as a row of the table
        """

        print "device", device
        self.table = gtk.Table(len(device['IP']), 1, True)
        self.window.add(self.table)
      
        searchbutton=gtk.Button('Search')
        searchbutton.connect("clicked", self.search)
        self.table.attach(searchbutton,0, 1, 0, 1 ) #Attaching Event handler
        #Depending upon the number of devices found in the network we add rows to the table layout of the GUI
        for i in range(0, len(device['IP'])):
            identity=gtk.Button(str(device['IP'][i])+" "+str(device['PORT'][i]))
            identity.connect("clicked", self.selection, i) #Attaching Event handlers
            self.table.attach(identity, 0, 1, i+1, i+2)
        
    
    def play(self, toplay):
        """
        Implementation of the GStreamer Pipeline
        """
        print "entered pipeline"

        print toplay
        TYPE = "_ssm._tcp"

        self.pipeline = gst.Pipeline("mypipeline")
            
        source = gst.element_factory_make("filesrc", "file-source")
        self.pipeline.add(source)
        self.pipeline.get_by_name("file-source").set_property("location", "./temp.mp3")
#These elements are not needed when we are saving to file.These are only needed when we directly need to play to some sink        
#        decoder = gst.element_factory_make("mad", "mp3-decoder")
#        self.pipeline.add(decoder)
#        
#        conv = gst.element_factory_make("audioconvert", "converter")
#        self.pipeline.add(conv)
        
        caps = gst.Caps("audio/x-raw-int, channels=1,depth=16,width=16,rate=44100")
        filter = gst.element_factory_make("capsfilter", "filter")
        filter.set_property("caps", caps)
        self.pipeline.add(filter)
        
        
        rtpaudiopayloader=gst.element_factory_make("rtpL16pay", "rtpaudio")
        self.pipeline.add(rtpaudiopayloader)
        
        udp=gst.element_factory_make("udpsink", "udp-packet-sender")
        self.pipeline.add(udp)
        #print IP, PORT
        self.pipeline.get_by_name("udp-packet-sender").set_property("host", toplay['IP'])
        self.pipeline.get_by_name("udp-packet-sender").set_property("port", toplay['PORT'])

        #self.pipeline.add( decoder, conv,filter, rtpaudiopayloader, udp)
        gst.element_link_many(source,filter, rtpaudiopayloader, udp)


        self.pipeline.set_state(gst.STATE_PLAYING)

    def selection(self, widget, n):
        """
        Assign toplay the value of the selected device
        """
        toplay['IP']=device['IP'][n]
        toplay['PORT']=device['PORT'][n]
        self.play(toplay)
    
    def search(self, widget):
        """
        Searches for more devices on the network when pressed.Refreshes the table layout
        """

        servicelistener.device={}
        servicelistener.device['IP']=[]
        servicelistener.device['PORT']=[]
        self.window.remove(self.table)
        servicelistener.listen()
        self.create_table()
        self.window.show_all()

servicelistener.listen()
device=servicelistener.device
print device

start=Main()
gtk.main()
