#!/usr/bin/env python

'''
Created on Jul 19, 2011

@author: rfajardo
'''

import udevparser
import udevcodegenerator

import sys
from os import path

if __name__ == '__main__':
    print 'This program works now with a document based on the usbStandard schema.'
    if len(sys.argv) == 1:
        print "This program requires the input of a XML document compliant with Udev XML Schema."
    else:
        document = sys.argv[1]
        if document[-4:] != '.xml':
            document += '.xml'
            
            
        xmlparser = udevparser.UdevParser()
        codegenerator = udevcodegenerator.UdevCodeGenerator()
        
        xmlparser.parse(document)
        xmlparser.validate()
        xmlparser.generate_usbcom()
        usbcom = xmlparser.get_usbcom()    
        
        codegenerator.generate_code(usbcom)
        files = codegenerator.get_files()
        
        print '\nOne header and one source generated'
        
        module_dir = sys.path[0]
        place = path.join(module_dir, "output")
        
        hdr = path.join(place, files['name'] + '.h')
        src = path.join(place, files['name'] + '.c')
        with open(hdr, 'w') as file:
            file.write(files['hdr'])
        with open(src, 'w') as file:
            file.write(files['src'])
                
