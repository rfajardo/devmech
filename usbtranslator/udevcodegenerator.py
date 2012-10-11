'''
Created on Jul 21, 2011

@author: rfajardo
'''

import udevparser
import string

device = {}
device['type'] = 'struct usbcom'
device['instance'] = 'usbcom'
device['argument'] = ' * ' + device['instance']
device['variable'] = device['instance'] + '->'


class LineFormatter():
    def __init__(self):
        self.indentation_level = 0
        self.prefix = ''
        self.index = []
        self.stored_prefix = ''
        
    def indent(self):
        self.indentation_level += 4
        
    def dedent(self):
        self.indentation_level -= 4        

    def _pop_prefix(self):
        index = []
        index.append(self.prefix.rfind('.'))
        index.append(self.prefix.rfind('>'))
        self.prefix = self.prefix[:max(index)+1]    
        
    def push_prefix(self, prefix):
        self.prefix += prefix + '.'
        
    def pop_prefix(self):
        self.prefix = self.prefix.rstrip('.')
        self._pop_prefix()

    def push_ptr_prefix(self, prefix):
        self.prefix += prefix + '->'    
            
    def pop_ptr_prefix(self):
        self.prefix = self.prefix.rstrip('->')
        self._pop_prefix()
        
    def start_new_index(self):
        self.index.append(0)
    
    def push_indexed_prefix(self, attr):
        self.prefix += attr + '[' + str(self.index[-1]) + ']' +'.'
        self.index[-1] += 1
        
    def pop_indexed_prefix(self):
        self.pop_prefix()
        
    def terminate_index(self):
        self.index.pop()
    
    def gen_index_length(self, attr, length):
        return self.assemble_assignment_prefix(attr + 'Nr', str(length))
    
    def store_prefix(self):
        self.stored_prefix = self.prefix
        
    def reset_prefix(self):
        self.prefix = ''        
        
    def restore_prefix(self):
        self.prefix = self.stored_prefix
    
    def indent_line(self):
        code = ''
        for i in range(self.indentation_level):
            code += ' '
        return code

    def assemble_line(self, line):
        code = ''
        code += self.indent_line()
        code += line + '\n'
        return code
    
    def assemble_line_prefix(self, line):
        code = ''
        code += self.assemble_line(self.prefix + line)
        return code
    
    def assemble_assignment(self, subject, predicate):
        return self.assemble_line(subject + ' = ' + predicate + ';')
    
    def assemble_assignment_prefix(self, subject, predicate):
        return self.assemble_line_prefix(subject + ' = ' + predicate + ';')


class NameResolver():
    name_clash_resolver = {}
    
    def __init__(self):        
        self.clashing_blocks = []
        self.block_clashes = {}
    
    def check_block_clash(self, block):    
        if block.name in self.clashing_blocks:
            self.block_clashes[block.name] += 1
            NameResolver.name_clash_resolver[block] = str(self.block_clashes[block.name])
        else:
            self.clashing_blocks.append(block.name)
            self.block_clashes[block.name] = 1
            NameResolver.name_clash_resolver[block] = ''
              
    @staticmethod
    def get_name(node):
        '''Mainly, this function checks if the name is added to be resolved. If it is,
        the name is adapted by prepending the parent node name to the ordinary name of 
        the node. 
        Address block is resolved differently, by adding numbers on name clashes, so this
        function has to check for the AddressBlock type.
        Furthermore, due to altgroup, which is a string and has to be checked for name clashes, 
        this function has to check if the input node is of string type. If yes, name 
        cannot be used, instead the node itself has the name content.''' 
        if node in NameResolver.name_clash_resolver:
            name = node.name + NameResolver.name_clash_resolver[node]
        else:
            name = node.name
        return name
    
       
class ResquestsResolver(NameResolver):
    def check_clashes(self, requests):
        for request in requests:
            self.check_block_clash(request)


class SetupPacketsResolver(NameResolver):       
    def check_clashes(self, setup_packets):
        for setup_packet in setup_packets:
            self.check_block_clash(setup_packet)


class EndpointDescriptorsResolver(NameResolver):
    def check_clashes(self, endpoint_descriptors):
        for endpoint_descriptor in endpoint_descriptors:
            self.check_block_clash(endpoint_descriptor)
            
            
class InterfaceDescriptorsResolver(NameResolver):
    def check_clashes(self, interface_descriptors):
        endpoint_descriptors_resolver = EndpointDescriptorsResolver()
        for interface_descriptor in interface_descriptors:
            self.check_block_clash(interface_descriptor)
            endpoint_descriptors_resolver.check_clashes(interface_descriptor.endpoint_descriptors)


