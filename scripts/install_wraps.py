import subprocess
import os


if not os.path.exists("subprojects"):
    os.mkdir("subprojects")


wraps = [
    "sdl2",
    "sdl2_image",
    "vulkan-memory-allocator",
    "cglm",
]

for wrap in wraps:
    print(f"[WRAP INSTALLER] Installing wrap: {wrap}")
    subprocess.run(f"meson wrap install {wrap}", shell=True)