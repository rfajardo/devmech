'''
Created on Jul 19, 2011

@author: rfajardo
'''

from lxml import etree

import sys
from os import path


udev_namespace = "http://www.ziti.uni-heidelberg.de/XMLSchemas/usbDevice"
udev_ns = "{%s}" % udev_namespace

class Comment(etree.CommentBase):
    def strip_tag(self):
        return ''

    stripped_tag = property(strip_tag)

class Element(etree.ElementBase):
    def strip_tag(self):
        return self.tag
    
    stripped_tag = property(strip_tag)


class UdevElement(etree.ElementBase):
    def strip_tag(self):
        strip_ns = len(udev_ns)
        return self.tag[strip_ns:]
    
    stripped_tag = property(strip_tag) 


class UdevLookup(etree.CustomElementClassLookup):
    def lookup(self, node_type, document, namespace, name):
        if namespace == udev_namespace:
            return UdevElement
        elif node_type == 'element':
            return Element
        elif node_type == 'comment':
            return Comment
        else:
            return None

class Node(object):
    node_classes = {}
    def import_values(self, element):
        '''Metaclasses have string type data, lists, and Node types.
        import_values iteratively fills metaclasses with xml data transforming its objects in metadata. 
        
        First, the children of the input xml element are retrieved and all strings are copied to the metaobject, if these elements are existent and also strings.
        Then, it iterates over all elements of the metaobject checking for Nodes and lists.   
        
        Strings: 
        import_values iterates over the input xml element retrieving its children and if the name equals a metaclass attribute
        and the attribute is a string it is filled with xml data.
        
        Nodes:
        If the Node attribute name equals a child of the element, the node will be filled with the data of this child.
        
        Lists:
        node_classes is a dictionary with list names, for each list name it has a corresponding xml tag name under the key 'tag'
        and the metaclass to be instantiated and filled under the name 'node_class'.  
        if the list attribute name is listed in node_classes, all direct children elements with the corresponding xml
        tag will be retrieved and its data filled to the corresponding metaclass.
        These filled metaclasses will be appended to the list.
        
        Note: A Node class has to re-implement this function in case it includes nodes or lists types
        which are found in 2nd order hierarchy and above, children of children or more.'''
        if element is not None:
            for item in element.getchildren():
                if item.stripped_tag in self.__dict__:
                    if isinstance(getattr(self, item.stripped_tag), str):
                        setattr(self, item.stripped_tag, item.text)
                        if item.text == 'notparsed':
                            print "Warning: name tag of %s is 'notparsed'; this is evaluated by the parser as not described. Output name will be '' (blank)" % self.__class__.__name__
            for key in self.__dict__:
                if isinstance(getattr(self, key), Node):
                    new_element = element.find(udev_ns + key)
                    getattr(self, key).import_values(new_element)
                elif isinstance(getattr(self, key), list):
                    if key in Node.node_classes:
                        items = element.findall(Node.node_classes[key]['tag'])
                        for item in items:
                            getattr(self, key).append(Node.node_classes[key]['node_class']())
                            getattr(self, key)[-1].import_values(item)
    
    def walk(self, visitor, *args, **kwargs):
        '''It calls the visit function from the input visitor for the calling node. 
        Then, it walks through this given node in its order calling walk again for
        every node attribute included in the calling node. 
        If an attribute is a list and this list include nodes, these nodes walk will 
        be called too.
        That results in a depth first traversing because every internal called node 
        will be completely walked through before the calling node is traversed.''' 
        visitor.visit(self, *args, **kwargs)
        for key in self.__dict__:
            if isinstance(getattr(self, key), Node):
                getattr(self, key).walk(visitor, self)
            elif isinstance(getattr(self, key), list):
                for item in getattr(self, key):
                    if isinstance(item, Node):
                        item.walk(visitor, self)
                        
    def set_default(self):
        pass
                
    @classmethod
    def get_default(cls):
        node = cls()
        node.set_default()
        return node
    
    
class BmRequestType(Node):
    def __init__(self):
        self.direction = 'notparsed'
        self.type = 'notparsed'
        self.recipient = 'notparsed'
        
    def set_default(self):
        self.direction = 'error'
        self.type = 'error'
        self.recipient = 'error'    
        
        
