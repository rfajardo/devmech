'''
Created on Dec 13, 2010

@author: rfajardo
'''

import spiritparser

import string


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


class RegisterGenerator(LineFormatter):
    def __init__(self, name_resolver):
        LineFormatter.__init__(self)
        
        self.name_resolver = name_resolver

        self.blacklist = []
        self.renamedlist = {}
        self.renamedValue = {}
        
        self.blacklist.append('name')
        self.blacklist.append('typeIdentifier')
        
        self.renamedlist['volatile'] = 'is_volatile'
        
        self.renamedValue['read-write'] = 'read_write'
        self.renamedValue['read-only'] = 'read_only'
        self.renamedValue['write-only'] = 'write_only'
        
    def gen_enum(self, enumerated_value):
        code = ''
        scope = self.stored_prefix
        for i in range(2):
            index = []
            index.append(scope.rfind('.'))
            index.append(scope.rfind('-'))
            scope = scope[:max(index)]
        scope = scope.replace('.', '_')
        predicate = 'GETNUM(' + scope + ', ' + self.name_resolver.get_name(enumerated_value) + ')'
        code += self.assemble_line('add_enumvalue(tmp_field, ' + predicate + ');')
        return code
    
    def gen_enum_list(self, enum_list):
        code = ''
        if enum_list:
            code += self.assemble_line('')
            self.push_prefix('tmp_enum')
            for enum in enum_list:
                code += self.gen_enum(enum)
            code += self.assemble_line('')
            self.pop_prefix()
        return code
        
    def gen_std_element(self, element):
        code = ''
        for attr in element.__dict__:
            value = element.__dict__[attr]
            if isinstance(value, str):
                if value in self.renamedValue:
                    value = self.renamedValue[value]
                if attr not in self.blacklist:                    
                    if attr in self.renamedlist:
                        code += self.assemble_assignment_prefix(self.renamedlist[attr], value)
                    else:
                        code += self.assemble_assignment_prefix(attr, value)
        return code
    
    def gen_field(self, field):
        code = ''
        
        code += self.assemble_line('')
        code += self.assemble_assignment('tmp_field', 'alloc_field()')
        
        self.reset_prefix()
        self.push_ptr_prefix('tmp_field')
        
        code += self.assemble_assignment_prefix("dirty", "false")
        
        code += self.gen_std_element(field)
        
        self.push_prefix("writeValueConstraint")
        code += self.gen_std_element(field.writeValueConstraint)
        self.pop_prefix()
            
        self.push_prefix("vendorExtensions")
        self.push_prefix("strobe")
        code += self.gen_std_element(field.vendorExtensions.strobe)
        self.pop_prefix()
        self.pop_prefix()
        
        code += self.gen_enum_list(field.enumerated_values)        
        
        self.restore_prefix()
        self.prefix = self.prefix.rstrip('->')
        code += self.assemble_line('add_field(' + self.prefix + ', ' + 'tmp_field);')
        
        return code
    
    def gen_field_list(self, field_list):
        code = ''
        self.store_prefix()
        for field in field_list:
            code += self.gen_field(field)
        self.restore_prefix()
        code += self.assemble_line('')
        return code
        
    def gen_alt_register(self, alt_register, parent_register):
        code = ''
        
        code += self.assemble_assignment_prefix("dim", parent_register.dim)
        code += self.assemble_assignment_prefix("addressOffset", parent_register.addressOffset)
               
        code += self.gen_std_element(alt_register)
        
        code += self.assemble_assignment_prefix("size", parent_register.size)
        
        self.push_prefix("reset")
        code += self.gen_std_element(alt_register.reset)
        self.pop_prefix()
        
        self.push_prefix("vendorExtensions")
        self.push_prefix("strobe")
        code += self.gen_std_element(alt_register.vendorExtensions.strobe)
        self.pop_prefix()         
        self.pop_prefix()
        
        code += self.gen_field_list(alt_register.fields)
            
        return code        
        
    def gen_register(self, register, name):
        code = ''
        self.push_ptr_prefix(name)
        code += self.gen_std_element(register)
        
        self.push_prefix("reset")
        code += self.gen_std_element(register.reset)
        self.pop_prefix()
            
        self.push_prefix("vendorExtensions")
        self.push_prefix("strobe")
        code += self.gen_std_element(register.vendorExtensions.strobe)
        self.pop_prefix()
        self.pop_prefix()
        
        code += self.gen_field_list(register.fields) 
        
        self.pop_ptr_prefix()
        code += self.assemble_line('')
        
        for altreg in register.alternate_registers:
            self.push_ptr_prefix(altreg.name)
            code += self.gen_alt_register(altreg, register)
            self.pop_ptr_prefix()
            code += self.assemble_line('')
        
        return code


