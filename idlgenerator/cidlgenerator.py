'''
Created on Mar 22, 2012

@author: raul
'''

from lxml import etree
from lxml import objectify

import cgen
import idlgen

idl_struct = {}
idl_struct['type'] = 'DevMech'
idl_struct['instance'] = 'devmech'
idl_struct['arg'] = ' * ' + idl_struct['instance']
idl_struct['ptr'] = idl_struct['instance'] + '->'

efsm_struct = {}
efsm_struct['type'] = 'struct efsm'
efsm_struct['instance'] = 'efsm'
efsm_struct['arg'] = ' * ' + efsm_struct['instance']
efsm_struct['ptr'] = efsm_struct['instance'] + '->'

out_types = {'value': 'unsigned int *',
         'stream': 'uint8_t *',
         'callback': 'enum cb_ret',
         'context': 'void *'}

in_types = {'value': 'unsigned int',
         'stream': 'uint8_t *',
         'callback': 'enum cb_ret',
         'context': 'void *'}

return_types = {'idlError': 'enum idl_error'}

idldebug = 'DEVCONTRACT_DEBUG'


def translatePseudoC(cmd):
    cmd = cmd.replace(' and ', ' && ')
    cmd = cmd.replace(' lt ', ' < ')
    cmd = cmd.replace(' le ', ' <= ')
    return cmd


class TransitionsResolver(cgen.NameResolver):
    def check_clashes(self, transitions):
        for transition in transitions:
            self.check_block_clash(transition)


class NamesResolver():
    def __init__(self):
        self.transitions_resolver = TransitionsResolver()
    
    def populate(self, mechanismIdl):            
        self.transitions_resolver.check_clashes(mechanismIdl.extendedFiniteStateMachine.behavior.transitions.transition)


class StateMachineHeaderGenerator(idlgen.StateMachineGenerator):
    def generate_machine(self, declaration):
        self.assemble_line('enum states')
        self.assemble_line('{')
        
        self.indent()
        for state in declaration.states.state:
            self.assemble_line(state + ',')
        self.dedent()
            
        self.assemble_line('};')
        self.new_line()
        
        self.assemble_line(efsm_struct['type'])
        self.assemble_line('{')
        
        self.indent()
        self.assemble_line('enum states state;')
        self.new_line()
        for variable in declaration.findall('.//variable'):
            self.assemble_line(in_types['value'] + ' ' + variable + ';')
        self.new_line()
        self.assemble_line('STATEMUTEX mutex;')
        self.dedent()
        
        self.assemble_line('};')
    
    def generate_start(self, start):
        self.new_line()
        self.assemble_line(efsm_struct['type'] + ' * init_efsm(void);')
        self.assemble_line('void stop_efsm(' + efsm_struct['type'] + efsm_struct['arg'] + ');')
        
    def generate(self, extendedFiniteStateMachine):
        self.assemble_line('//description of the extended finite state machine')
        idlgen.StateMachineGenerator.generate(self, extendedFiniteStateMachine)
        self.assemble_line('//~description of the extended finite state machine')
        self.new_line()
        return self.code


class InterfaceHeaderGenerator(idlgen.InterfaceGenerator):
    def generate_operation(self, operation):
        name = operation.get("name")
        parameters = []
        result = []
        if hasattr(operation, 'parameters'):
            for parameter in operation.parameters.iterchildren():
                parameters.append({})
                parameters[-1]['name'] = parameter.get('name')
                type = parameter.get("type")
                if parameter.tag == 'in':
                    if type in in_types:
                        parameters[-1]['type'] = in_types[parameter.get("type")]
                    else:
                        parameters[-1]['type'] = parameter.get("type")
                else:
                    if type in out_types:
                        parameters[-1]['type'] = out_types[parameter.get("type")]
                    else:
                        parameters[-1]['type'] = parameter.get("type") + ' *'
                        
        result_text = [ op.get("type") for op in operation.iterchildren(tag='return') ]
        for res in result_text:
            if res in return_types:
                result.append(return_types[res])
            else:
                if not res:
                    result.append('void')
                else:
                    result.append(res)
            
        parameters_string = idl_struct['type'] + idl_struct['arg']
        delimiter = ', '
        for parameter in parameters:
            if parameter['type'] == in_types['callback'] or parameter['type'] == out_types['callback']:
                single_paramter_string = parameter['type'] + '(*' + parameter['name'] + ')(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets)'
            else:
                single_paramter_string = parameter['type'] + ' ' + parameter['name']
            if parameters_string:
                parameters_string = parameters_string + delimiter + single_paramter_string
            else:
                parameters_string = single_paramter_string
        
        self.assemble_line("{0} {1}({2});".format(result[-1], name, parameters_string))
        
    def generate(self, interface):
        self.new_line()
        idlgen.InterfaceGenerator.generate(self, interface)
        return self.code