class EndpointLinksResolver(NameResolver):
    def check_clashes(self, endpoint_links):
        for endpoint_link in endpoint_links:
            self.check_block_clash(endpoint_link)


class InterfaceDescriptorGroupsResolver(NameResolver):
    def check_clashes(self, interface_descriptor_groups):
        for interface_descriptor_group in interface_descriptor_groups:
            self.check_block_clash(interface_descriptor_group)
            endpoint_links_resolver = EndpointLinksResolver()
            endpoint_links_resolver.check_clashes(interface_descriptor_group.endpoint_links)
            interface_descriptors_resolver = InterfaceDescriptorsResolver()
            interface_descriptors_resolver.check_clashes(interface_descriptor_group.interface_descriptors)

            
class ConfigurationDescriptorsResolver(NameResolver):
    def check_clashes(self, configuration_descriptors):
        for configuration_descriptor in configuration_descriptors:
            self.check_block_clash(configuration_descriptor)
            interface_descriptor_groups_resolver = InterfaceDescriptorGroupsResolver()
            interface_descriptor_groups_resolver.check_clashes(configuration_descriptor.interface_descriptor_groups)
            

class NamesResolver():
    def __init__(self):
        self.requests_resolver = ResquestsResolver()
        self.setup_packets_resolver = SetupPacketsResolver()
        self.descriptors_resolver = ConfigurationDescriptorsResolver()
    
    def populate(self, usbcom):            
        self.requests_resolver.check_clashes(usbcom.requests)
        self.setup_packets_resolver.check_clashes(usbcom.setupPackets)
        self.descriptors_resolver.check_clashes(usbcom.standardDescriptors.deviceDescriptor.configuration_descriptors)

            
class Generator(LineFormatter):        
    def gen_requests(self, requests):
        code = ''
        return code
    
    def gen_setup_packets(self, setup_packets):
        code = ''
        return code
    
    def gen_endpoint_descriptor(self, endpoint_descriptor):
        code = ''
        return code
    
    def gen_alternate_interface(self, alternate_interface):
        code = ''
        for endpoint_descriptor in alternate_interface.endpoint_descriptors:
            code += self.gen_endpoint_descriptor(endpoint_descriptor)
        return code

    def gen_interface(self, interface):
        code = ''
        for endpoint_descriptor in interface.endpoint_descriptors:
            code += self.gen_endpoint_descriptor(endpoint_descriptor)
        return code
    
    def gen_interface_group(self, interface_group):
        code = ''
        for index in range(len(interface_group)):        
            if index == 0:
                code += self.gen_interface(interface_group[index])
            else:
                code += self.gen_alternate_interface(interface_group[index])
        return code
    
    def gen_interface_descriptors(self, interface_descriptors):
        '''We guarantee that only interfaces of same interface number are grouped instead of believing 
        that the XML group has been built correctly.'''
        code = ''
        
        interface_groups = []
        for interface_descriptor in interface_descriptors:
            if len(interface_groups) == int(interface_descriptor.bInterfaceNumber,10):
                interface_groups.append([])
            interface_groups[int(interface_descriptor.bInterfaceNumber,10)].append(interface_descriptor)
        
        for interface_group in interface_groups:
            code += self.gen_interface_group(interface_group)
            
        return code
    
    def gen_endpoint_link(self, endpoint_link):
        code = ''
        return code
    
    def gen_interface_descriptor_group(self, interface_descriptor_group):
        code = ''
        for endpoint_link in interface_descriptor_group.endpoint_links:
            code += self.gen_endpoint_link(endpoint_link)
        code += self.gen_interface_descriptors(interface_descriptor_group.interface_descriptors)
        return code
    
    def gen_configuration_descriptor(self, configuration_descriptor):
        code = ''
        for interface_descriptor_group in configuration_descriptor.interface_descriptor_groups:
            code += self.gen_interface_descriptor_group(interface_descriptor_group)
        return code
    
    def gen_standard_descriptors(self, standard_descriptors):
        code = ''        
        for configuration_descriptor in standard_descriptors.deviceDescriptor.configuration_descriptors:
            code += self.gen_configuration_descriptor(configuration_descriptor)
        return code
        
    def generate(self, usbcom):
        code = ''   
        self.push_ptr_prefix(device['instance'])
        code += self.gen_standard_descriptors(usbcom.standardDescriptors)
        code += self.gen_requests(usbcom.requests)
        code += self.gen_setup_packets(usbcom.setupPackets)
        self.pop_ptr_prefix()
        return code
            
            
