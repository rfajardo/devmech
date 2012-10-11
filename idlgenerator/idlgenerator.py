#!/usr/bin/env python
'''
Created on Mar 5, 2012

@author: raul
'''

import sys
from os import path

import cidlgenerator

if __name__ == '__main__':
    print 'This program works now with a document based on the IDL schema.'
    if len(sys.argv) == 1:
        print "This program requires the input of an XML document compliant with IDL XML Schema."
    else:
        document = sys.argv[1]
        if document[-4:] != '.xml':
            document += '.xml'
            
        codegenerator = cidlgenerator.IdlCodeGenerator()
        codegenerator.generate_code(document)
        
        files = codegenerator.get_files()
        
        module_dir = sys.path[0]
        place = path.join(module_dir, "output")
        
        hdr = path.join(place, files['name'] + '.h')
        src = path.join(place, files['name'] + '.c')
        with open(hdr, 'w') as file:
            file.write(files['hdr'])
        with open(src, 'w') as file:
            file.write(files['src'])
            