class HeaderGenerator(idlgen.Generator):
    def generate(self, mechanismIdl):
        self.assemble_line('#ifndef ' + mechanismIdl.get('name').upper() + '_H_')
        self.assemble_line('#define ' + mechanismIdl.get('name').upper() + '_H_')
        self.new_line()
        
        self.assemble_line('#include "devcontract.h"')
        for resource in mechanismIdl.resource:
            self.assemble_line('#include "' + resource + '"')
        self.new_line()
        self.new_line()
        
        idlgen.Generator.generate(self, mechanismIdl)
        self.new_line()
        self.new_line()
        
        self.assemble_line('#endif')

        return self.code


class StateMachineSourceGenerator(idlgen.StateMachineGenerator):
    def isVar(self, text):
        if text in self.variables:
            return True
        else:
            return False
    
    def getCGuard(self, guard_text):
        guard = translatePseudoC(guard_text)
        guard_list = []
        for word in guard.split():
            if self.isVar(word):
                guard_list.append(idl_struct['ptr'] + efsm_struct['ptr'] + word)
            else:
                guard_list.append(word)
        return ' '.join(guard_list)
    
    def transition(self, transition):
        if hasattr(transition, 'update'):
            for cmd in transition.update.cmd:
                self.assemble_line(idl_struct['ptr'] + efsm_struct['ptr'] + translatePseudoC(cmd.text) + ';')
        self.assemble_line(idldebug + '("<< ' + 'transition' + cgen.NameResolver.get_name(transition).capitalize() + '(): state updated to ' + transition.get('to') + '\\n");')
        self.assemble_assignment(idl_struct['ptr'] + efsm_struct['ptr'] + 'state', transition.get('to'))
    
    def generate_transition(self, transition):
        self.assemble_line('void ' + 'transition' + cgen.NameResolver.get_name(transition).capitalize() + '(' + idl_struct['type'] + idl_struct['arg'] + ')')
        self.assemble_line('{')
        self.indent()

        self.assemble_line(idldebug + '(">> ' + 'transition' + cgen.NameResolver.get_name(transition).capitalize() + '() called!\\n");')       
        self.assemble_line('if (' + idl_struct['ptr'] + efsm_struct['ptr'] + 'state == ' + transition.get('from') + ')')
        self.assemble_line('{')
        self.indent()
        
        if hasattr(transition, 'guard'):
            self.assemble_line('if ( ' + self.getCGuard(transition.guard.text) + ' )')
            self.assemble_line('{')
            self.indent()
            self.transition(transition)
            self.dedent()
            self.assemble_line('}')

            self.assemble_line('else')
            self.assemble_line('{')
            self.indent()
            self.assemble_line(idldebug + '("' + 'transition' + cgen.NameResolver.get_name(transition).capitalize() + '(): guard evaluated false\\n");')
            self.assemble_line(idldebug + '("Guard rule: ' + self.getCGuard(transition.guard.text) + '\\n");')
            for word in transition.guard.text.split():
                if self.isVar(word):
                    self.assemble_line(idldebug + '("    ' + word + ': %x' + '\\n", ' + idl_struct['ptr'] + efsm_struct['ptr'] + word + ');')
            self.assemble_line(idldebug + '("<< ' + 'transition' + cgen.NameResolver.get_name(transition).capitalize() + '() skipped\\n");')
            self.dedent()
            self.assemble_line('}')

        else:
            self.transition(transition)
            
        self.dedent()
        self.assemble_line('}')
        
        self.assemble_line('else')
        self.assemble_line('{')
        self.indent()
        self.assemble_line(idldebug + '("' + 'transition' + cgen.NameResolver.get_name(transition).capitalize() + '(): only valid from state ' + transition.get('from') + ' but current state is %u\\n", ' + idl_struct['ptr'] + efsm_struct['ptr'] + 'state' +  ');')
        self.assemble_line(idldebug + '("<< ' + 'transition' + cgen.NameResolver.get_name(transition).capitalize() + '() skipped\\n");')
        self.dedent()
        self.assemble_line('}')
            
        self.dedent()
        self.assemble_line('}')
        self.new_line()
    
    def generate_start(self, start):
        self.new_line()
        self.assemble_line(efsm_struct['type'] + ' * init_efsm(void)')
        self.assemble_line('{')
        self.indent()
        self.assemble_assignment(efsm_struct['type'] + efsm_struct['arg'], 'ALLOC(sizeof(' + efsm_struct['type'] + '))')
        self.assemble_assignment(efsm_struct['ptr'] + 'state', start.state)
        for cmd in start.variables.cmd:
            self.assemble_line(efsm_struct['ptr'] + translatePseudoC(cmd.text) + ';')
        self.assemble_line('INIT_MUTEX(&' + efsm_struct['ptr'] + 'mutex);')
        self.assemble_line('return ' + efsm_struct['instance'] + ';')
        self.dedent()
        self.assemble_line('}')
        self.new_line()
        
        self.assemble_line('void stop_efsm(' + efsm_struct['type'] + efsm_struct['arg'] + ')')
        self.assemble_line('{')
        self.indent()
        self.assemble_line('DEST_MUTEX(&' + efsm_struct['ptr'] + 'mutex);')
        self.assemble_line('FREE(' + efsm_struct['instance'] + ');')
        self.dedent()
        self.assemble_line('}')
        
    def generate(self, extendedFiniteStateMachine):
        self.new_line()
        self.assemble_line('//description of the extended finite state machine')
        self.variables = [ variable.text for variable in extendedFiniteStateMachine.declaration.variables.variable ]
        idlgen.StateMachineGenerator.generate(self, extendedFiniteStateMachine)
        self.assemble_line('//~description of the extended finite state machine')
        self.new_line()
        self.new_line()
        return self.code