class NameResolver():
    def __init__(self):
        self.clashing_blocks = []
        self.clashing_variables = []
        self.clashing_enumerations = []
        
        self.address_block_clashes = {}
        self.name_clash_resolver = {}
    
    def check_block_clash(self, block):    
        if block.name in self.clashing_blocks:
            self.address_block_clashes[block.name] += 1
            self.name_clash_resolver[block] = str(self.address_block_clashes[block.name])
        else:
            self.clashing_blocks.append(block.name)
            self.address_block_clashes[block.name] = 1
            self.name_clash_resolver[block] = ''        
        
    def check_variable_clash(self, variable, parent):
        '''Every node is called only once.'''
        if variable.name in self.clashing_variables:
            self.name_clash_resolver[variable] = self.get_name(parent) + '_'
        else:
            self.clashing_variables.append(variable.name)                
 
    def check_enum_clash(self, enum, parent):
        '''Every node should be called only once, but register_file is called a second time here, so 
        get_name assures that the adapted name is kept and clash on that still avoided by changing name of others if necessary'''
        if self.get_name(enum) in self.clashing_enumerations:
            self.name_clash_resolver[enum] = self.get_name(parent) + '_'
        else:
            self.clashing_enumerations.append(enum.name)
            
    def check_altgroup_clash(self, altgroup, parent):
        if altgroup in self.clashing_enumerations:
            self.name_clash_resolver[altgroup] = self.get_name(parent) + '_'
        else:
            self.clashing_enumerations.append(altgroup)
            
    def check_field_clash(self, field, parent):
        self.check_variable_clash(field, parent)
        for enum in field.enumerated_values:
            self.check_enum_clash(enum, field)
            
    def check_register_clash(self, register, parent):
        self.check_variable_clash(register, parent)
        for field in register.fields:
            self.check_field_clash(field, register)
        for altreg in register.alternate_registers:
            self.check_variable_clash(altreg, register)
            for altgroup in altreg.alternate_groups:
                self.check_altgroup_clash(altgroup, altreg)
            for field in altreg.fields:
                self.check_field_clash(field, altreg)                
            
    def check_register_file_clash(self, register_file, parent):
        self.clashing_variables = []
        self.clashing_enumerations = []

        for register in register_file.registers:
            self.check_register_clash(register, register_file)
            
        for int_register_file in register_file.register_files:
            self.check_variable_clash(int_register_file, register_file)
            self.check_enum_clash(int_register_file, register_file)
            
        for int_register_file in register_file.register_files:
            self.check_register_file_clash(int_register_file, register_file)            
            
    def check_address_block_clash(self, address_block):
        self.clashing_variables = []
        self.clashing_enumerations = []
        
        self.check_block_clash(address_block)
            
        for register in address_block.registers:
            self.check_register_clash(register, address_block)
            
        for register_file in address_block.register_files:
            self.check_variable_clash(register_file, address_block)

        for register_file in address_block.register_files:
            self.check_register_file_clash(register_file, address_block)
   
                
    def get_name(self, node):
        '''Mainly, this function checks if the name is added to be resolved. If it is,
        the name is adapted by prepending the parent node name to the ordinary name of 
        the node. 
        Address block is resolved differently, by adding numbers on name clashes, so this
        function has to check for the AddressBlock type.
        Furthermore, due to altgroup, which is a string and has to be checked for name clashes, 
        this function has to check if the input node is of string type. If yes, name 
        cannot be used, instead the node itself has the name content.''' 
        node_type = node.__class__.__name__
        if node in self.name_clash_resolver:
            if node_type == 'AddressBlock':
                name = node.name + self.name_clash_resolver[node]
                name = string.lower(name)            
            elif node_type == str:                    #altgroup
                name = self.name_clash_resolver[node] + node
            elif node in self.name_clash_resolver:
                name = self.name_clash_resolver[node] + node.name
        else:
            if node_type == str:                    #altgroup
                name = node
            else:                        
                name = node.name
        return name
    
    def populate(self, memory_map):
        self.clashing_blocks = []
        
        self.address_block_clashes = {}
        self.name_clash_resolver = {}
        
        register_blocks = memory_map.get_register_blocks()
        
        for register_block in register_blocks:
            self.check_address_block_clash(register_block)
            
            
