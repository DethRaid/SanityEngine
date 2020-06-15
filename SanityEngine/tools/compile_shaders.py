#!/bin/python3

# Compiles shaders from HLSL to DXIL

# First argument is the directory of the shaders to compile, second argument is the directory to output shaders to

import os
import sys
import subprocess

from pathlib import Path

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Provided args: "{}"', str(sys.argv))
        print('Usage: compile_shaders.py <HLSL directory> <output directory>')
        exit(1)

    hlsl_directory = str(sys.argv[1])
    dxil_directory = str(sys.argv[2])

    if not os.path.exists(dxil_directory):
        os.makedirs(dxil_directory)

    print('HLSL Directory:', hlsl_directory, 'DXIL directory:', dxil_directory)

    hlsl_paths = Path(hlsl_directory).glob('**/*.hlsl')

    any_failed = False
    any_processed = False

    for path in hlsl_paths:
        any_processed = True
        print('Compiling', str(path))

        dxil_filename = path.stem
        dxil_path = dxil_directory + '/' + dxil_filename

        if str(path).endswith('.pixel.hlsl'):
            target = 'ps_6_5'

        elif str(path).endswith('.vertex.hlsl'):
            target = 'vs_6_5'

        elif str(path).endswith('.compute.hlsl'):
            target = 'cs_6_5'

        else:
            print('Could not determine shading stage for shader `', str(path), '`, skipping')
            continue

        output = subprocess.run(['dxc', '-E', 'main', '-T', target, str(path), '-Fo', dxil_path], capture_output=True, text=True)
        print(output.stdout)
        if output.returncode != 0:
            print('Could not compile shader ', str(path), ': ', output.stderr)
            any_failed = True
        else:
            print('Successfully compiled ', str(path))

    if not any_processed:
        print('No shaders found in the HLSL directory')

    if any_failed:
        exit(2)