class InterfaceSourceGenerator(idlgen.InterfaceGenerator):
    def down_lock(self, operation):
        if self.has_statemachine_update(operation):
            self.assemble_line('DOWN_WRITE(&' + idl_struct['ptr'] + efsm_struct['ptr'] + 'mutex);')
        elif self.has_preconditions(operation):
            self.assemble_line('DOWN_READ(&' + idl_struct['ptr'] + efsm_struct['ptr'] + 'mutex);')
    
    def up_lock(self, operation):
        if self.has_statemachine_update(operation):
            self.assemble_line('UP_WRITE(&' + idl_struct['ptr'] + efsm_struct['ptr'] + 'mutex);')
        elif self.has_preconditions(operation):
            self.assemble_line('UP_READ(&' + idl_struct['ptr'] + efsm_struct['ptr'] + 'mutex);')
    
    def _has_precondition(self, current_operation, precondition):
        if precondition.get('mode') == 'state' or precondition.get('mode') == 'variable':
            return True
        else:
            return False
            
    def gen_precondition(self, current_operation, precondition):
        if self._has_precondition(current_operation, precondition):
            if precondition.get('mode') == 'state':
                self.assemble_line('if ( ' + idl_struct['ptr'] + efsm_struct['ptr'] + 'state != ' + precondition.text + ' )')
                self.assemble_line('{')
                self.indent()
                self.assemble_line(idldebug + '("' + current_operation.get('name') + '(): ' + precondition.get('mode') + ' preCondition' + ' failed! ' + 'Expected state, ' + precondition.text + ', differs from current state: %u' + '\\n", ' + idl_struct['ptr'] + efsm_struct['ptr'] + 'state' + ');')
                self.up_lock(current_operation)
                self.assemble_line('return INVALID_STATE;')
                self.dedent()
                self.assemble_line('}')
            elif precondition.get('mode') == 'variable':
                self.assemble_line('if ( !( ' + idl_struct['ptr'] + efsm_struct['ptr'] + translatePseudoC(precondition.text) + ' ) )')
                self.assemble_line('{')
                self.indent()
                self.assemble_line(idldebug + '("' + current_operation.get('name') + '(): ' 
                                   + precondition.get('mode') + ' preCondition' + ' failed! ' + 'Expected ' + 
                                   translatePseudoC(precondition.text).split()[0] + ' value: ' + 
                                   translatePseudoC(precondition.text).split()[-1] + ', differs from current value: %u' + '\\n", ' + 
                                   idl_struct['ptr'] + efsm_struct['ptr'] + translatePseudoC(precondition.text).split()[0] + ');')
                self.up_lock(current_operation)
                self.assemble_line('return INVALID_STATE;')
                self.dedent()
                self.assemble_line('}')
        else:
            print '!!Precondition mode: ' + precondition.get('mode') + ' with value: ' + precondition.text + ' not supported!!'
            self.assemble_line('//!!Precondition mode: ' + precondition.get('mode') + ' with value: ' + precondition.text + ' not supported!!')
    
    def _for_precondition(self, current_operation, func):
        has = False
        if hasattr(current_operation, 'preCondition'):
            for precondition in current_operation.preCondition:
                if not has:
                    has = func(current_operation, precondition)
                else:
                    func(current_operation, precondition)
        return has
    
    def generate_preconditions(self, current_operation):
        self._for_precondition(current_operation, self.gen_precondition)
                
    def has_preconditions(self, current_operation):
        return self._for_precondition(current_operation, self._has_precondition)
                  
    def gen_postcondition(self, current_operation, postcondition):
        if postcondition.get('mode') == 'timing':
            self.assemble_line('MSLEEP(' + postcondition.text + ');')
        else:
            print '!!Postcondition mode: ' + postcondition.get('mode') + ' with value: ' + postcondition.text + ' not supported!!'
            self.assemble_line('//!!Postcondition mode: ' + postcondition.get('mode') + ' with value: ' + postcondition.text + ' not supported!!')
    
    def gen_assignment(self, assignment):
        self.assemble_line(idl_struct['ptr'] + efsm_struct['ptr'] + translatePseudoC(assignment.cmd.text) + ';')
        self.assemble_line(idldebug + '("' + assignment.cmd.text.split()[0] + ' <- ' + '%x\\n", ' + idl_struct['ptr'] + efsm_struct['ptr'] + assignment.cmd.text.split()[0] + ');')
        
    def gen_transition(self, transition):
        self.assemble_line('transition' + cgen.NameResolver.get_name(transition).capitalize() + '(' + idl_struct['instance'] + ');')
        
    def _run_closure(self, current_operation, func):
        transitions = []
        assignments = []
        for assignment in self.variables_assignments:
            if assignment.event == current_operation.get('name'):
                assignments.append(assignment)
        for trigger in self.transitions_trigger:
            for event in trigger.event:
                if event == current_operation.get('name'):
                    transitions.append(trigger.getparent())
        return func(current_operation, transitions, assignments)
        
    def _has_function_closure(self, current_operation, transitions, assignments):
        if hasattr(current_operation, 'postCondition') or len(assignments) or len(transitions):
            return True
        else:
            return False
        
    def _generate_function_closure(self, current_operation, transitions, assignments):
        if self._has_function_closure(current_operation, transitions, assignments):
            self.assemble_line('if ( ret == 0 )')
            self.assemble_line('{')
            self.indent()
            if hasattr(current_operation, 'postCondition'):
                for postcondition in current_operation.postCondition:    
                    self.gen_postcondition(current_operation, postcondition)
            if assignments:
                self.assemble_line(idldebug + '(">> ' + current_operation.get('name') + '(): updating state machine variables\\n");')
                for assignment in assignments:
                    self.gen_assignment(assignment)
                self.assemble_line(idldebug + '("<< ' + current_operation.get('name') + '(): state machine variables updated\\n");')
            for transition in transitions:
                self.gen_transition(transition)
            self.dedent()
            self.assemble_line('}')
            
    def generate_closure(self, current_operation):
        self._run_closure(current_operation, self._generate_function_closure)
        
    def _has_statemachine_update(self, current_operation, transitions, assignments):
        if len(assignments) or len(transitions):
            return True
        else:
            return False
        
    def has_statemachine_update(self, current_operation):
        return self._run_closure(current_operation, self._has_statemachine_update)
    
    def generate_operation(self, operation):
        name = operation.get("name")
        internal_function = operation.internal
        parameters = []
        result = []
        if hasattr(operation, 'parameters'):
            for parameter in operation.parameters.iterchildren():
                parameters.append({})
                parameters[-1]['name'] = parameter.get('name')
                type = parameter.get("type")
                if parameter.tag == 'in':
                    if type in in_types:
                        parameters[-1]['type'] = in_types[parameter.get("type")]
                    else:
                        parameters[-1]['type'] = parameter.get("type")
                else:
                    if type in out_types:
                        parameters[-1]['type'] = out_types[parameter.get("type")]
                    else:
                        parameters[-1]['type'] = parameter.get("type") + ' *'
            
        result_text = [ op.get("type") for op in operation.iterchildren(tag='return') ]
        for res in result_text:
            if res in return_types:
                result.append(return_types[res])
            else:
                if not res:
                    result.append('void')
                else:
                    result.append(res)
        
        parameters_string = idl_struct['type'] + idl_struct['arg']
        delimiter = ', '
        for parameter in parameters:
            if parameter['type'] == in_types['callback'] or parameter['type'] == out_types['callback']:
                single_paramter_string = parameter['type'] + '(*' + parameter['name'] + ')(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets)'
            else:
                single_paramter_string = parameter['type'] + ' ' + parameter['name']
            if parameters_string:
                parameters_string = parameters_string + delimiter + single_paramter_string
            else:
                parameters_string = single_paramter_string
                
        arg_string = idl_struct['instance']
        for parameter in parameters:
            single_arg_string = parameter['name']
            if arg_string:
                arg_string = arg_string + delimiter + single_arg_string
            else:
                arg_string = single_arg_string
        
        self.assemble_line(str(result[-1]) + ' ' + name + '(' + parameters_string + ') {')
        
        self.indent()
        
        self.assemble_line(str(result[-1]) + ' ret;')

        self.down_lock(operation)
        
        self.generate_preconditions(operation)

        self.assemble_line('ret = ' + internal_function + '(' + arg_string + ');')
        
        self.generate_closure(operation)
        
        self.up_lock(operation)
        
        self.assemble_line('return ret;')
        
        self.dedent()
        
        self.assemble_line('}')
        self.assemble_line('EXPORT_SYMBOL_GPL(' + name + ');')
        self.new_line()
        
    def generate(self, interface):
        self.variables_assignments = interface.getparent().extendedFiniteStateMachine.behavior.assignments.findall('.//assignment')
        self.transitions_trigger = interface.getparent().extendedFiniteStateMachine.behavior.transitions.findall('.//trigger')
        return idlgen.InterfaceGenerator.generate(self, interface)


