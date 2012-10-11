'''
Created on Nov 8, 2010

@author: rfajardo
'''

import spiritparser

import copy


class SpiritError(Exception):
    def __init__(self, text):
        self.text = text
        
    def __str__(self):
        return repr(self.text)
    
    
class Visitor(object):    
    def visit(self, node, *args, **kwargs):
        meth = None
        for cls in node.__class__.__mro__:
            meth_name = 'visit_'+cls.__name__
            meth = getattr(self, meth_name, None)
            if meth:
                break

        if not meth:
            meth = self.generic_visit
            
        return meth(node, *args, **kwargs)

    def generic_visit(self, node, *args, **kwargs):
        pass
    

class TypeDefinition():
    '''Types have to be saved independent from their type definers
    when the definer changes, the type has to remain unchanged
    the same applies to the targets, when target changes, type has to remain unchanged.
    
    As error example for not using deepcopy() you can use two registerFiles with typeIdentifier, 
    where one offset differs from the other. The hierarchical offset appliance will be applied twice.'''
    def __init__(self):
        self.typeIdentifier = 'notparsed'
        
    def import_type(self, node):
        if self.typeIdentifier == 'notparsed':
            for attr in self.__dict__:
                value = getattr(node, attr)
                setattr(self, attr, copy.deepcopy(value))
    
    def update_node(self, node):
        if self.typeIdentifier != 'notparsed':
            for attr in self.__dict__:
                value = getattr(self, attr)
                setattr(node, attr, copy.deepcopy(value))
    
    
class FieldDefinition(TypeDefinition):
    def __init__(self):
        TypeDefinition.__init__(self)
        self.bitWidth = 'notparsed'
        self.volatile = 'notparsed'
        self.access = 'notparsed'
        self.enumerated_values = []
        self.modifiedWriteValue = 'notparsed'
        self.writeValueConstraint = spiritparser.WriteValueConstraint()
        self.readAction = 'notparsed'
        self.testable = 'notparsed'
        
            
class AlternateRegisterDefinition(TypeDefinition):
    def __init__(self):                
        TypeDefinition.__init__(self)
        self.volatile = 'notparsed'
        self.access = 'notparsed'
        self.reset = spiritparser.Reset()
        self.fields = []    
        

class RegisterDefinition(TypeDefinition):
    def __init__(self):
        TypeDefinition.__init__(self)
        self.size = 'notparsed'
        self.volatile = 'notparsed'
        self.access = 'notparsed'
        self.reset = spiritparser.Reset()
        self.fields = []
        
        
class RegisterFileDefinition(TypeDefinition):        
    def __init__(self):
        TypeDefinition.__init__(self)
        self.range = 'notparsed'
        self.register_files = []
        self.registers = []


class AddressBlockDefinition(TypeDefinition):        
    def __init__(self):
        TypeDefinition.__init__(self)
        self.memory = spiritparser.Memory()
        self.usage = 'notparsed'
        self.volatile = 'notparsed'
        self.access = 'notparsed'
        self.register_files = []        
        self.registers = []
                