class HeaderGenerator(LineFormatter):
    def __init__(self, name_resolver):
        LineFormatter.__init__(self)
        self.name_resolver = name_resolver
        
    def gen_field_enum(self, register_file):
        code = ''
        for register in register_file.registers:
            for field in register.fields:
                if field.enumerated_values:
                    code += self.assemble_line('enum { //' + self.name_resolver.get_name(field) + '_enum')
                    self.indent()
                    for enum_value in field.enumerated_values:
                        code += self.assemble_line('SCOPENUM(' + self.prefix + self.name_resolver.get_name(enum_value) + ') = ' + enum_value.value + ',')
                        
                    self.dedent()
                    code = code.rstrip(',\n')
                    code += self.assemble_line('')
                    code += self.assemble_line('} ' + self.name_resolver.get_name(field) + '_enum;')
                    code += self.assemble_line('')
            
        return code
  
    @staticmethod
    def get_alt_groups_list(address_block):
        '''Since address_block and register_files have both registers and register_files, 
        they can be interchangeably used in this function'''
        alt_groups = []
        for register in address_block.registers:
            for alt_reg in register.alternate_registers:
                for alt_group in alt_reg.alternate_groups:
                    if alt_group not in alt_groups:
                        alt_groups.append(alt_group)
            
        return alt_groups
        
    def gen_alt_groups_active(self, address_block):
        '''Since address_block and register_files have both registers and register_files, 
        they can be interchangeably used in this function'''        
        code = ''
        
        alt_groups = HeaderGenerator.get_alt_groups_list(address_block)
            
        if alt_groups:
            for alt_group in alt_groups:
                code += self.assemble_line('bool * ' + alt_group + 'GroupActive;')
                
            code += self.assemble_line('')
 
        return code
 
    def gen_register_file_active(self, address_block):
        '''Since address_block and register_files have both registers and register_files, 
        they can be interchangeably used in this function'''        
        code = ''
        
        if address_block.register_files:
            for register_file in address_block.register_files:
                code += self.assemble_line('bool * ' + self.name_resolver.get_name(register_file) + 'RegFileActive;')
    
        code += self.assemble_line('')
        
        return code
    
    def gen_active_enum(self, scope, address_block):
        '''Since address_block and register_files have both registers and register_files, 
        they can be interchangeably used in this function'''          
        code = ''
        
        code += self.gen_register_file_active(address_block)
      
        code += self.gen_alt_groups_active(address_block)
        
        enum_field = False
        for register in address_block.registers:
            for field in register.fields:
                if field.enumerated_values:
                    enum_field = True

        if enum_field:
            code += self.assemble_line('#undef SCOPE')    #avoid warnings
            code += self.assemble_line('#define SCOPE ' + scope)        
        
        code += self.gen_field_enum(address_block)
        
        return code        
               
    def gen_register_pointers(self, register):
        code = ''
        code += self.assemble_line('Reg * ' + self.name_resolver.get_name(register) + ';')
        for alt_reg in register.alternate_registers:
            code += self.assemble_line('Altreg * ' + self.name_resolver.get_name(alt_reg) + ';')
            for field in alt_reg.fields:
                code += self.assemble_line('RegField * ' + self.name_resolver.get_name(field) + ';')           
        for field in register.fields:
            code += self.assemble_line('RegField * ' + self.name_resolver.get_name(field) + ';')
        code += self.assemble_line('')
        return code
        
    def gen_register_file_pointers(self, register_file, scope):   
        code = ''
        code += self.assemble_line_prefix('struct { //%s\n' % self.name_resolver.get_name(register_file))
        
        self.indent()
        
        for register in register_file.registers:
            code += self.gen_register_pointers(register)
            
        scope += '_' + self.name_resolver.get_name(register_file)
        code += self.gen_active_enum(scope, register_file)
        
        code += self.assemble_line('Mem * ' + self.prefix + 'cache;')
        
        code += self.assemble_line('')
        
        for int_regfile in register_file.register_files:
            code += self.gen_register_file_pointers(int_regfile, scope)
        
        self.dedent()
            
        code += self.assemble_line_prefix('} ' + self.name_resolver.get_name(register_file) + ';\n')        
        
        return code
    
    def gen_address_block_header(self, address_block):
        code = ''
        
        code += self.assemble_line('struct %s {' % self.name_resolver.get_name(address_block))
        
        self.indent()
        
        code += self.assemble_line('//interface data')
        code += self.assemble_line('DevCom * handle;        //this handle has to take the first position of the structure')
        code += self.assemble_line('bool trueGroup;')
        code += self.assemble_line('//~interface data')
        code += self.assemble_line('')
        
        for register in address_block.registers:
            code += self.gen_register_pointers(register)
            
        scope = self.name_resolver.get_name(address_block)
        code += self.gen_active_enum(scope, address_block)
        
        code += self.assemble_line('Mem * ' + self.prefix + 'cache;')    
        
        code += self.assemble_line('')    
            
        for register_file in address_block.register_files:
            code += self.gen_register_file_pointers(register_file, scope)
            
        self.dedent()
        
        code += self.assemble_line('};\n')
            
        return code        
        
    def generate(self, address_block):
        code = ''
        code += self.assemble_line('#ifndef ' + self.name_resolver.get_name(address_block) + '_H_')
        code += self.assemble_line('#define ' + self.name_resolver.get_name(address_block) + '_H_')
        code += self.assemble_line('')
            
        code += self.assemble_line('#include <devif/regdata.h>')
        code += self.assemble_line('')
        code += self.assemble_line('')
        
        if BindGenerator.require_binding(address_block):
            code += self.assemble_line('struct usbcom;')
            code += self.assemble_line('')
            code += self.assemble_line('')        
        
        code += self.gen_address_block_header(address_block)
        
        code += self.assemble_line('struct ' + self.name_resolver.get_name(address_block) + ' * ifinit_' + self.name_resolver.get_name(address_block) + '(void);')
        code += self.assemble_line('void ifstop_' + self.name_resolver.get_name(address_block) + '(struct ' + self.name_resolver.get_name(address_block) + ' * ' + self.name_resolver.get_name(address_block) + ');')
        
        if BindGenerator.require_binding(address_block):
            code += self.assemble_line('')
            code += self.assemble_line('void ifbind_' + self.name_resolver.get_name(address_block) + '(struct ' + self.name_resolver.get_name(address_block) + ' * ' + self.name_resolver.get_name(address_block) + ', struct usbcom * usbcom);\n')
        
        code += self.assemble_line('#endif //' + self.name_resolver.get_name(address_block) + '_H_')
        
        return code
        
        
