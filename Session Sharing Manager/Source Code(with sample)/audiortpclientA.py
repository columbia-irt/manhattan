#This is to run on Sink1 to advertise its services of routing/forwarding media to other sinks
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
        self.pipeline.get_by_name("udp-packet-receiver").set_property("port", 5000)
        
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
 
 
def test_client():
    """
    Publishes the service and loads the GUI for playing the media
    """

    service = SsmService(name="Audio Player Service 5000", port=5000,DEBUG=True)
    service.publish()
    start=Main()
    gtk.main()
 
if __name__ == "__main__":
    test_client()