class TypeIdentifierVisitor(Visitor):
    '''This class substitutes elements with specified typeIdentifiers with the set type. 
    The first visited class set the type and the following classes of the same type are updated with its values.
    
    typeIdentifiers of addressBlocks defined on another memoryMap won't work.'''
    def __init__(self):
        self.typeIdentifiers = {}
        self.typeIdentifiers['Field'] = {'DefinitionClass': FieldDefinition}
        self.typeIdentifiers['AlternateRegister'] = {'DefinitionClass': AlternateRegisterDefinition}
        self.typeIdentifiers['Register'] = {'DefinitionClass': RegisterDefinition}
        self.typeIdentifiers['RegisterFile'] = {'DefinitionClass': RegisterFileDefinition}
        self.typeIdentifiers['AddressBlock'] = {'DefinitionClass': AddressBlockDefinition}
        self.subRegisterFiles = {} 
        
    def generic_visit(self, node, *args, **kwargs):
        key = node.__class__.__name__
        if key in self.typeIdentifiers:
            if node.typeIdentifier != 'notparsed':
                if node.typeIdentifier in self.typeIdentifiers[key]:
                    self.typeIdentifiers[key][node.typeIdentifier].update_node(node)                    
                else:
                    self.typeIdentifiers[key][node.typeIdentifier] = self.typeIdentifiers[key]['DefinitionClass']()
                    self.typeIdentifiers[key][node.typeIdentifier].import_type(node)
        
    def register_subRegisterFiles(self, type_identifier, register_files):
        sub_register_files = []
        for register_file in register_files:
            if register_file.typeIdentifier == type_identifier:
                sub_register_files.append(register_file)
            if register_file.register_files:
                sub_register_files.extend(self.register_subRegisterFiles(type_identifier, register_file.register_files))
        return sub_register_files
                    
    def visit_RegisterFile(self, node, *args, **kwargs):
        key = 'RegisterFile'
        if node.typeIdentifier != 'notparsed':
            if node.typeIdentifier in self.typeIdentifiers[key]:
                if node not in self.subRegisterFiles[node.typeIdentifier]:
                    self.typeIdentifiers[key][node.typeIdentifier].update_node(node)
                else:
                    print "\nWarning: Register File %s cannot import type %s" % (node.name, node.typeIdentifier)
                    print "This register file is a child from the type definer."
                    print "Defining this as the same as the parent will lead to infinite loop, since it will redefine itself.\n"                                                        
            else:
                self.typeIdentifiers[key][node.typeIdentifier] = self.typeIdentifiers[key]['DefinitionClass']()
                self.typeIdentifiers[key][node.typeIdentifier].import_type(node)
                self.subRegisterFiles[node.typeIdentifier] = self.register_subRegisterFiles(node.typeIdentifier, node.register_files)
                    
    def visit_AddressBlock(self, node, *args, **kwargs):
        key = 'AddressBlock'
        if node.typeIdentifier != 'notparsed':
            if node.typeIdentifier in self.typeIdentifiers[key]:
                self.typeIdentifiers[key][node.typeIdentifier].update_node(node)
                print "\nWarning: previously imported type %s being used as type for %s" % (node.typeIdentifier, node.name)                                    
            else:
                self.typeIdentifiers[key][node.typeIdentifier] = self.typeIdentifiers[key]['DefinitionClass']()
                self.typeIdentifiers[key][node.typeIdentifier].import_type(node)
                print "\nWarning: importing attributes of addressBlock %s as type %s:" % (node.name, node.typeIdentifier)
                print "This typeIdentifier might have been defined in another memoryMap."
                print "In that case, whatever was written in this addressBlock will be assumed to be the new type."                                  

    def update(self, memory_map):
        memory_map.walk(self)

    
