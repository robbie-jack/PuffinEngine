import os
import argparse
import subprocess
from pathlib import Path
from glob import glob

glsl_validator = os.environ.get("VULKAN_SDK") + "/Bin/glslangValidator.exe"
# glsl_validator = "/usr/bin/glslangValidator"
valid_extensions = { ".frag": "_fs.spv", ".vert": "_vs.spv" }

def build_shader(shader_path, output_path):

    print("Compiling shader " + shader_path)

    subprocess.call([glsl_validator, "-V", shader_path, "-o", output_path])

    print("Shader binary output to " + output_path)

    pass

def build_shaders(shader_dir, output_dir, recurse):

    shaders = []

    # Get all valid files in directory    
    for file in os.listdir(shader_dir):

        shader_path = os.path.join(shader_dir, file)

        file_path, file_extension = os.path.splitext(shader_path)
        file_name = os.path.basename(file_path)

        folder_name = os.path.basename(shader_dir)

        if file_extension in valid_extensions:

            output_path = output_dir + "\\" + file_name + valid_extensions.get(file_extension)

            shaders.append((shader_path, output_path))

    # If there were any valid files, build them
    if shaders:

        os.makedirs(output_dir, exist_ok=True)

        for shader in shaders:

            build_shader(shader[0], shader[1])

    # If recurse is true, build shaders for subfolders as well
    if recurse == True:

        p = Path(shader_dir)

        subfolders = [f for f in p.iterdir() if f.is_dir()]

        for subfolder in subfolders:
            
            build_shaders(subfolder, os.path.join(output_dir, subfolder.name), recurse)

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--shader_path", type=str)
parser.add_argument("-o", "--output_path", type=str)
parser.add_argument("-r", "--recurse", action="store_true")

args = parser.parse_args()

file_path, file_extension = os.path.splitext(args.shader_path)

if file_extension:

    file_name = os.path.basename(file_path)

    output_path = args.output_path + "\\" + file_name + valid_extensions.get(file_extension)
    
    build_shader(args.shader_path, output_path)
    
else:

    build_shaders(args.shader_path, args.output_path, args.recurse)
