from pathlib import Path
import platform
import subprocess


SHADERS_DIR = Path.cwd() / "shaders"

# Shader file extensions, spv is ignored
SPIRV = {".spv", ".spirv", ".spir-v", ".spir_v"}
VERTEX = {".vsh", ".vert", ".vertex"}
TESS_CONT = {".tscsh", ".tesc", ".tesscont"}
TESS_EVAL = {".tsesh", ".tese", ".tesseval"}
GEOMETRY = {".gsh", ".geo", ".geometry"}
FRAGMENT = {".fsh", ".frag", ".fragment"}
COMPUTE = {".csh", ".comp", ".compute"}


def locate(binary: str) -> str:
    if platform.system() == "Windows":
        win_binary = f"{binary}.exe"

        try:
            path = subprocess.check_output(f"where {win_binary}", shell=True)
        except subprocess.CalledProcessError:
            raise Exception(f"Could not locate {win_binary}, make sure you have the Vulkan SDK installed properly.")
        
        return path.decode("utf-8").strip()
    else:
        try:
            path = subprocess.check_output(f"which {binary}", shell=True)
        except subprocess.CalledProcessError:
            raise Exception(f"Could not locate {binary}, make sure you have the Vulkan SDK installed properly.")
        
        return path.decode("utf-8").strip()


def main():
    glslc = locate("glslc")

    args = "\"{input}\" -x glsl -fshader-stage={stage} -O -o \"{output}\""
    cmd = f"\"{glslc}\" {args}"

    for root, _, files in SHADERS_DIR.walk():
        for file in files:
            input = root / file

            suffix = input.suffix.lower().strip()

            if suffix in SPIRV:
                continue

            stage = ""
            ext = ""

            if suffix in VERTEX:
                stage = "vertex"
                ext = "vert"

            elif suffix in TESS_CONT:
                stage = "tesscontrol"
                ext = "tesc"

            elif suffix in TESS_EVAL:
                stage = "tesseval"
                ext = "tese"

            elif suffix in GEOMETRY:
                stage = "geometry"
                ext = "geo"

            elif suffix in FRAGMENT:
                stage = "fragment"
                ext = "frag"

            elif suffix in COMPUTE:
                stage = "compute"
                ext = "comp"

            if not stage:
                print(f"[FAILED] Could not determine shader stage of '{input}', skipping.")
                continue

            output = root / f"{input.stem}.{ext}.spv"

            rendered = cmd.format(input=input, stage=stage, output=output)
            subprocess.run(rendered, shell=True)

            print(f"[SUCCESS] Compiled '{input}'.")


if __name__ == "__main__":
    main()