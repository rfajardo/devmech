'''
Created on Dec 16, 2010

@author: rfajardo
'''

from lxml import etree

import sys
from os import path


spirit_namespace = "http://www.spiritconsortium.org/XMLSchema/SPIRIT/1685-2009"
spirit_ns = "{%s}" % spirit_namespace


class Element(etree.ElementBase):
    def strip_tag(self):
        return self.tag
    
    stripped_tag = property(strip_tag)


class SpiritElement(etree.ElementBase):
    def strip_tag(self):
        strip_ns = len(spirit_ns)
        return self.tag[strip_ns:]
    
    stripped_tag = property(strip_tag) 


class SpiritLookup(etree.CustomElementClassLookup):
    def lookup(self, node_type, document, namespace, name):
        if namespace == spirit_namespace:
            return SpiritElement
        else:
            return Element

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
                    new_element = element.find(spirit_ns + key)
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

class ComplexType(Node):
    def __init__(self):
        self.recvPacket = 'notparsed'
        self.sendPacket = 'notparsed'
        
    def set_default(self):
        self.recvPacket = 'NULL'
        self.sendPacket = 'NULL'


class Strobe(Node):
    def __init__(self):
        self.read = 'notparsed'
        self.write = 'notparsed'
        self.noAction = 'notparsed'
        
    def set_default(self):
        '''Value for noAction has to have the length of the containing 
        field. Since the default is zero, it is representation in length 
        is the same regardless of bit width.'''
        self.read = 'false'
        self.write = 'false'
        self.noAction = '0'
            

class VendorExtensions(Node):
    def __init__(self):
        self.strobe = Strobe()
        self.complexType = ComplexType()
        

class Reset(Node):
    def __init__(self):
        self.valid = 'notparsed'
        self.value = 'notparsed'
        self.mask = 'notparsed'
        
    def set_default(self):
        '''Values for value and mask have to have the length of the containing 
        register. spiritresolver takes care of this.'''
        self.valid = 'false'
        self.value = '0'
        self.mask = '0xFF'
        

class WriteValueConstraint(Node):
    def __init__(self):
        self.valid = 'notparsed'
        self.writeAsRead = 'notparsed'
        self.useEnumeratedValues = 'notparsed'
        self.minimum = 'notparsed'
        self.maximum = 'notparsed'
        
    def set_default(self):
        self.valid = 'false'
        self.writeAsRead = 'false'
        self.useEnumeratedValues = 'false'
        self.minimum = '0'
        self.maximum = '0'