class SourceGenerator(idlgen.Generator):
    def generate(self, mechanismIdl):
        self.assemble_line('#include "' + mechanismIdl.get('name') + '.h"')
        self.new_line()
        
        self.assemble_line('#include <devif/devif.h>')
        self.assemble_line('#include <usbif/usbif.h>')
        self.new_line()
        
        for resource in mechanismIdl.internalResource:
            self.assemble_line('#include "' + resource + '"')
        self.new_line()
        
        return idlgen.Generator.generate(self, mechanismIdl)


class FilesGenerator:    
    '''Functions working on register_files are recursive'''
    def __init__(self):
        self.names_resolver = NamesResolver()
        
        self.header_generator = HeaderGenerator(StateMachineHeaderGenerator(), InterfaceHeaderGenerator())
        self.source_generator = SourceGenerator(StateMachineSourceGenerator(), InterfaceSourceGenerator())
        
        self.files = {}
    
    def gen_code(self, mechanismIdl):
        self.files = {}
        
        self.names_resolver.populate(mechanismIdl)
        
        self.files['name'] = mechanismIdl.get('name')
        self.files['hdr'] = self.header_generator.generate(mechanismIdl)
        self.files['src'] = self.source_generator.generate(mechanismIdl)
            
    
class IdlCodeGenerator:
    def __init__(self):
        self.code_generator = FilesGenerator()
        
    def generate_code(self, document):
        tree = objectify.parse(document)
        mechanismIdl = tree.getroot()
        self.code_generator.gen_code(mechanismIdl)
        
    def get_files(self):
        return self.code_generator.files
    
