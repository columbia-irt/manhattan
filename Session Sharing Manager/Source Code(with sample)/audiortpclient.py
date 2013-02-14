#This is run on Sink 2 to advertise its capability of playing any media sent to it
import sys, os
import gobject
import wx
import pygst
pygst.require("0.10")
import gst
import pygtk
import gtk
import avahi
import dbus


class Main:
    def __init__(self):
        """
        This constructor builds the gstreamer pipeline that receives an RTP stream from the server and dumps it to my customized media player
        """

        self.pipeline = gst.Pipeline("mypipeline")
        
        udp=gst.element_factory_make("udpsrc", "udp-packet-receiver")
        self.pipeline.add(udp)
        self.pipeline.get_by_name("udp-packet-receiver").set_property("port", 5001)
        caps = gst.Caps("application/x-rtp, media=(string)audio, clock-rate=44100, width=16, height=16, encoding-name=(string)L16,encoding-params=(string)1, channels=(int)1, channel-position=(int)1, payload=(int)96")
        filter = gst.element_factory_make("capsfilter", "filter")
        filter.set_property("caps", caps)
        self.pipeline.add(filter)
        
        jitter=gst.element_factory_make("gstrtpjitterbuffer", "jitter")
        self.pipeline.add(jitter)
        self.pipeline.get_by_name("jitter").set_property("do-lost", "true")
        
        rtpaudiodepayloader=gst.element_factory_make("rtpL16depay", "rtpdeaudio")
        self.pipeline.add(rtpaudiodepayloader)
                          
        conv = gst.element_factory_make("audioconvert", "converter")
        self.pipeline.add(conv)

        sink= gst.element_factory_make("filesink", "file-sink")
        self.pipeline.add(sink)
        uri="./temp.mp3"
        self.pipeline.get_by_name("file-sink").set_property("location", uri)


        gst.element_link_many(udp, filter, jitter, rtpaudiodepayloader,conv, sink)


        self.pipeline.set_state(gst.STATE_PLAYING)

class SsmService:
    """
    A simple class to publish a network service using avahi.
    """
    def __init__(self, name, port, stype="_ssm._tcp",
                 domain="", host="", text="",DEBUG=False):
        self.name   = name
        self.type   = stype
        self.domain = domain
        self.host   = host
        self.port   = port
        self.text   = text
        self.DEBUG  = DEBUG
 
    def publish(self):
        bus     = dbus.SystemBus()
        server  = dbus.Interface(
                        bus.get_object(
                                 avahi.DBUS_NAME,
                                 avahi.DBUS_PATH_SERVER),
                        avahi.DBUS_INTERFACE_SERVER)
        g = dbus.Interface(
                    bus.get_object(avahi.DBUS_NAME,
                                   server.EntryGroupNew()),
                    avahi.DBUS_INTERFACE_ENTRY_GROUP)
        
        g.AddService(
            avahi.IF_UNSPEC,                #interface
            avahi.PROTO_UNSPEC,             #protocol
            dbus.UInt32(0),                 #flags
            self.name, self.type,           
            self.domain, self.host,
            dbus.UInt16(self.port),
            avahi.string_array_to_txt_array(self.text))

        g.Commit()
        self.group = g
        if(self.DEBUG):
            print("service : %s, published :)" % self.name)
        return 1
 
 #The following class builds the customized media player,which was already there in GStreamer but we modified it to suit our needs
class GTK_Main:
	
	def __init__(self):
		window = gtk.Window(gtk.WINDOW_TOPLEVEL)
		window.set_title("Audio-Player")
		window.set_default_size(500, 50)
		window.connect("destroy", gtk.main_quit, "WM destroy")
		vbox = gtk.VBox()
		window.add(vbox)
		hbox = gtk.HBox()
		vbox.pack_start(hbox, False)
		self.entry = gtk.Entry()
		hbox.add(self.entry)
		self.button = gtk.Button("Start")  #To start playing the video
		hbox.pack_start(self.button, False)
		self.button.connect("clicked", self.start_stop)
		self.movie_window = gtk.DrawingArea()
		vbox.add(self.movie_window)
		window.show_all()
		
		self.player = gst.element_factory_make("playbin2", "player")
		bus = self.player.get_bus()
		bus.add_signal_watch()
		bus.enable_sync_message_emission()
		bus.connect("message", self.on_message)
		bus.connect("sync-message::element", self.on_sync_message)
		
	def start_stop(self, w):
		if self.button.get_label() == "Start":
			filepath ="/home/digvijay/Desktop/Columbia_Sem1/IRT/final_demo/temp.mp3"
			if os.path.isfile(filepath):
				self.button.set_label("Stop")
				self.player.set_property("uri", "file://" + filepath)
				self.player.set_state(gst.STATE_PLAYING)
		else:
			self.player.set_state(gst.STATE_NULL)
			self.button.set_label("Start")
						
	def on_message(self, bus, message):
		t = message.type
		if t == gst.MESSAGE_EOS:
			self.player.set_state(gst.STATE_NULL)
			self.button.set_label("Start")
		elif t == gst.MESSAGE_ERROR:
			self.player.set_state(gst.STATE_NULL)
			err, debug = message.parse_error()
			print "Error: %s" % err, debug
			self.button.set_label("Start")
	
	def on_sync_message(self, bus, message):
		if message.structure is None:
			return
		message_name = message.structure.get_name()
		if message_name == "prepare-xwindow-id":
			imagesink = message.src
			imagesink.set_property("force-aspect-ratio", True)
			gtk.gdk.threads_enter()
			imagesink.set_xwindow_id(self.movie_window.window.xid)
			gtk.gdk.threads_leave()

def test_client():
    """
    Publishes the service and loads the GUI for playing the media
    """
    service = SsmService(name="Audio Player Service 5001", port=5001,DEBUG=True)
    service.publish()
    start=Main()
    GTK_Main()
    gtk.gdk.threads_init()

    gtk.main()
 
if __name__ == "__main__":
    test_client()