class HeaderGenerator(Generator):        
    def gen_requests(self, requests):
        code = ''
        code += self.assemble_line('struct  //requests')
        code += self.assemble_line('{')
        self.indent()
        
        for request in requests:
            code += self.assemble_line('Request * ' + NameResolver.get_name(request) + ';')
        
        self.dedent()
        code += self.assemble_line('} requests;')
        code += self.assemble_line('')
        return code
    
    def gen_setup_packets(self, setup_packets):
        code = ''
        code += self.assemble_line('//This is a named structure. This structure has to be given as parameter to a bind function for registers.')
        code += self.assemble_line('struct setup_packets')
        code += self.assemble_line('{')
        self.indent()
        
        for setup_packet in setup_packets:
            code += self.assemble_line('SetupPacket * ' + NameResolver.get_name(setup_packet) + ';')
        
        self.dedent()
        code += self.assemble_line('} setup_packets;')
        code += self.assemble_line('')
        return code
    
    def gen_endpoint_descriptor(self, endpoint_descriptor):
        code = ''
        
        endpoint_type = {'Control': 'CtrlEndpoint', 'Isochronous': 'IsoEndpoint', 'Bulk': 'BulkEndpoint', 'Interrupt': 'IntEndpoint'}
        code += self.assemble_line(endpoint_type[endpoint_descriptor.bmAttributes.transferType] + ' * ' + NameResolver.get_name(endpoint_descriptor) + ';')
        
        return code
    
    def gen_alternate_interface(self, alternate_interface):
        code = ''
        code += self.assemble_line('AltIf * ' + NameResolver.get_name(alternate_interface) + ';')
        
        code += Generator.gen_alternate_interface(self, alternate_interface)
            
        code += self.assemble_line('')
            
        return code

    def gen_interface(self, interface):
        code = ''
        
        code += self.assemble_line('')
        
        code += self.assemble_line('AltIf * ' + NameResolver.get_name(interface) + ';')
            
        code += Generator.gen_interface(self, interface)
            
        code += self.assemble_line('')
            
        return code
    
    def gen_endpoint_link(self, endpoint_link):
        code = ''
        code += self.assemble_line('EndpointDesc * ' + NameResolver.get_name(endpoint_link) + ';')
        return code
    
    def gen_interface_descriptor_group(self, interface_descriptor_group):
        code = ''
        code += self.assemble_line('struct  //' + NameResolver.get_name(interface_descriptor_group))
        code += self.assemble_line('{')
        self.indent()
    
        code += self.assemble_line('IfConf * ' + NameResolver.get_name(interface_descriptor_group) + 'Conf;')
    
        code += Generator.gen_interface_descriptor_group(self, interface_descriptor_group)
        
        self.dedent()
        code += self.assemble_line('} ' + NameResolver.get_name(interface_descriptor_group) + ';')
        code += self.assemble_line('')
            
        return code
    
    def gen_configuration_descriptor(self, configuration_descriptor):
        code = ''
        code += self.assemble_line('struct  //' + NameResolver.get_name(configuration_descriptor))
        code += self.assemble_line('{')
        self.indent()
        
        code += self.assemble_line('Configuration * ' + NameResolver.get_name(configuration_descriptor) + 'Conf' + ';')
        code += self.assemble_line('')
        
        code += Generator.gen_configuration_descriptor(self, configuration_descriptor)
        
        self.dedent()
        code += self.assemble_line('} ' + NameResolver.get_name(configuration_descriptor) + ';')
        code += self.assemble_line('')
        return code
    
    def gen_standard_descriptors(self, standard_descriptors):
        code = ''
        code += self.assemble_line('struct  //standard_descriptors')
        code += self.assemble_line('{')
        self.indent()
        
        code += self.assemble_line('UsbDeviceDesc * deviceDescriptor;')
        code += self.assemble_line('')
        
        code += Generator.gen_standard_descriptors(self, standard_descriptors)
        
        self.dedent()
        code += self.assemble_line('} standard_descriptors;')
        code += self.assemble_line('')
        return code
        
    def generate(self, usbcom):
        code = ''   
        code += self.assemble_line('#ifndef usbcom_H_')
        code += self.assemble_line('#define usbcom_H_') 
        code += self.assemble_line('')
        
        code += self.assemble_line('#include <usbif/usbdata.h>')
        code += self.assemble_line('#include <usbif/usbint.h>')
            
        code += self.assemble_line('')
        code += self.assemble_line('')
        
        code += self.assemble_line(device['type'])
        code += self.assemble_line('{')
        self.indent()
        
        code += self.assemble_line('//interface data')
        code += self.assemble_line('UsbDevice * handle;        //this handle has to take the first position of the structure')
        code += self.assemble_line('//~interface data')
        code += self.assemble_line('')
        
        code += Generator.generate(self, usbcom)
        
        self.dedent()
        code += self.assemble_line('};')
        code += self.assemble_line('')
        
        code += self.assemble_line(device['type'] + ' * ifinit_usbcom(void);')
        code += self.assemble_line('void ifstop_usbcom(' + device['type'] + device['argument'] + ');\n')
        
        code += self.assemble_line('#endif //usbcom_H_')
        
        return code
        
        
