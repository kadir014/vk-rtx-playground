from pathlib import Path
import platform
import subprocess


if __name__ == "__main__":
    shaders = Path.cwd() / "shaders"

    # TODO: Locate glslc from the SDK path
    if platform.system() == "Windows":
        binary = "glslc.exe"
    else:
        binary = "glslc"

    args = "\"{input}\" -x glsl -fshader-stage={stage} -O -o \"{output}\""
    cmd = f"{binary} {args}"

    for root, _, files in shaders.walk():
        for file in files:
            input = root / file

            if input.suffix in {".spv", ".spirv", ".spir-v"}:
                continue

            stage = ""
            ext = ""

            if input.suffix in {".vsh", ".vert", ".vertex"}:
                stage = "vertex"
                ext = "vert"

            elif input.suffix in {".fsh", ".frag", ".fragment"}:
                stage = "fragment"
                ext = "frag"

            elif input.suffix in {".gsh", ".geo", ".geometry"}:
                stage = "geometry"
                ext = "geo"

            elif input.suffix in {".csh", ".comp", ".compute"}:
                stage = "compute"
                ext = "comp"

            if not stage:
                print(f"[FAILED] Could not determine shader stage of '{input}', skipping.")
                continue

            output = root / f"{input.stem}.{ext}.spv"

            rendered = cmd.format(input=input, stage=stage, output=output)
            subprocess.run(rendered)

            print(f"[SUCCESS] Compiled '{input}'.")