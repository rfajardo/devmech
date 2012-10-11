'''
Created on Mar 22, 2012

@author: raul
'''


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
    
    def new_line(self):
        return self.assemble_line('')
    
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
        if block.name.text in self.clashing_blocks:
            self.block_clashes[block.name.text] += 1
            NameResolver.name_clash_resolver[block] = str(self.block_clashes[block.name.text])
        else:
            self.clashing_blocks.append(block.name.text)
            self.block_clashes[block.name.text] = 1
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
    

class Generator(LineFormatter):
    def __init__(self):
        LineFormatter.__init__(self)
        self.code = str()
        
    def assemble_line(self, line):
        self.code += LineFormatter.assemble_line(self, line)
          
    def generate(self):
        return self.code
    