class AllocGenerator(Generator):
    def gen_requests(self, requests):
        code = ''
        self.push_prefix('requests')
        
        for request in requests:
            code += self.assemble_assignment_prefix(NameResolver.get_name(request), 'ALLOC(sizeof(Request))')
        
        self.pop_prefix()
        
        code += self.assemble_line('')
        return code
    
    def gen_setup_packets(self, setup_packets):
        code = ''
        self.push_prefix('setup_packets')
        
        for setup_packet in setup_packets:
            code += self.assemble_assignment_prefix(NameResolver.get_name(setup_packet), 'ALLOC(sizeof(SetupPacket))')
    
        self.pop_prefix()
        
        code += self.assemble_line('')
        return code
    
    def gen_endpoint_descriptor(self, endpoint_descriptor):
        code = ''
        
        endpoint_type = {'Control': 'CtrlEndpoint', 'Isochronous': 'IsoEndpoint', 'Bulk': 'BulkEndpoint', 'Interrupt': 'IntEndpoint'}
        code += self.assemble_assignment_prefix(NameResolver.get_name(endpoint_descriptor), 'ALLOC(sizeof(' + endpoint_type[endpoint_descriptor.bmAttributes.transferType] + '))')
        
        return code
    
    def gen_alternate_interface(self, alternate_interface):
        code = ''
        code += self.assemble_assignment_prefix(NameResolver.get_name(alternate_interface), 'alloc_alternate_interface()')
        
        code += Generator.gen_alternate_interface(self, alternate_interface)
            
        code += self.assemble_line('')
            
        return code

    def gen_interface(self, interface):
        code = ''
        code += self.assemble_assignment_prefix(NameResolver.get_name(interface), 'alloc_alternate_interface()')
            
        code += Generator.gen_interface(self, interface)
            
        code += self.assemble_line('')
        return code
    
    def gen_interface_descriptor_group(self, interface_descriptor_group):
        code = ''
        
        self.push_prefix(NameResolver.get_name(interface_descriptor_group))
        
        code += self.assemble_assignment_prefix(NameResolver.get_name(interface_descriptor_group) + 'Conf', 'alloc_interface()')
    
        code += self.assemble_line('')
    
        code += Generator.gen_interface_descriptor_group(self, interface_descriptor_group)
        
        self.pop_prefix()
        
        code += self.assemble_line('')    
        return code
    
    def gen_configuration_descriptor(self, configuration_descriptor):
        code = ''
        self.push_prefix(NameResolver.get_name(configuration_descriptor))
        
        code += self.assemble_assignment_prefix(NameResolver.get_name(configuration_descriptor) + 'Conf', 'ALLOC(sizeof(Configuration))')
        code += self.assemble_line('')
        
        code += Generator.gen_configuration_descriptor(self, configuration_descriptor)
        
        self.pop_prefix()
        
        return code
    
    def gen_standard_descriptors(self, standard_descriptors):
        code = ''
        self.push_prefix('standard_descriptors')
        
        code += self.assemble_assignment_prefix('deviceDescriptor', 'ALLOC(sizeof(UsbDeviceDesc))')
        code += self.assemble_line('')
        
        code += Generator.gen_standard_descriptors(self, standard_descriptors)
        
        self.pop_prefix()
        code += self.assemble_line('')
        return code    
    
    def generate(self, usbcom):
        code = ''
        code += self.assemble_line(device['type'] + ' * alloc_usbcom(void)')
        code += self.assemble_line('{')
        self.indent()
        
        code += self.assemble_assignment(device['type'] + device['argument'], 'ALLOC(sizeof(' + device['type'] + '))')
        code += self.assemble_line('')
        
        code += Generator.generate(self, usbcom)
        
        code += self.assemble_line('return ' + device['instance'] + ';')
        
        self.dedent()
        code += self.assemble_line('}')
        
        return code