class AllocGenerator(LineFormatter):
    def __init__(self, name_resolver, block_conformer):
        LineFormatter.__init__(self)
        self.name_resolver = name_resolver
        self.block_conformer = block_conformer
    
    def gen_regfile_memory_maps(self, register_file):
        code = ''
        code += self.assemble_assignment_prefix('cache', 'alloc_memory(' + str(register_file.range) + ')')
        
        for index in range(int(register_file.range)):
            code += self.assemble_assignment('(' + self.prefix + 'cache+' + str(index) + ')->if_addr', hex( self.block_conformer.getRegisterBankOffset(index) ) )
            
        code += '\n'
        
        for reg_file in register_file.register_files:
            self.push_prefix(self.name_resolver.get_name(reg_file))
            code += self.gen_regfile_memory_maps(reg_file)
            self.pop_prefix()
        
        return code                
        
    def gen_memory_maps(self, address_block):
        code = ''
        code += self.assemble_assignment_prefix('cache', 'alloc_memory(' + str(address_block.memory.size.range) + ')')
        
        for index in range(int(address_block.memory.size.range)):
            code += self.assemble_assignment('(' + self.prefix + 'cache+' + str(index) + ')->if_addr', hex( self.block_conformer.getRegisterBankOffset(index) ) )
            
        code += '\n'
        
        for reg_file in address_block.register_files:
            self.push_prefix(self.name_resolver.get_name(reg_file))
            code += self.gen_regfile_memory_maps(reg_file)
            self.pop_prefix()
        
        return code
        
    def gen_register_header_heap(self, register):
        code = ''
        
        code += self.assemble_assignment_prefix(self.name_resolver.get_name(register), 'alloc_register()')
        for alt_reg in register.alternate_registers:
            code += self.assemble_assignment_prefix(self.name_resolver.get_name(alt_reg), 'alloc_register()')
            for field in alt_reg.fields:
                code += self.assemble_assignment_prefix(self.name_resolver.get_name(field), 'alloc_regfield()')           
        for field in register.fields:
            code += self.assemble_assignment_prefix(self.name_resolver.get_name(field), 'alloc_regfield()')
        code += '\n'
        return code
    
    def gen_header_heap(self, address_block):
        code = ''

        for register in address_block.registers:
            code += self.gen_register_header_heap(register)
            
        for regfile in address_block.register_files:
            self.push_prefix(self.name_resolver.get_name(regfile))
            code += self.gen_header_heap(regfile)
            self.pop_prefix()
            
        return code
        
    def gen_active_heap(self, address_block):
        '''Since address_block and register_files have both registers and register_files, 
        they can be interchangeably used in this function'''        
        code = ''
        alt_groups = HeaderGenerator.get_alt_groups_list(address_block)
            
        if alt_groups:
            for alt_group in alt_groups:
                code += self.assemble_assignment_prefix(alt_group + 'GroupActive', 'ALLOC(sizeof(bool))')
            
        for register_file in address_block.register_files:
            code += self.assemble_assignment_prefix(self.name_resolver.get_name(register_file) + 'RegFileActive', 'ALLOC(sizeof(bool))')
                
        for regfile in address_block.register_files:
            self.push_prefix(self.name_resolver.get_name(regfile))
            code += self.gen_active_heap(regfile)
            self.pop_prefix()

        return code        
        
    def generate(self, address_block):
        code = ''
        
        code += self.assemble_line('struct ' + self.name_resolver.get_name(address_block) + ' * alloc_' + self.name_resolver.get_name(address_block) + '(void)')
        code += self.assemble_line('{')
        
        self.push_ptr_prefix(self.name_resolver.get_name(address_block))
        self.indent()
        
        code += self.assemble_assignment('struct ' + self.name_resolver.get_name(address_block) + ' * ' + self.name_resolver.get_name(address_block), 'ALLOC(sizeof(struct ' + self.name_resolver.get_name(address_block) + '))')
        code += self.assemble_line('')
        
        code += self.assemble_assignment_prefix('handle', 'alloc_com()')
        code += self.assemble_line('')
                
        code += self.gen_active_heap(address_block)
        code += self.assemble_line('')
        
        code += self.gen_header_heap(address_block)
        
        code += self.gen_memory_maps(address_block)
        
        code += self.assemble_line('return ' + self.name_resolver.get_name(address_block) + ';')
        
        self.dedent()
        self.pop_ptr_prefix()
        
        code += self.assemble_line('}')
        
        return code


