"""
convert the tokens file in which each line is a token to a const char* array in C++ header file
"""
import sys, os

with open(sys.argv[1], 'r') as f:
    tokens = f.readlines()

tokens = [token.strip() for token in tokens]

with open(sys.argv[2], 'w') as f:
    f.write('#pragma once\n\n')
    f.write('static const char* tokens[] = {\n')
    for token in tokens:
        f.write('    "' + token + '",\n')
    f.write('};\n')

#print the log info to the console
print(f'convert tokens file done!, token number: {len(tokens)} and save to {sys.argv[2]}')