class PopulateGenerator(Generator):        
    def __init__(self):
        Generator.__init__(self)
        self.interface_group = ''
        
    def gen_requests(self, requests):
        code = ''
        self.push_prefix('requests')
        
        for request in requests:
            code += self.assemble_assignment('*' + self.prefix + NameResolver.get_name(request), hex(int(request.bRequest, 16)))
        
        self.pop_prefix()
        
        code += self.assemble_line('')
        return code
    
    def gen_setup_packets(self, setup_packets):
        code = ''
        
        direction = {'DeviceToHost': 1, 'HostToDevice': 0}
        type = {'Standard': 0, 'Class': 1, 'Vendor': 2, 'Reserved': 3}
        recipient = {'Device': 0, 'Interface': 1, 'Endpoint': 2, 'Other': 3, 'Reserved': 31}
        
        self.push_prefix('setup_packets')
        
        for setup_packet in setup_packets:
            self.push_ptr_prefix(NameResolver.get_name(setup_packet))
            
            bmrequest_type = 0
            bmrequest_type = direction[setup_packet.bmRequestType.direction] << 7
            bmrequest_type |= type[setup_packet.bmRequestType.type] << 5
            bmrequest_type |= recipient[setup_packet.bmRequestType.recipient]
            
            code += self.assemble_assignment_prefix('bmRequestType', hex(bmrequest_type))
            
            if setup_packet.requestName == 'notparsed':
                code += self.assemble_assignment_prefix('bRequest', setup_packet.bRequest)
            else:
                code += self.assemble_assignment_prefix('bRequest', '*' + device['variable'] + 'requests.' + setup_packet.requestName)
                
            code += self.assemble_assignment_prefix('wValue', hex(int(setup_packet.wValue, 16)) )
                
            code += self.assemble_assignment_prefix('wIndex', hex(int(setup_packet.wIndex, 16)) )
            
            code += self.assemble_assignment_prefix('wLength', str(int(setup_packet.wLength, 10)) )
            
            self.pop_ptr_prefix()
            
            code += self.assemble_line('')
    
        self.pop_prefix()
        
        code += self.assemble_line('')
        return code
    
    def gen_endpoint_descriptor(self, endpoint_descriptor):
        code = ''
        usage_type = {'Data': 0, 'Feedback': 1, 'ImplicitFeedback': 2, 'Reserved': 3 }
        synchronization_type = {'None': 0, 'Asynchronous': 1, 'Adaptive': 2, 'Synchronous': 3 , 'Reserved': 0 }
        transfer_type = {'Control': 0, 'Isochronous': 1, 'Bulk': 2, 'Interrupt': 3 }
        
        self.push_ptr_prefix(NameResolver.get_name(endpoint_descriptor))
        
        if endpoint_descriptor.bEndpointAddress.direction == "DeviceToHost":
            direction = 1
        else:
            direction = 0
        number = int(endpoint_descriptor.bEndpointAddress.number, 10)
        address = 0
        address = direction << 7
        address |= number
        code += self.assemble_assignment_prefix('bEndpointAddress', hex(address) )
        
        usage = usage_type[endpoint_descriptor.bmAttributes.usageType]
        sync = synchronization_type[endpoint_descriptor.bmAttributes.synchronizationType]
        transfer = transfer_type[endpoint_descriptor.bmAttributes.transferType]
        
        bm_attributes = 0
        bm_attributes = usage << 4 | sync << 2 | transfer
        
        code += self.assemble_assignment_prefix('bmAttributes', hex(bm_attributes) )
        
        code += self.assemble_assignment_prefix('wMaxPacketSize', endpoint_descriptor.wMaxPacketSize)
        code += self.assemble_assignment_prefix('bInterval', endpoint_descriptor.bInterval)
        
        self.pop_ptr_prefix()
        code += self.assemble_line('')
        return code
    
    def gen_alternate_interface(self, alternate_interface):
        code = ''
        self.push_ptr_prefix(NameResolver.get_name(alternate_interface))
        code += self.assemble_assignment_prefix('bAlternateSetting', alternate_interface.bAlternateSetting)
        self.pop_ptr_prefix()
        
        code += self.assemble_line('')
        
        code += Generator.gen_alternate_interface(self, alternate_interface)
            
        code += self.assemble_line('')
            
        return code

    def gen_interface(self, interface):
        code = ''
        
        code += self.assemble_assignment(self.interface_group + '->bInterfaceNumber', interface.bInterfaceNumber)
        code += self.assemble_assignment(self.interface_group + '->bAlternateSetting', interface.bAlternateSetting)
        
        code += self.assemble_line('')
        
        self.push_ptr_prefix(NameResolver.get_name(interface))
        code += self.assemble_assignment_prefix('bAlternateSetting', interface.bAlternateSetting)
        self.pop_ptr_prefix()
        
        code += self.assemble_line('')
                    
        code += Generator.gen_interface(self, interface)
            
        code += self.assemble_line('')
        return code
    
    def gen_interface_descriptor_group(self, interface_descriptor_group):
        code = ''
        self.push_prefix(NameResolver.get_name(interface_descriptor_group))
    
        self.interface_group = self.prefix + NameResolver.get_name(interface_descriptor_group) + 'Conf'    
    
        code += Generator.gen_interface_descriptor_group(self, interface_descriptor_group)
        
        self.pop_prefix()
        
        code += self.assemble_line('')    
        return code
    
    def gen_configuration_descriptor(self, configuration_descriptor):
        code = ''
        self.push_prefix(NameResolver.get_name(configuration_descriptor))
        
        code += self.assemble_assignment('*' + self.prefix + NameResolver.get_name(configuration_descriptor) + 'Conf', configuration_descriptor.bConfigurationValue)
        code += self.assemble_line('')
        
        code += Generator.gen_configuration_descriptor(self, configuration_descriptor)
        
        self.pop_prefix()
        
        return code
    
    def gen_standard_descriptors(self, standard_descriptors):
        code = ''
        self.push_prefix('standard_descriptors')
        
        self.push_ptr_prefix('deviceDescriptor')
        
        code += self.assemble_assignment_prefix('idVendor', hex( int(standard_descriptors.deviceDescriptor.idVendor, 16) ) )
        code += self.assemble_assignment_prefix('idProduct', hex( int(standard_descriptors.deviceDescriptor.idProduct, 16) ) )
        code += self.assemble_assignment_prefix('bMaxPacketSize0', standard_descriptors.deviceDescriptor.bMaxPacketSize0)
        code += self.assemble_assignment_prefix('bNumConfigurations', standard_descriptors.deviceDescriptor.bNumConfigurations)
        code += self.assemble_assignment_prefix('bConfigurationValue', '0')
        
        code += self.assemble_line('')
        self.pop_ptr_prefix()
        
        code += Generator.gen_standard_descriptors(self, standard_descriptors)
                
        self.pop_prefix()
        code += self.assemble_line('')
        return code    
    
    def generate(self, usbcom):
        code = ''
        code += self.assemble_line('void populate_usbcom(' + device['type'] + device['argument'] + ')')
        code += self.assemble_line('{')
        self.indent()
        
        code += Generator.generate(self, usbcom)
        
        self.dedent()
        code += self.assemble_line('}')
        code += self.assemble_line('')
        return code