class BankOffsetVisitor():
    def __init__(self):
        self.base_addr = 0
        self.address_unit_bits = 0
        self.block_origin = spiritparser.MemOffset()
        self.block_size_coord = spiritparser.MemOffset()
        self.bank_size_coord = spiritparser.MemOffset()
        
    def getRegisterBankOffset(self, register_offset):
        register_column = register_offset%self.block_size_coord.column
        register_row = register_offset/self.block_size_coord.column
        
        bank_register_offset_column = register_column + self.block_origin.column 
        bank_register_offset_row = register_row + self.block_origin.row
        
        bank_register_offset = bank_register_offset_column + bank_register_offset_row*self.bank_size_coord.column
        return bank_register_offset + self.base_addr
    
    def update(self, memory_map, address_block):
        self.address_unit_bits = memory_map.address_unit_bits
        
        self.base_addr = int(address_block.base_addr, 0)
        self.block_origin.assume(address_block.memory.offset)
        self.block_size_coord.assume(address_block.memory.size.tocoord(self.address_unit_bits))
        
        self.bank_size_coord.assume(address_block.memory.size.tocoord(self.address_unit_bits))
        banks = memory_map.get_banks()
        for bank in banks:
            register_blocks = bank.get_register_blocks()
            for register_block in register_blocks:
                if register_block is address_block:
                    self.bank_size_coord.assume(bank.memory.size.tocoord(self.address_unit_bits))

class PopulateGenerator(LineFormatter):
    def __init__(self, name_resolver, block_conformer):
        LineFormatter.__init__(self)
        self.name_resolver = name_resolver
        self.register_generator = RegisterGenerator(name_resolver)
        self.block_conformer = block_conformer
    
    def gen_address_block(self, address_block):
        code = ''

        for register in address_block.registers:
            code += self.register_generator.gen_register(register, self.name_resolver.get_name(register))
            
        code += self.assemble_line('')
        code += self.assemble_line('')
        
        for regfile in address_block.register_files:
            self.register_generator.push_prefix(self.name_resolver.get_name(regfile))
            code += self.gen_address_block(regfile)
            self.register_generator.pop_prefix()

        return code
    
    def gen_active_heap(self, address_block):
        '''Since address_block and register_files have both registers and register_files, 
        they can be interchangeably used in this function'''        
        code = ''

        alt_groups = HeaderGenerator.get_alt_groups_list(address_block)
            
        if alt_groups:
            for alt_group in alt_groups:
                code += self.assemble_line('*' + self.prefix + alt_group + 'GroupActive' + ' = ' + 'false;')
            
        if address_block.register_files:
            for register_file in address_block.register_files:
                code += self.assemble_line('*' + self.prefix + self.name_resolver.get_name(register_file) + 'RegFileActive' + ' = ' + 'false;')
                
        for register_file in address_block.register_files:
            self.push_prefix(self.name_resolver.get_name(register_file))
            code += self.gen_active_heap(register_file)
            self.pop_prefix()

        return code            
        
    def generate(self, address_block):
        code = ''
        
        name = self.name_resolver.get_name(address_block)
        
        code += self.assemble_line_prefix('void populate_' + name + '(struct ' + name + ' * ' + name + ')')
        code += self.assemble_line_prefix('{')
        
        self.push_ptr_prefix(name)
        self.register_generator.push_ptr_prefix(name)
        self.indent()
        
        code += self.assemble_line('Field * tmp_field;')
        code += self.assemble_line('')
        
        code += self.assemble_assignment_prefix('trueGroup', 'true')
        code += self.assemble_line('')        
  
        code += self.gen_active_heap(address_block)
        code += self.assemble_line('')
        
        self.register_generator.indent()
        
        code += self.gen_address_block(address_block)
        
        self.register_generator.dedent()
        
        self.dedent()
        self.register_generator.pop_ptr_prefix()
        self.pop_ptr_prefix()
        
        code += self.assemble_line_prefix('}')
        code += self.assemble_line('')
        code += self.assemble_line('')
        
        return code
        