class SetupPacket(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.bmRequestType = BmRequestType()
        
        self.requestName = 'notparsed'
        self.bRequest = 'notparsed'
        
        self.wValue = 'notparsed'
        
        self.wIndex = 'notparsed'
        
        self.wLength = 'notparsed'
        
    def set_default(self):
        self.name = 'error'
        
        self.requestName = 'error'
        self.bRequest = 'error'
        
        self.wValue = 'error'
        
        self.wIndex = 'error'
        self.endpoint = 'error'
        self.interface = 'error'
        
        self.wLength = 'error'
    
    def import_values(self, setup_packet):
        Node.import_values(self, setup_packet)
        if self.wIndex == 'notparsed':
            ns = {'udev': 'http://www.ziti.uni-heidelberg.de/XMLSchemas/usbDevice'}
            
            endpoint_xpath = '/udev:usbCom/udev:standardDescriptors/udev:deviceDescriptor/udev:configurationDescriptor/udev:interfaceDescriptor/udev:endpointDescriptor[udev:name = $name]'
            otherendpoint_xpath = '/udev:usbCom/udev:standardDescriptors/udev:deviceQualifierDescriptor/udev:otherSpeedconfigurationDescriptor/udev:interfaceDescriptors/udev:endpointDescriptor[udev:name = $name]'
            
            interface_xpath = '/udev:usbCom/udev:standardDescriptors/udev:deviceDescriptor/udev:configurationDescriptor/udev:interfaceDescriptor[udev:name = $name]'
            otherinterface_xpath = '/udev:usbCom/udev:standardDescriptors/udev:deviceQualifierDescriptor/udev:otherSpeedconfigurationDescriptor/udev:interfaceDescriptors[udev:name = $name]'            
                    
            for field in setup_packet:
                if field.stripped_tag == 'endpoint':
                    endpoint = setup_packet.xpath(endpoint_xpath, name = field.text, namespaces = ns)
                    otherendpoint = setup_packet.xpath(otherendpoint_xpath, name = field.text, namespaces = ns)
                    if len(endpoint):
                        endpoint_address = endpoint[0].find(udev_ns + 'bEndpointAddress')
                    elif len(otherendpoint):
                        endpoint_address = otherendpoint[0].find(udev_ns + 'bEndpointAddress')
                    else:
                        continue
                        
                    direction_text = endpoint_address.findtext(udev_ns + 'direction')
                    if direction_text == "DeviceToHost":
                        direction = 1
                    else:
                        direction = 0
                    number = endpoint_address.findtext(udev_ns + 'number')
                    
                    index = 0
                    index = direction << 7
                    index |=  int(number, 10)
                    
                    self.wIndex = hex(index)
                        
                                
                if field.stripped_tag == 'interface':
                    interface = setup_packet.xpath(interface_xpath, name = field.text, namespaces = ns)
                    otherinterface = setup_packet.xpath(otherinterface_xpath, name = field.text, namespaces = ns)
                    if len(interface):
                        interface_number = interface[0].findtext(udev_ns + 'bInterfaceNumber')
                    elif len(otherinterface):
                        interface_number = otherinterface[0].findtext(udev_ns + 'bInterfaceNumber')
                    else:
                        continue
                    
                    index = int(interface_number, 10) & 0xFF
                    self.wIndex = hex(index)
        
        
class BmAttributes(Node):
    def __init__(self):
        self.selfPowered = 'notparsed'
        self.remoteWakeup = 'notparsed'
        
    def set_default(self):
        self.selfPowered = 'error'
        self.remoteWakeup = 'error' 
        
        
class EndpointAddress(Node):
    def __init__(self):
        self.direction = 'notparsed'
        self.number = 'notparsed'
        
    def set_default(self):
        self.direction = 'error'
        self.number = 'error'
        

class EndpointBmAttributes(Node):
    def __init__(self):
        self.usageType = 'notparsed'
        self.synchronizationType = 'notparsed'
        self.transferType = 'notparsed'
        
    def set_default(self):
        self.usageType = 'error'
        self.synchronizationType = 'error'
        self.transferType = 'error'
        
        
class EndpointDescriptor(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.bLength = 'notparsed'
        self.bDescriptorType = 'notparsed'
        self.bEndpointAddress = EndpointAddress()
        self.bmAttributes = EndpointBmAttributes()
        self.wMaxPacketSize = 'notparsed'
        self.bInterval = 'notparsed'
        
    def set_default(self):
        self.name = 'error'
        self.bLength = 'error'
        self.bDescriptorType = 'error'
        self.wMaxPacketSize = 'error'
        self.bInterval = 'error'       
        
        
class InterfaceDescriptor(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.bLength = 'notparsed'
        self.bDescriptorType = 'notparsed'
        self.bInterfaceNumber = 'notparsed'
        self.bAlternateSetting = 'notparsed'
        self.bNumEndpoints = 'notparsed'
        self.bInterfaceClass = 'notparsed'
        self.bInterfaceSubClass = 'notparsed'
        self.bInterfaceProtocol = 'notparsed'
        self.iInterface = 'notparsed'
        self.endpoint_descriptors = []
        
    def set_default(self):
        self.name = 'error'
        self.bLength = 'error'
        self.bDescriptorType = 'error'
        self.bInterfaceNumber = 'error'
        self.bAlternateSetting = 'error'
        self.bNumEndpoints = 'error'
        self.bInterfaceClass = 'error'
        self.bInterfaceSubClass = 'error'
        self.bInterfaceProtocol = 'error'
        self.iInterface = 'error'
        
        
class EndpointLink(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.bEndpointAddress = EndpointAddress()
            
    def set_default(self):
        self.name = 'error'
        
        
class InterfaceDescriptorGroup(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.endpoint_links = []
        self.interface_descriptors = []
        
    def set_default(self):
        self.name = 'error'
            

class ConfigurationDescriptor(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.bLength = 'notparsed'
        self.bDescriptorType = 'notparsed'
        self.wTotalLength = 'notparsed'
        self.bNumInterfaces = 'notparsed'
        self.bConfigurationValue = 'notparsed'
        self.iConfiguration = 'notparsed'
        self.bmAttributes = BmAttributes()
        self.bMaxPower = 'notparsed'
        self.interface_descriptor_groups = []
        
    def set_default(self):
        self.name = 'error'
        self.bLength = 'error'
        self.bDescriptorType = 'error'
        self.wTotalLength = 'error'
        self.bNumInterfaces = 'error'
        self.bConfigurationValue = 'error'
        self.iConfiguration = 'error'
        self.bmAttributes = BmAttributes()
        self.bMaxPower = 'error'
        
        
class DeviceDescriptor(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.bLength = 'notparsed'
        self.bDescriptorType = 'notparsed'
        self.bcdUSB = 'notparsed'
        self.bDeviceClass = 'notparsed'
        self.bDeviceProtocol = 'notparsed'
        self.bMaxPacketSize0 = 'notparsed'
        self.idVendor = 'notparsed'
        self.idProduct = 'notparsed'
        self.bcdDevice = 'notparsed'
        self.iManufacturer = 'notparsed'
        self.iProduct = 'notparsed'
        self.iSerialNumber = 'notparsed'
        self.bNumConfigurations = 'notparsed'
        self.configuration_descriptors = []
        
    def set_default(self):
        self.name = 'error'
        self.bLength = 'error'
        self.bDescriptorType = 'error'
        self.bcdUSB = 'error'
        self.bDeviceClass = 'error'
        self.bDeviceProtocol = 'error'
        self.bMaxPacketSize0 = 'error'
        self.idVendor = 'error'
        self.idProduct = 'error'
        self.bcdDevice = 'error'
        self.iManufacturer = 'error'
        self.iProduct = 'error'
        self.iSerialNumber = 'error'
        self.bNumConfigurations = 'error'
        
        
class DeviceQualifierDescriptor(Node):
    def __init__(self):
        self.bLength = 'notparsed'
        self.bDescriptorType = 'notparsed'
        self.bcdUSB = 'notparsed'
        self.bDeviceClass = 'notparsed'
        self.bDeviceSubClass = 'notparsed'
        self.bDeviceProtocol = 'notparsed'
        self.bMaxPacketSize = 'notparsed'
        self.bNumConfigurations = 'notparsed'
        self.bReserved = 'notparsed'
        self.otherSpeedConfiguration_descriptors = []
        

class StringDescriptorZero(Node):
    def __init__(self):
        self.bLength = 'notparsed'
        self.bDescriptorType = 'notparsed'
        self.wLANGIDs = []
        
    def set_default(self):
        self.bLength = 'error'
        self.bDescriptorType = 'error'
        
    def import_values(self, element):
        Node.import_values(self, element)
        if element is not None:
            langids = element.findall(udev_ns + 'wLANGID')
            for langid in langids:
                self.wLANGIDs.append(langid.text)
            
            
class UnicodeStringDescriptor(Node):
    def __init__(self):
        self.bLength = 'notparsed'
        self.bDescriptorType = 'notparsed'
        self.bString = 'notparsed'
        
    def set_default(self):
        self.bLength = 'error'
        self.bDescriptorType = 'error'
        self.bString = 'error'
        
        
class UsbStandardDescriptors(Node):
    def __init__(self):
        self.deviceDescriptor = DeviceDescriptor()
        self.deviceQualifierDescriptor = DeviceQualifierDescriptor()
        self.stringDescriptorZero = StringDescriptorZero()
        self.unicodeStringDescriptor = UnicodeStringDescriptor()
        
        
class Request(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.bRequest = 'notparsed'
        
    def set_default(self):
        self.name = 'noname'
        self.bRequest = 'error'
        
        
class UsbCom(Node):
    def __init__(self):
        self.standardDescriptors = UsbStandardDescriptors()
        self.requests = []
        self.setupPackets = []
        
    def import_values(self, element):
        standard_descriptors = element.find(udev_ns + 'standardDescriptors')
        self.standardDescriptors.import_values(standard_descriptors)
        requests = element.find(udev_ns + 'requests')
        if requests is not None:
            request_list = requests.findall(udev_ns + 'request')
            for request in request_list:
                self.requests.append(Request())
                self.requests[-1].import_values(request)
        setup_packets = element.find(udev_ns + 'setupPackets')
        if setup_packets is not None:
            setup_packet_list = setup_packets.findall(udev_ns + 'setupPacket')
            for setup_packet in setup_packet_list:
                self.setupPackets.append(SetupPacket())
                self.setupPackets[-1].import_values(setup_packet)                


class UdevParser():
    def __init__(self):
        self.parser = etree.XMLParser(attribute_defaults=True)
        self.parser.set_element_class_lookup(UdevLookup())
        
        module_dir = sys.path[0]
        schema = path.join(module_dir, "udev", "usbStandard.xsd")        
        xmlschema_doc = etree.parse(schema)
        self.xmlschema = etree.XMLSchema(xmlschema_doc)
        
        self.usbcom = UsbCom()
        self.document = None
        
    def parse(self, document):
        self.document = etree.parse(document, self.parser)        

    def validate(self):
        try:
            self.xmlschema.assertValid(self.document)
        except etree.DocumentInvalid:
            print "Input XML document is invalid for Udev Schema"
            raise
        else:
            print "Input XML document is valid for Udev Schema"
            
    def generate_usbcom(self):      
        self.usbcom.import_values(self.document)
        
    def get_usbcom(self):
        return self.usbcom
    

Node.node_classes['endpoint_descriptors'] = {'tag': udev_ns + 'endpointDescriptor', 'node_class': EndpointDescriptor}
Node.node_classes['interface_descriptors'] = {'tag': udev_ns + 'interfaceDescriptor', 'node_class': InterfaceDescriptor}
Node.node_classes['endpoint_links'] = {'tag': udev_ns + 'endpointLink', 'node_class': EndpointLink}
Node.node_classes['interface_descriptor_groups'] = {'tag': udev_ns + 'interfaceDescriptorGroup', 'node_class': InterfaceDescriptorGroup}
Node.node_classes['configuration_descriptors'] = {'tag': udev_ns + 'configurationDescriptor', 'node_class': ConfigurationDescriptor}
Node.node_classes['OtherSpeedConfiguration_descriptors'] = {'tag': udev_ns + 'otherSpeedConfigurationDescriptor', 'node_class': ConfigurationDescriptor}