class HierarchyVisitor(Visitor):
    def __init__(self):
        self.block_access = 'notparsed'
        self.bank_baseadr = 'notparsed'
        
    def update_addr(self, node, parents_addr_offset):
        if hasattr(node, 'addressOffset') and node.addressOffset != 'notparsed':
            own_addr = int(node.addressOffset, 0)
            new_addr = own_addr + int(parents_addr_offset, 0)
            node.addressOffset = hex(new_addr)
            
    def assume_access(self, node, parents_access):
        if hasattr(node, 'access') and node.access == 'notparsed':
            node.access = parents_access
            
    def import_hierarchical_values(self, node, parent):
        if hasattr(parent, 'addressOffset') and parent.addressOffset != 'notparsed':
            self.update_addr(node, parent.addressOffset)
        if hasattr(parent, 'access') and parent.access != 'notparsed':
            self.assume_access(node, parent.access)
            
    def visit_MemoryMap(self, node, *args, **kwargs):
        pass            
            
    def generic_visit(self, node, parent, *args, **kwargs):
        self.import_hierarchical_values(node, parent)
        
    def visit_Bank(self, node, parent, *args, **kwargs):
        '''When a second bank group containing base address of itself self.bank_baseadr
        will be overwritten producing the desired result.'''
        self.generic_visit(node, parent)
        if node.base_addr == 'notparsed':
            node.base_addr = self.bank_baseadr
        else:
            self.bank_baseadr = node.base_addr
        
    def visit_AddressBlock(self, node, parent, *args, **kwargs):
        '''Base address of address block is never 'notparsed' when address block exists
        out of a bank. Address blocks out of banks never contains internal address blocks 
        so these will not be overwritten mistakenly.'''
        self.generic_visit(node, parent)
        self.block_access = node.access
        if node.base_addr == 'notparsed':
            node.base_addr = self.bank_baseadr
        
    def visit_Register(self, node, parent, *args, **kwargs):
        self.generic_visit(node, parent)
        self.assume_access(node, self.block_access)
                
    def update(self, memory_map):
        if memory_map.defaults_updated:
            raise SpiritError('''Hierarchy update cannot be applied after update of defaults.
            That occurs because both check for the string 'notparsed' to see if they are allowed to change a field.''')
        
        if memory_map.hierarchy_updated:
            raise SpiritError('Hierarchy was already applied.')
        
        memory_map.walk(self)
                    
        memory_map.hierarchy_updated = True


class DefaultsVisitor(Visitor):
    def __init__(self):
        self.defaults = {}
        self.defaults['ComplexType'] = spiritparser.ComplexType.get_default()
        self.defaults['Strobe'] = spiritparser.Strobe.get_default()
        self.defaults['Reset'] = spiritparser.Reset.get_default()
        self.defaults['WriteValueConstraint'] = spiritparser.WriteValueConstraint.get_default()                
        self.defaults['Field'] = spiritparser.Field.get_default()
        self.defaults['AlternateRegister'] = spiritparser.AlternateRegister.get_default()
        self.defaults['Register'] = spiritparser.Register.get_default()
        self.defaults['RegisterFile'] = spiritparser.RegisterFile.get_default()
        self.defaults['AddressBlock'] = spiritparser.AddressBlock.get_default()
        self.defaults['Bank'] = spiritparser.Bank.get_default()
        self.defaults['MemoryMap'] = spiritparser.MemoryMap.get_default()
        
        self.regwidth = 0
        self.reset_value = 0
        self.reset_mask = 0
            
    def apply_default(self, element, default):
        valid = False
        for key in element.__dict__:
            if getattr(element, key) == 'notparsed':
                setattr(element, key, getattr(default, key))
            else:
                valid = True
        if valid: 
            if 'valid' in element.__dict__:
                element.valid = 'true'     
            
    def generic_visit(self, node, *args, **kwargs):
        node_type = node.__class__.__name__
        if node_type in self.defaults:
            self.apply_default(node, self.defaults[node_type])
            
    def visit_Register(self, node, *args, **kwargs):
        self.regwidth = int(node.size)
        self.reset_mask = 2**self.regwidth - 1
        self.generic_visit(node)
            
    def visit_Reset(self, node, *args, **kwargs):
        '''This is done explicitly because we cannot know the width of the register upfront.'''
        valid = False
        if node.valid == 'notparsed':
            node.valid = self.defaults['Reset'].valid
        if node.value == 'notparsed':
            node.value = hex(self.reset_value)
        else:
            valid = True
        if node.mask == 'notparsed':
            node.mask = hex(self.reset_mask)
        else:
            valid = True
        if valid: 
            if 'valid' in node.__dict__:
                node.valid = 'true'

    def update(self, memory_map):
        if not memory_map.hierarchy_updated:
            print '''Warning: hierarchical updates not applied yet, the appliance of defaults will blacklist the appliance of hierarchy updates.
            That occurs because both check for the string 'notparsed' to see if they are allowed to change a field.'''
        memory_map.walk(self)
        memory_map.defaults_updated = True

    