class AssociateGenerator(Generator):    
    def __init__(self):
        Generator.__init__(self)
        self.endpoint_links = []
        self.ifconf = ''
        self.confvalue = ''
        self.parent_altif = ''
        
    def gen_endpoint_descriptor(self, endpoint_descriptor):
        code = ''
        
        code += self.assemble_line('associate_endpoint(' + self.prefix + NameResolver.get_name(endpoint_descriptor) + ', ' + self.confvalue + ', ' + self.parent_altif + ');')
        code += self.assemble_line('add_endpoint(' + device['variable'] + 'handle' + ', ' + self.prefix + NameResolver.get_name(endpoint_descriptor) + ');')
        
        return code
    
    def gen_alternate_interface(self, alternate_interface):
        code = ''
        self.push_ptr_prefix(NameResolver.get_name(alternate_interface))
        self.parent_altif = self.prefix[:-2]
        code += self.assemble_line('associate_alternate_interface(' + self.parent_altif + ', ' + self.ifconf + ');')
        self.pop_ptr_prefix()
        
        code += Generator.gen_alternate_interface(self, alternate_interface)
            
        code += self.assemble_line('')
            
        return code

    def gen_interface(self, interface):
        code = ''
        
        self.parent_altif = self.prefix + NameResolver.get_name(interface)
        
        for endpoint_link in self.endpoint_links:
            code += self.assemble_line('add_endpoint_link('+ self.ifconf + ', ' + endpoint_link['address'] + ', ' + endpoint_link['name'] + ');')
            
        code += self.assemble_line('')
        
        code += self.assemble_line('associate_alternate_interface(' + self.parent_altif + ', ' + self.ifconf + ');')
                    
        code += Generator.gen_interface(self, interface)
        
        code += self.assemble_line('')
        code += self.assemble_line('set_setting_data(' + self.parent_altif + ');')
            
        code += self.assemble_line('')
        return code    
    
    def gen_endpoint_link(self, endpoint_link):
        code = ''
        if endpoint_link.bEndpointAddress.direction == "DeviceToHost":
            direction = 1
        else:
            direction = 0
        number = int(endpoint_link.bEndpointAddress.number, 10)
        address = 0
        address = direction << 7
        address |= number
        
        self.endpoint_links.append({'address': hex(address), 'name': '&' + self.prefix + NameResolver.get_name(endpoint_link)})
        
        return code
    
    def gen_interface_descriptor_group(self, interface_descriptor_group):
        code = ''
        self.push_prefix(NameResolver.get_name(interface_descriptor_group))
    
        self.ifconf = self.prefix + NameResolver.get_name(interface_descriptor_group) + 'Conf'
    
        self.endpoint_links = []
    
        code += Generator.gen_interface_descriptor_group(self, interface_descriptor_group)
        
        self.pop_prefix()
        
        code += self.assemble_line('')    
        return code
    
    def gen_configuration_descriptor(self, configuration_descriptor):
        code = ''
        self.push_prefix(NameResolver.get_name(configuration_descriptor))
        
        self.confvalue = self.prefix + NameResolver.get_name(configuration_descriptor) + 'Conf'
        
        code += Generator.gen_configuration_descriptor(self, configuration_descriptor)
        
        self.pop_prefix()
        
        return code
    
    def gen_standard_descriptors(self, standard_descriptors):
        code = ''
        self.push_prefix('standard_descriptors')
        
        self.push_ptr_prefix('deviceDescriptor')
        
        code += self.assemble_assignment(device['variable'] + 'handle', 'alloc_usb_device(' + self.prefix[:-2] + ')')
        
        code += self.assemble_line('')
        self.pop_ptr_prefix()
        
        code += Generator.gen_standard_descriptors(self, standard_descriptors)
                
        self.pop_prefix()
        code += self.assemble_line('')
        return code    
    
    def generate(self, usbcom):
        code = ''
        code += self.assemble_line('void associate_usbcom(' + device['type'] + device['argument'] + ')')
        code += self.assemble_line('{')
        self.indent()
        
        code += Generator.generate(self, usbcom)
        
        self.dedent()
        code += self.assemble_line('}')
        code += self.assemble_line('')
        return code
    
    