class EnumeratedValue(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.value = 'notparsed'
        
    def set_default(self):
        self.name = ''
        self.value = '0'


class Field(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.bitOffset = 'notparsed'
        self.typeIdentifier = 'notparsed'
        self.bitWidth = 'notparsed'
        self.volatile = 'notparsed'
        self.access = 'notparsed'
        self.enumerated_values = []
        self.modifiedWriteValue = 'notparsed'
        self.writeValueConstraint = WriteValueConstraint()        
        self.readAction = 'notparsed'
        self.testable = 'notparsed'
        self.vendorExtensions = VendorExtensions()
    
    def set_default(self):
        self.name = ''
        self.bitOffset = '0'
        self.typeIdentifier = 'none'
        self.bitWidth = '1'
        self.volatile = 'false'
        self.access = 'read-write'
        self.modifiedWriteValue = 'none'        
        self.readAction = 'none'
        self.testable = 'true'
        
    def import_values(self, element):
        Node.import_values(self, element)
        enumerated_values = element.find(spirit_ns + 'enumeratedValues')
        if enumerated_values is not None:
            enumerated_value_list = enumerated_values.findall(spirit_ns + 'enumeratedValue')
            for enumerated_value in enumerated_value_list:
                self.enumerated_values.append(EnumeratedValue())
                self.enumerated_values[-1].import_values(enumerated_value)


class AlternateRegister(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.alternate_groups = []                
        self.typeIdentifier = 'notparsed'
        self.volatile = 'notparsed'
        self.access = 'notparsed'
        self.reset = Reset()
        self.fields = []        
        self.vendorExtensions = VendorExtensions()
    
    def set_default(self):
        self.name = ''
        self.typeIdentifier = 'none'
        self.volatile = 'false'
        self.access = 'read-write' 
        
    def import_values(self, element):
        Node.import_values(self, element)
        alternate_groups = element.find(spirit_ns + 'alternateGroups')
        if alternate_groups is not None:
            alternate_group_list = alternate_groups.findall(spirit_ns + 'alternateGroup')
            for alternate_group in alternate_group_list:
                self.alternate_groups.append(alternate_group.text)


class Register(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.dim = 'notparsed'
        self.addressOffset = 'notparsed'
        self.typeIdentifier = 'notparsed'
        self.size = 'notparsed'
        self.volatile = 'notparsed'
        self.access = 'notparsed'
        self.reset = Reset()
        self.fields = []
        self.alternate_registers = []
        self.vendorExtensions = VendorExtensions()
    
    def set_default(self):
        self.name = ''
        self.dim = '1'
        self.addressOffset = '0'
        self.typeIdentifier = 'none'
        self.size = '8'
        self.volatile = 'false'
        self.access = 'read-write'
        
    def import_values(self, element):
        Node.import_values(self, element)
        alternate_registers = element.find(spirit_ns + 'alternateRegisters')
        if alternate_registers is not None:
            alternate_register_list = alternate_registers.findall(spirit_ns + 'alternateRegister')
            for alternate_register in alternate_register_list:
                self.alternate_registers.append(AlternateRegister())
                self.alternate_registers[-1].import_values(alternate_register)      
        
        
class RegisterFile(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.dim = 'notparsed'
        self.addressOffset = 'notparsed'
        self.typeIdentifier = 'notparsed'
        self.range = 'notparsed'
        self.register_files = []
        self.registers = []
        self.vendorExtensions = VendorExtensions()
    
    def set_default(self):
        self.name = ''
        self.dim = '1'
        self.addressOffset = '0'
        self.typeIdentifier = 'none'
        self.range = '0'


class MemOffset():
    def __init__(self, column = 0, row = 0):
        self.column = column
        self.row = row
    def assume(self, offset):
        self.column = offset.column
        self.row = offset.row
    def tosize(self, adr_unit_bits):
        width = self.column*adr_unit_bits
        range = self.row*self.column
        size = MemSize(width, range)
        return size        


class MemSize():
    def __init__(self, width = 0, range = 0):
        self.width = width
        self.range = range
    def assume(self, offset):
        self.width = offset.width
        self.range = offset.range
    def tocoord(self, adr_unit_bits):
        column = self.width/adr_unit_bits
        if column == 0:
            row = self.range
        else:
            row = self.range/column
            if self.range%column:       #update range in case of memory not completely filling a square (block)
                row += 1
        offset = MemOffset(column, row)
        return offset
        

class Memory():
    def __init__(self, offset = MemOffset(), size = MemSize()):
        self.offset = MemOffset()
        self.size = MemSize()
        self.offset.assume(offset)
        self.size.assume(size)
        
    def assume(self, memory):
        self.offset.assume(memory.offset)
        self.size.assume(memory.size)
        
    def update_size(self, size, adr_unit_bits, alignment):
        size_coord = self.size.tocoord(adr_unit_bits)
        extcoord = size.tocoord(adr_unit_bits)
        if alignment == 'serial':
            if extcoord.column > size_coord.column:
                size_coord.column = extcoord.column
            size_coord.row += extcoord.row
        if alignment == 'parallel':
            if extcoord.row > size_coord.row:
                size_coord.row = extcoord.row
            size_coord.column += extcoord.column
        result_size = size_coord.tosize(adr_unit_bits)
        self.size.assume(result_size)
        
    def next_offset(self, adr_unit_bits, alignment):
        offset = MemOffset()
        offset.assume(self.offset)
        size_coord = self.size.tocoord(adr_unit_bits)
        if alignment == 'serial':
            offset.row += size_coord.row        #update for non-complete squared blocks done by tocoord()
        if alignment == 'parallel':
            offset.column += size_coord.column
        return offset        
        

class AddressBlock(Node):
    def __init__(self, memory = Memory()):
        self.name = 'notparsed'
        self.base_addr = 'notparsed'
        
        self.memory = Memory()
        self.memory.assume(memory)
        
        self.typeIdentifier = 'notparsed'        
        self.usage = 'notparsed'
        self.volatile = 'notparsed'
        self.access = 'notparsed'
        
        self.register_files = []        
        self.registers = []
        
    def set_default(self):
        self.name = ''
        self.base_addr = '0'
        self.typeIdentifier = 'none'        
        self.usage = 'reserved'
        self.volatile = 'false'
        self.access = 'read-write'        
        
    def import_values(self, element):
        Node.import_values(self, element)
        self.memory.size.width = int(element.findtext(spirit_ns + 'width'), 0)
        self.memory.size.range = int(element.findtext(spirit_ns + 'range'), 0)
        self._get_base_address(element)
        
    def _get_base_address(self, element):
        base_adr = element.findtext(spirit_ns + 'baseAddress')
        if base_adr is not None:
            self.base_addr = base_adr
                
    def has_register(self):                    
        if self.registers or self.register_files:
            return True
        else:
            return False        


class Bank(Node):
    def __init__(self, alignment = 'serial', memory = Memory()):
        self.name = 'notparsed'
        self.base_addr = 'notparsed'
        
        self.alignment = alignment
        self.memory = Memory()
        self.memory.assume(memory)
        
        self.usage = 'notparsed'
        self.volatile = 'notparsed'
        self.access = 'notparsed'        
        
        self.banks = []        
        self.address_blocks = []
        
    def set_default(self):
        self.name = ''
        self.base_addr = '0'
        self.usage = 'reserved'
        self.volatile = 'false'
        self.access = 'read-write'
        
    def import_values(self, element, address_unit_bits):
        Node.import_values(self, element)
        self._get_base_address(element)
        self.generate_bank(element, address_unit_bits)        
        
    def _get_base_address(self, element):
        base_adr = element.findtext(spirit_ns + 'baseAddress')
        if base_adr is not None:
            self.base_addr = base_adr
                    
    def _update_size(self, memory, address_unit_bits):
        self.memory.update_size(memory.size, address_unit_bits, self.alignment)
    
    def _next_offset(self, address_unit_bits):
        return self.memory.next_offset(address_unit_bits, self.alignment)
    
    def generate_bank(self, element, address_unit_bits):
        items = element.getchildren()
        for item in items:
            if item.stripped_tag == 'addressBlock':
                block_memory = Memory(self._next_offset(address_unit_bits))
                block = AddressBlock(block_memory)
                block.import_values(item)
                self._update_size(block.memory, address_unit_bits)
                self.address_blocks.append(block)
            elif item.stripped_tag == 'bank':
                next_bank_alignment = item.get(spirit_ns + 'bankAlignment')
                next_bank = Bank(next_bank_alignment, Memory(self._next_offset(address_unit_bits)))
                next_bank.generate_bank(item, address_unit_bits)
#                self.address_blocks.extend(next_bank.address_blocks)
                self._update_size(next_bank.memory, address_unit_bits)
                self.banks.append(next_bank)
        return self
    
    def get_register_blocks(self):
        register_blocks = []
        for block in self.address_blocks:
            if block.has_register():
                register_blocks.append(block)
        for bank in self.banks:
            register_blocks.extend(bank.get_register_blocks())
        return register_blocks


class MemoryMap(Node):
    def __init__(self):
        self.name = 'notparsed'
        self.address_unit_bits = 0
        self.address_blocks = []
        self.banks = []
        
        self.defaults_updated = False
        self.hierarchy_updated = False        

    def set_default(self):
        self.name = ''

    def import_values(self, element):
        if isinstance(element, SpiritElement):
            Node.import_values(self, element)
            unitbits = element.findtext(spirit_ns + 'addressUnitBits')
            if unitbits:
                self.address_unit_bits = int(unitbits, 0)
            else:
                self.address_unit_bits = 8
            self.instantiate_memory(element)
        else:
            raise TypeError('Input element is not of type SpiritElement')

    def instantiate_memory(self, element):
        self.address_blocks = []
        self.banks = []        
        items = element.getchildren()
        for item in items:
            if item.stripped_tag == 'addressBlock':
                adr_block = AddressBlock()
                adr_block.import_values(item)
                self.address_blocks.append(adr_block)
            elif item.stripped_tag == 'bank':
                bank_alignment = item.get(spirit_ns + 'bankAlignment')
                bank = Bank(bank_alignment)
                bank.import_values(item, self.address_unit_bits)
                self.banks.append(bank)
                
    def get_banks(self):
        return self.banks
        
    def get_register_blocks(self):
        register_blocks = []
        for block in self.address_blocks:
            if block.has_register():
                register_blocks.append(block)
        for bank in self.banks:
            blocks = bank.get_register_blocks()
            register_blocks.extend(blocks)
        return register_blocks
            

class SpiritParser():
    def __init__(self):
        self.parser = etree.XMLParser(attribute_defaults=True)
        self.parser.set_element_class_lookup(SpiritLookup())
        
        module_dir = sys.path[0]
        schema = path.join(module_dir, "xact", "memoryMap.xsd")
        xmlschema_doc = etree.parse(schema)
        self.xmlschema = etree.XMLSchema(xmlschema_doc)
        
        self.memory_map = MemoryMap()
        self.document = None
        
    def parse(self, document):
        self.document = etree.parse(document, self.parser)        

    def validate(self):
        try:
            self.xmlschema.assertValid(self.document)
        except etree.DocumentInvalid:
            print "Input XML document is invalid for Spirit Schema"
            raise
        else:
            print "Input XML document is valid for Spirit Schema"
            
    def generate_memory_map(self):
        memory_map_element = self.document.find(spirit_ns + "memoryMap")        
        self.memory_map.import_values(memory_map_element)
        
    def get_memory_map(self):
        return self.memory_map


Node.node_classes['register_files'] = {'tag': spirit_ns + 'registerFile', 'node_class': RegisterFile}
Node.node_classes['registers'] = {'tag': spirit_ns + 'register', 'node_class': Register}
Node.node_classes['fields'] = {'tag': spirit_ns + 'field', 'node_class': Field}        