#!/usr/bin/env python

'''
Created on Nov 8, 2010

@author: rfajardo
'''

import spiritparser
import spiritresolver
import spiritcodegenerator

import sys
from os import path

if __name__ == '__main__':
    print '''This program works now with a document based on the memoryMap schema.
    Restrictions:
        -Parameters are not supported. 
    Extensions: 
        -vendorExtensions: <spirit:strobe> and <spirit:complexType> supported, node structure:
            <spirit:vendorExtensions>
                <spirit:strobe>
                    <spirit:write>false</spirit:write>
                    <spirit:read>false</spirit:read>
                    <spirit:noAction>0x00</spirit:noAction>
                </spirit:strobe>
                <spirit:complexType>
                    <spirit:recvPacket>getReg</recvPacket>
                    <spirit:sendPacket>setReg</sendPacket>
                </spirit:complexType>
            </spirit:vendorExtensions>\n'''
    if len(sys.argv) == 1:
        print "This program requires the input of a XML document compliant with Spirit XML Schema."
    else:
        document = sys.argv[1]
        if document[-4:] != '.xml':
            document += '.xml'
            
            
        xmlparser = spiritparser.SpiritParser()
        resolver = spiritresolver.SpiritResolver()
        codegenerator = spiritcodegenerator.SpiritCodeGenerator()
        
        
        xmlparser.parse(document)
        xmlparser.validate()
        xmlparser.generate_memory_map()
        memory_map = xmlparser.get_memory_map()    
        
        resolver.update_type(memory_map)
        resolver.update_hierarchy(memory_map)
        resolver.update_defaults(memory_map)
        resolver.validate(memory_map)
        
        codegenerator.generate_code(memory_map)
        files = codegenerator.get_files()
        
        nr_of_files = len(files)
        print '\n%u header(s) and source(s) generated' % nr_of_files
        
        module_dir = sys.path[0]
        place = path.join(module_dir, "output")
        
        for i in range(len(files)):
            hdr = path.join(place, files[i]['name'] + '.h')
            src = path.join(place, files[i]['name'] + '.c')
            with open(hdr, 'w') as file:
                file.write(files[i]['hdr'])
            with open(src, 'w') as file:
                file.write(files[i]['src'])
                