class FreeGenerator(Generator):
    def gen_requests(self, requests):
        code = ''
        self.push_prefix('requests')
        
        for request in requests:
            code += self.assemble_line('FREE(' + self.prefix + NameResolver.get_name(request) + ');')
        
        self.pop_prefix()
        
        code += self.assemble_line('')
        return code
    
    def gen_setup_packets(self, setup_packets):
        code = ''
        self.push_prefix('setup_packets')
        
        for setup_packet in setup_packets:
            code += self.assemble_line('FREE(' + self.prefix + NameResolver.get_name(setup_packet) + ');')
    
        self.pop_prefix()
        
        code += self.assemble_line('')
        return code
    
    def gen_endpoint_descriptor(self, endpoint_descriptor):
        code = ''

        code += self.assemble_line('FREE(' + self.prefix + NameResolver.get_name(endpoint_descriptor) + ');')
        
        return code
    
    def gen_alternate_interface(self, alternate_interface):
        code = ''
        code += self.assemble_line('free_alternate_interface(' + self.prefix + NameResolver.get_name(alternate_interface) + ');')
        
        code += Generator.gen_alternate_interface(self, alternate_interface)
            
        code += self.assemble_line('')
            
        return code

    def gen_interface(self, interface):
        code = ''
        code += self.assemble_line('free_alternate_interface(' + self.prefix + NameResolver.get_name(interface) + ');')
            
        code += Generator.gen_interface(self, interface)
            
        code += self.assemble_line('')
        return code
    
    def gen_interface_descriptor_group(self, interface_descriptor_group):
        code = ''
        self.push_prefix(NameResolver.get_name(interface_descriptor_group))
        
        code += self.assemble_line('free_interface(' + self.prefix + NameResolver.get_name(interface_descriptor_group) + 'Conf);')
        code += self.assemble_line('')
            
        code += Generator.gen_interface_descriptor_group(self, interface_descriptor_group)
        
        self.pop_prefix()
        
        code += self.assemble_line('')    
        return code
    
    def gen_configuration_descriptor(self, configuration_descriptor):
        code = ''
        self.push_prefix(NameResolver.get_name(configuration_descriptor))
        
        code += self.assemble_line('FREE(' + self.prefix + NameResolver.get_name(configuration_descriptor) + 'Conf);')
        code += self.assemble_line('')
        
        code += Generator.gen_configuration_descriptor(self, configuration_descriptor)
        
        self.pop_prefix()
        
        return code
    
    def gen_standard_descriptors(self, standard_descriptors):
        code = ''
        self.push_prefix('standard_descriptors')
        
        code += self.assemble_line('free_usb_device(' + device['variable'] + 'handle' + ');')
        code += self.assemble_line('')
        
        code += self.assemble_line('FREE(' + self.prefix + 'deviceDescriptor);')
        code += self.assemble_line('')
        
        code += Generator.gen_standard_descriptors(self, standard_descriptors)
        
        self.pop_prefix()
        code += self.assemble_line('')
        return code    
    
    def generate(self, usbcom):
        code = ''
        code += self.assemble_line('void free_usbcom(' + device['type'] + device['argument'] + ')')
        code += self.assemble_line('{')
        self.indent()
        
        code += Generator.generate(self, usbcom)
        
        code += self.assemble_line('FREE(' + device['instance'] + ');')
        
        self.dedent()
        code += self.assemble_line('}')
        
        return code    
    