class AssociateGenerator(LineFormatter):
    def __init__(self, name_resolver):
        LineFormatter.__init__(self)
        self.name_resolver = name_resolver
    
    def gen_field_srctail(self, field, register):
        code = ''
        code += self.assemble_line('associate_regfield(' + self.prefix + self.name_resolver.get_name(field) + ', ' + self.prefix + self.name_resolver.get_name(register) + ', ' + '((FieldListData*)fieldHook->data)->regField->field' + ');')
        code += self.assemble_assignment('fieldHook', 'fieldHook->next')           
        code += self.assemble_line('')
        return code
        
    def gen_register(self, register, parent_regfiles, parent_prefix):
        code = ''

        for index in range(len(parent_regfiles)):
            if index > 0:
                code += self.assemble_line('add_register_file_active(' + self.prefix + self.name_resolver.get_name(register) + ', ' + parent_prefix[index] + self.name_resolver.get_name(parent_regfiles[index]) + 'RegFileActive' + ');')
                    
        code += self.assemble_line('add_alternate_group_active(' + self.prefix + self.name_resolver.get_name(register) + ', &' + self.name + '->trueGroup' + ');')        

        code += self.assemble_line('associate_memory(' + self.prefix + self.name_resolver.get_name(register) + ', ' + self.prefix + 'cache+' + str(register.addressOffset) + ');')
        if register.reset.valid == 'true':
            code += self.assemble_assignment_prefix(self.name_resolver.get_name(register) + '->pcache->value', str( hex( int(register.reset.mask,0) & int(register.reset.value,0)) ) )
        code += self.assemble_line('')
        
        if register.fields:
            code += self.assemble_assignment('fieldHook', self.prefix + self.name_resolver.get_name(register) + '->allFields')
        for field in register.fields:
            code += self.gen_field_srctail(field, register)
        code += self.assemble_line('')
            
        for altreg in register.alternate_registers:
            for index in range(len(parent_regfiles)):
                if index > 0:
                    code += self.assemble_line('add_register_file_active(' + self.prefix + self.name_resolver.get_name(altreg) + ', ' + parent_prefix[index] + self.name_resolver.get_name(parent_regfiles[index]) + 'RegFileActive' + ');')

            altgroups = HeaderGenerator.get_alt_groups_list(parent_regfiles[-1])
            for index in range(len(altgroups)):
                code += self.assemble_line('add_alternate_group_active(' + self.prefix + self.name_resolver.get_name(altreg) + ', ' + self.prefix + altgroups[index] + 'GroupActive' + ');')
            
            code += self.assemble_line('associate_memory(' + self.prefix + self.name_resolver.get_name(altreg) + ', ' + self.prefix + 'cache+' + str(register.addressOffset) + ');')
            if altreg.reset.valid == 'true':
                code += self.assemble_assignment_prefix(self.name_resolver.get_name(altreg) + '->pcache->value', altreg.reset.mask + ' & ' + altreg.reset.value)
            code += self.assemble_line('')
            
            if altreg.fields:
                code += self.assemble_assignment('fieldHook', self.prefix + self.name_resolver.get_name(altreg) + '->allFields')
            for field in altreg.fields:
                code += self.gen_field_srctail(field, altreg)
            code += self.assemble_line('')
                
        return code
    
    def gen_address_block_srctail(self, register_file, parent_register_files = [], parent_prefix = []):
        code = ''
        
        for register in register_file.registers:
            code += self.gen_register(register, parent_register_files, parent_prefix)

        for regfile in register_file.register_files:
            parent_register_files.append(register_file)
            parent_prefix.append(self.prefix)
            self.push_prefix(self.name_resolver.get_name(regfile))
            code += self.gen_address_block_srctail(regfile, parent_register_files, parent_prefix)
            parent_register_files.pop()
            self.pop_prefix()
                        
        return code
        
    def generate(self, address_block):
        code = ''
        
        self.name = self.name_resolver.get_name(address_block)
        
        code += self.assemble_line('void associate_' + self.name + '(struct ' + self.name + ' * ' + self.name + ')')
        code += self.assemble_line('{')
        
        self.push_ptr_prefix(self.name_resolver.get_name(address_block))        
        self.indent()

        code += self.assemble_line('List * fieldHook;')
        code += self.assemble_line('')
        
        code += self.gen_address_block_srctail(address_block)
              
        self.dedent()
        self.pop_ptr_prefix()
        
        code += self.assemble_line('}')
        code += self.assemble_line('')
        code += self.assemble_line('')
        return code
    
    