class RulesVisitor(Visitor):
    def __init__(self):
        self.register_size = 0
        self.block_size = spiritparser.MemSize()
        
    def register_width(self, register, mem_width):
        if int(register.size, 0) <= mem_width:
            pass
        else:
            raise SpiritError('Register %s\'s width is wider than the memory map addressable width' % register.name)
        
    def register_address(self, register, mem_range):
        if int(register.addressOffset, 0) < mem_range:
            pass
        else:
            raise SpiritError('Register %s\'s address is higher than the memory map address space' % register.name)
            
    def register_values(self, register):
        if register.reset.value != 'notparsed':
            if int(register.reset.value, 0) < 2**self.register_size:
                pass
            else:
                raise SpiritError('Register %s\'s reset value is higher than the register size allows' % register.name)        
        
    def field_position(self, field, regsize):
        if int(field.bitOffset, 0) + int(field.bitWidth, 0) <= regsize:
            pass
        else:
            raise SpiritError('Bit field %s\'s offset + width does not fit in its register' % field.name)                    
        
    def field_enumerations(self, field):
        for enumerated_value in field.enumerated_values:
            if int(enumerated_value.value, 0) < 2**int(field.bitWidth, 0):
                pass
            else:
                raise SpiritError('Bit field %s\'s enumeration %s do not not fit in field\'s width' % (field.name, enumerated_value.name))                            
        
    def field_values(self, field):
        if int(field.writeValueConstraint.minimum, 0) < 2**int(field.bitWidth, 0):
            pass
        else:   
            raise SpiritError('Bit field %s\'s writeValueConstraint minimum is higher than allowed by field\'s width' % field.name)                 
        
        if int(field.writeValueConstraint.maximum, 0) < 2**int(field.bitWidth, 0):
            pass
        else:
            raise SpiritError('Bit field %s\'s writeValueConstraint maximum is higher than allowed by field\'s width' % field.name)
        
        if int(field.writeValueConstraint.maximum, 0) >= int(field.writeValueConstraint.minimum, 0):
            pass
        else:
            raise SpiritError('Bit field %s\'s writeValueConstraint minimum is higher than its maximum' % field.name)    
    
    def visit_Register(self, register, *args, **kwargs):
        self.register_size = int(register.size, 0) 
        self.register_width(register, self.block_size.width)
        self.register_address(register, self.block_size.range)
        self.register_values(register)
    
    def visit_Field(self, field, *args, **kwargs):
        self.field_position(field, self.register_size)
        self.field_enumerations(field)
        self.field_values(field)
        
    def visit_AlternateRegister(self, alt_reg, *args, **kwargs):
        self.register_values(alt_reg)
        
    def visit_RegisterFile(self, register_file, *args, **kwargs):
        self.block_size.range = int(register_file.range)
        
    def visit_AddressBlock(self, address_block, *args, **kwargs):
        self.block_size.assume(address_block.memory.size)
        
    def validate(self, memory_map):
        register_blocks = memory_map.get_register_blocks()
        for register_block in register_blocks:
            try:
                register_block.walk(self)
            except SpiritError as error:
                print 'Address block %s did not pass following defined rule: %s' % (register_block.name, error.text)
                raise
            else:
                print 'Address block %s passed all defined rules' % register_block.name


class SpiritResolver():
    def __init__(self):
        self.type_importer = TypeIdentifierVisitor()
        self.hierarchy_controller = HierarchyVisitor()
        self.default_includer = DefaultsVisitor()
        self.rule_checker = RulesVisitor()
        
    def update_type(self, memory_map):
        self.type_importer.update(memory_map)
        
    def update_hierarchy(self, memory_map):
        self.hierarchy_controller.update(memory_map)
        
    def update_defaults(self, memory_map):
        self.default_includer.update(memory_map)

    def validate(self, memory_map):
        return self.rule_checker.validate(memory_map)