class SourceGenerator(LineFormatter):
    def __init__(self):
        LineFormatter.__init__(self)
        
        self.alloc_generator = AllocGenerator()
        self.populate_generator = PopulateGenerator()
        self.associate_generator = AssociateGenerator()
        self.free_generator = FreeGenerator()
        
    def generate(self, usbcom):
        #header
        code = ''
        code += self.assemble_line('#include "usbcom.h"')
        code += self.assemble_line('\n')
        
        code += self.alloc_generator.generate(usbcom)
        
        code += self.assemble_line('\n')
        
        
        #body
        code += self.populate_generator.generate(usbcom)
        
        #tail
        code += self.associate_generator.generate(usbcom)
        
        #free
        code += self.free_generator.generate(usbcom)
        code += self.assemble_line('\n')
        
        code += self.assemble_line(device['type'] + ' * ifinit_usbcom()')
        code += self.assemble_line('{')
        self.indent()
        code += self.assemble_line(device['type'] + device['argument'] + ';')
        code += self.assemble_assignment(device['instance'], 'alloc_usbcom();')
        code += self.assemble_line('populate_usbcom(' + device['instance'] + ');')
        code += self.assemble_line('associate_usbcom(' + device['instance'] + ');')
        code += self.assemble_line('return ' + device['instance'] + ';')
        self.dedent()
        code += self.assemble_line('}\n')

        code += self.assemble_line('void ifstop_usbcom(' + device['type'] + device['argument'] + ')')
        code += self.assemble_line('{')
        self.indent()
        code += self.assemble_line('free_usbcom(' + device['instance'] + ');')
        self.dedent()
        code += self.assemble_line('}\n')
              
        return code


class FilesGenerator(LineFormatter):    
    '''Functions working on register_files are recursive'''
    def __init__(self):
        LineFormatter.__init__(self)
        
        self.names_resolver = NamesResolver()
        
        self.header_generator = HeaderGenerator()
        self.source_generator = SourceGenerator()
        
        self.files = {}
    
    def gen_code(self, usbcom):
        self.files = {}
        
        self.names_resolver.populate(usbcom)
        
        self.files['name'] = 'usbcom'
        self.files['src'] = self.source_generator.generate(usbcom)
        self.files['hdr'] = self.header_generator.generate(usbcom)
            
    
class UdevCodeGenerator():
    def __init__(self):
        self.code_generator = FilesGenerator()
        
    def generate_code(self, usbcom):
        self.code_generator.gen_code(usbcom)
        
    def get_files(self):
        return self.code_generator.files

