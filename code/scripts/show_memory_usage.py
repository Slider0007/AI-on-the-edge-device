import os
import sys
Import("env")

def post_build_memory_info(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    map_file = os.path.join(build_dir, "AI-on-the-Edge-Device.map")
    python_exe = sys.executable

    if not os.path.exists(map_file):
        print(f"\n[Memory Report Warning] Map file not found: {map_file}")
        return

    print("\n=== MEMORY SUMMARY ===")
    env.Execute(f'"{python_exe}" -m esp_idf_size "{map_file}"')

    # Shows RAM and Flash usage per library/component
    #print("\n=== MEMORY BREAKDOWN --> COMPONENTS ===")
    #env.Execute(f'"{python_exe}" -m esp_idf_size --archives "{map_file}"')

    # Shows RAM and Flash usage per library/component
    #print("\n=== MEMORY BREAKDOWN --> FILES ===")
    #env.Execute(f'"{python_exe}" -m esp_idf_size --archives "{map_file}"')

# Triggers automatically when PlatformIO finishes generating the binary ELF
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", post_build_memory_info)