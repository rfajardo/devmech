'''
Created on Mar 22, 2012

@author: raul
'''

import cgen


class StateMachineGenerator(cgen.Generator):
    def generate_start(self, start):
        pass
    
    def generate_machine(self, declaration):
        pass
    
    def generate_transition(self, transition):
        pass
    
    def generate(self, extendedFiniteStateMachine):
        self.generate_machine(extendedFiniteStateMachine.declaration)
        
        for transition in extendedFiniteStateMachine.behavior.transitions.transition:
            self.generate_transition(transition)
            
        self.generate_start(extendedFiniteStateMachine.start)
        
        return cgen.Generator.generate(self)


class InterfaceGenerator(cgen.Generator):
    def generate_operation(self, operation):
        pass
    
    def generate(self, interface):
        for operation in interface.operation:
            self.generate_operation(operation)
        return cgen.Generator.generate(self)
    

class Generator(cgen.Generator):
    def __init__(self, stateMachineGen, interfaceGen):
        cgen.Generator.__init__(self)
        self.state_machine_gen = stateMachineGen
        self.interface_gen = interfaceGen
    
    def generate(self, mechanismIdl):
        self.code += self.state_machine_gen.generate(mechanismIdl.extendedFiniteStateMachine)
        self.code += self.interface_gen.generate(mechanismIdl.interface)
        return cgen.Generator.generate(self)
    
