#!/bin/python3

# Compiles shaders from HLSL to DXIL

# First argument is the directory of the shaders to compile, second argument is the directory to output shaders to

import os
import sys
import subprocess

from pathlib import Path

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: compile_shaders.py <HLSL directory> <output directory>')
        exit(1)

    hlsl_directory = str(sys.argv[1])
    dxil_directory = str(sys.argv[2])

    if not os.path.exists(dxil_directory):
        os.makedirs(dxil_directory)

    hlsl_paths = Path(hlsl_directory).glob('**/*.hlsl')

    any_failed = False

    for path in hlsl_paths:
        dxil_filename = path.stem + '.dxbc'
        dxil_path = dxil_directory + '/' + dxil_filename

        if str(path).endswith('.pixel.hlsl'):
            target = 'ps_6_0'

        elif str(path).endswith('.vertex.hlsl'):
            target = 'vs_6_0'

        else:
            print('Could not determine shading stage for shader `%s`, skipping', str(path))
            continue

        output = subprocess.run(['dxc', '-E', 'main', '-T', target, str(path), '-Fo', dxil_path], capture_output=True, text=True)
        print(output.stdout)
        if output.returncode != 0:
            print('Could not compile shader ', str(path), ': ', output.stderr)
            any_failed = True
        else:
            print('Successfully compiled ', str(path))

    if any_failed:
        exit(2)
