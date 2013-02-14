#This is used to send a mp4 video over the network in a RTP Stream
import wx
import pygst
pygst.require("0.10")
import gst
import pygtk
import gtk
import dbus, gobject, avahi
from dbus import DBusException
from dbus.mainloop.glib import DBusGMainLoop
import servicelistenerA_B


toplay={}
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
        self.table.attach(searchbutton,0, 1, 0, 1 )
#        for i in device.keys():
        for i in range(0, len(device['IP'])):
            identity=gtk.Button(str(device['IP'][i])+" "+str(device['PORT'][i]))
            identity.connect("clicked", self.selection, i)
            self.table.attach(identity, 0, 1, i+1, i+2)
        
    
    def play(self, toplay):
        """
        Implementation of the GStreamer Pipeline
        """

        print "entered pipeline"

        print toplay
        TYPE = "_ssm._tcp"

        self.pipeline = gst.parse_launch("filesrc location=./sample.mp4 ! qtdemux name=d ! queue ! rtpmp4vpay ! udpsink host="+toplay['IP']+" port=5000 d. ! queue ! rtpmp4gpay ! udpsink host="+toplay['IP']+" port=5002")
            
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

        servicelistenerA_B.device={}
        servicelistenerA_B.device['IP']=[]
        servicelistenerA_B.device['PORT']=[]
        self.window.remove(self.table)
        servicelistenerA_B.listen()
        self.create_table()
        self.window.show_all()

servicelistenerA_B.listen()
device=servicelistenerA_B.device
print device

start=Main()
gtk.main()