class BindGenerator(LineFormatter):
    def __init__(self, name_resolver):
        LineFormatter.__init__(self)
        self.name_resolver = name_resolver
        
    @staticmethod
    def require_binding(address_block):
        for register in address_block.registers:
            if register.vendorExtensions.complexType.recvPacket != 'NULL':
                return True
        for register_file in address_block.register_files:
            if BindGenerator.require_binding(register_file):
                return True
        return False
        
    def gen_register(self, register):
        code = ''

        code += self.assemble_line('add_usb_data(' + self.prefix + self.name_resolver.get_name(register) + ', alloc_usb_data());')
        if register.vendorExtensions.complexType.recvPacket != 'NULL':
            code += self.assemble_assignment('((UsbData*)' + self.prefix + self.name_resolver.get_name(register) + '->if_complex_data)->recv_packet', 'usbcom->setup_packets.' + register.vendorExtensions.complexType.recvPacket)
        if register.vendorExtensions.complexType.sendPacket != 'NULL':
            code += self.assemble_assignment('((UsbData*)' + self.prefix + self.name_resolver.get_name(register) + '->if_complex_data)->send_packet', 'usbcom->setup_packets.' + register.vendorExtensions.complexType.sendPacket)
        code += self.assemble_line('')
                
        return code
    
    def gen_address_block_srctail(self, address_block):
        code = ''
        
        for register in address_block.registers:
            code += self.gen_register(register)

        for regfile in address_block.register_files:
            self.push_prefix(self.name_resolver.get_name(regfile))
            code += self.gen_address_block_srctail(regfile)
            self.pop_prefix()

        return code
        
    def generate(self, address_block):
        code = ''
        
        name = self.name_resolver.get_name(address_block)
        
        code += self.assemble_line('void ifbind_' + name + '(struct ' + name + ' * ' + name + ', struct usbcom * usbcom)')
        code += self.assemble_line('{')
        
        self.push_ptr_prefix(self.name_resolver.get_name(address_block))
        self.indent()
        
        code += self.gen_address_block_srctail(address_block)
        code += self.assemble_line('')
        
        code += self.assemble_line('usbif_bind_com(' + self.name_resolver.get_name(address_block) + ', usbcom);')
              
        self.dedent()
        self.pop_ptr_prefix()
        
        code += self.assemble_line('}')
        code += self.assemble_line('')
        code += self.assemble_line('')
        return code
    
    
class FreeGenerator(LineFormatter):
    def __init__(self, name_resolver):
        LineFormatter.__init__(self)
        self.name_resolver = name_resolver
        
    def gen_memory_maps(self, address_block):
        code = ''
        
        code += self.assemble_line('free_memory(' + self.prefix + 'cache);')
        
        for reg_file in address_block.register_files:
            self.push_prefix(self.name_resolver.get_name(reg_file))
            code += self.gen_memory_maps(reg_file)
            self.pop_prefix()
        
        return code        
        
    def gen_field_heap_memory(self, register, index):
        code = ''
        
        self.push_indexed_prefix('fields')
        
        if len(register.fields[index].enumerated_values):
            code += self.assemble_line('FREE(' + self.prefix + 'enumeratedValues);')
            
        self.pop_indexed_prefix()                     
        
        return code 
        
    def gen_register_header_heap(self, register):
        code = ''
        
        code += self.assemble_line('free_register(' + self.prefix + self.name_resolver.get_name(register) + ');')
        for alt_reg in register.alternate_registers:
            code += self.assemble_line('free_register(' + self.prefix + self.name_resolver.get_name(alt_reg) + ');')
            for field in alt_reg.fields:
                code += self.assemble_line('free_regfield(' + self.prefix + self.name_resolver.get_name(field) + ');')      
        for field in register.fields:
            code += self.assemble_line('free_regfield(' + self.prefix + self.name_resolver.get_name(field) + ');')
        code += self.assemble_line('')
        return code
    
    def gen_header_heap(self, address_block):
        code = ''
        
        for register in address_block.registers:
            code += self.gen_register_header_heap(register)
            
        for regfile in address_block.register_files:
            self.push_prefix(self.name_resolver.get_name(regfile))
            code += self.gen_header_heap(regfile)
            self.pop_prefix()
            
        return code        
        
    def gen_active_heap(self, address_block):
        '''Since address_block and register_files have both registers and register_files, 
        they can be interchangeably used in this function'''        
        code = ''

        alt_groups = HeaderGenerator.get_alt_groups_list(address_block)
            
        if alt_groups:
            for alt_group in alt_groups:
                code += self.assemble_line('FREE(' + self.prefix + alt_group + 'GroupActive);')
            
        if address_block.register_files:
            for register_file in address_block.register_files:
                code += self.assemble_line('FREE(' + self.prefix + self.name_resolver.get_name(register_file) + 'RegFileActive);')
                
        for register_file in address_block.register_files:
            self.push_prefix(self.name_resolver.get_name(register_file))
            code += self.gen_active_heap(register_file)            
            self.pop_prefix()

        return code        
        
    def generate(self, address_block):
        code = ''
        code += self.assemble_line('void free_' + self.name_resolver.get_name(address_block) + '(struct ' + self.name_resolver.get_name(address_block) + ' * ' + self.name_resolver.get_name(address_block) + ')')
        code += self.assemble_line('{')
        
        self.push_ptr_prefix(self.name_resolver.get_name(address_block))
        self.indent()
        
        code += self.assemble_line('FREE(' + self.prefix + 'handle);')
        code += self.assemble_line('')
        
        code += self.gen_active_heap(address_block)
        code += self.assemble_line('')
        
        code += self.gen_header_heap(address_block)
        
        code += self.gen_memory_maps(address_block)
        
        code += self.assemble_line('')
        code += self.assemble_line('FREE(' + self.name_resolver.get_name(address_block) + ');')
        
        self.dedent()
        self.pop_ptr_prefix()
        
        code += self.assemble_line('}')
        
        return code    
    
    

class SourceGenerator(LineFormatter):
    def __init__(self, name_resolver, block_conformer):
        LineFormatter.__init__(self)
        
        self.name_resolver = name_resolver
        self.block_conformer = block_conformer
        
        self.alloc_generator = AllocGenerator(self.name_resolver, self.block_conformer)
        self.populate_generator = PopulateGenerator(self.name_resolver, self.block_conformer)
        self.associate_generator = AssociateGenerator(self.name_resolver)
        self.free_generator = FreeGenerator(self.name_resolver)
        
        self.bind_generator = BindGenerator(self.name_resolver)
        
    def generate(self, address_block):
        #header
        code = ''
        code += self.assemble_line('#include "' + self.name_resolver.get_name(address_block) + '.h"')
        if BindGenerator.require_binding(address_block):
            code += self.assemble_line('#include "usbcom.h"')
        
        code += self.assemble_line('\n')
        
        code += self.assemble_line('#include <devif/com.h>')
        code += self.assemble_line('#include <usbif/usbif.h>')
        code += self.assemble_line('\n')
        
        code += self.alloc_generator.generate(address_block)
        
        code += self.assemble_line('\n')
        
        
        #body
        code += self.populate_generator.generate(address_block)
        
        #tail
        code += self.associate_generator.generate(address_block)
        
        #free
        code += self.free_generator.generate(address_block)
        code += self.assemble_line('\n')
        
        code += self.assemble_line('struct ' + self.name_resolver.get_name(address_block) + ' * ifinit_' + self.name_resolver.get_name(address_block) + '(void)')
        code += self.assemble_line('{')
        self.indent()
        code += self.assemble_line('struct ' + self.name_resolver.get_name(address_block) + ' * ' + self.name_resolver.get_name(address_block) + ';')
        code += self.assemble_assignment(self.name_resolver.get_name(address_block), 'alloc_' + self.name_resolver.get_name(address_block) + '();')
        code += self.assemble_line('populate_' + self.name_resolver.get_name(address_block) + '(' + self.name_resolver.get_name(address_block) + ');')
        code += self.assemble_line('associate_' + self.name_resolver.get_name(address_block) + '(' + self.name_resolver.get_name(address_block) + ');')
        code += self.assemble_line('return ' + self.name_resolver.get_name(address_block) + ';')
        self.dedent()
        code += self.assemble_line('}\n')

        code += self.assemble_line('void ifstop_' + self.name_resolver.get_name(address_block) + '(struct ' + self.name_resolver.get_name(address_block) + ' * ' + self.name_resolver.get_name(address_block) + ')')
        code += self.assemble_line('{')
        self.indent()
        code += self.assemble_line('free_' + self.name_resolver.get_name(address_block) + '(' + self.name_resolver.get_name(address_block) + ');')
        self.dedent()
        code += self.assemble_line('}\n')        
        
        if BindGenerator.require_binding(address_block):
            code += self.assemble_line('')
            code += self.bind_generator.generate(address_block)
            code += self.assemble_line('')
              
        return code


class FilesGenerator(LineFormatter):    
    '''Functions working on register_files are recursive'''
    def __init__(self):
        LineFormatter.__init__(self)
        
        self.name_resolver = NameResolver()
        self.block_conformer = BankOffsetVisitor()
        
        self.header_generator = HeaderGenerator(self.name_resolver)
        self.source_generator = SourceGenerator(self.name_resolver, self.block_conformer)
        
        self.files = []
    
    def gen_code(self, memory_map):
        self.register_files = []        
        self.files = []
        
        self.name_resolver.populate(memory_map)
        
        register_blocks = memory_map.get_register_blocks()
        for index in range(len(register_blocks)):
            self.block_conformer.update(memory_map, register_blocks[index])
            self.files.append({})
            self.files[index]['name'] = self.name_resolver.get_name(register_blocks[index])
            self.files[index]['src'] = self.source_generator.generate(register_blocks[index])
            self.files[index]['hdr'] = self.header_generator.generate(register_blocks[index])
            
    
class SpiritCodeGenerator():
    def __init__(self):
        self.code_generator = FilesGenerator()
        
    def generate_code(self, memory_map):
        self.code_generator.gen_code(memory_map)
        
    def get_files(self):
        return self.code_generator.files