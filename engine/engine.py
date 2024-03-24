import sys
import os
import shutil


jicedir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "jice")
cmakeflags_conf = "-G Ninja"

def configure_proj(proj_dir, debug):
    if debug:
        os.system(f"cmake -DJICE_DIR={jicedir} {proj_dir} -B {proj_dir}/build/CMakeBuild -DCMAKE_BUILD_TYPE=Debug " + cmakeflags_conf)
    else:
        os.system(f"cmake -DJICE_DIR={jicedir} {proj_dir} -B {proj_dir}/build/CMakeBuild -DCMAKE_BUILD_TYPE=Release " + cmakeflags_conf)


def compile_proj(proj_dir, debug):
    if not os.path.exists(proj_dir):
        print(f"Directory {proj_dir} does not exist")
        return
    if not os.path.exists(os.path.join(proj_dir, "CMakeLists.txt")):
        print(f"CMakeLists.txt does not exist in {proj_dir}")
        return
    if not os.path.exists(os.path.join(proj_dir, "proj.json")):
        print(f"proj.json does not exist in {proj_dir}")
        return
    configure_proj(proj_dir, debug)
    if debug:
        os.system(f"cmake --build {proj_dir}/build/CMakeBuild --config Debug --target game")
    else:
        os.system(f"cmake --build {proj_dir}/build/CMakeBuild --config Release --target game")

def editor_proj(proj_dir):
    if not os.path.exists(proj_dir):
        print(f"Directory {proj_dir} does not exist")
        return
    if not os.path.exists(os.path.join(proj_dir, "CMakeLists.txt")):
        print(f"CMakeLists.txt does not exist in {proj_dir}")
        return
    if not os.path.exists(os.path.join(proj_dir, "proj.json")):
        print(f"proj.json does not exist in {proj_dir}")
        return
    configure_proj(proj_dir, True)
    os.system(f"cmake --build {proj_dir}/build/CMakeBuild --config Debug --target editor_import")


def clean_proj(proj_dir):
    if not os.path.exists(proj_dir):
        print(f"Directory {proj_dir} does not exist")
        return
    if not os.path.exists(os.path.join(proj_dir, "CMakeLists.txt")):
        print(f"CMakeLists.txt does not exist in {proj_dir}")
        return
    if not os.path.exists(os.path.join(proj_dir, "proj.json")):
        print(f"proj.json does not exist in {proj_dir}")
        return
    shutil.rmtree(os.path.join(proj_dir, "build"))

def new_proj(proj_dir):
    if os.path.exists(proj_dir):
        print(f"Directory {proj_dir} already exists")
        return
    # make dirs UP TO proj_dir
    if not os.path.exists(os.path.dirname(proj_dir)):
        os.makedirs(os.path.dirname(proj_dir))
    shutil.copytree(os.path.join(jicedir, "template"), proj_dir)
    print(f"Created new project in {proj_dir}")

if __name__ == '__main__':

    if len(sys.argv) < 3:
        print("Invalid number of arguments")
        print("Usage:")
        print("jice compile <directory> [debug]")
        print("jice editor <directory>") # compile for the editor mode
        print("jice clean <directory>")
        print("jice new <directory>")
        sys.exit(1)
    if sys.argv[1].lower() == "compile":
        compile_proj(sys.argv[2], len(sys.argv) > 3 and (sys.argv[3].lower() == "debug" or sys.argv[3].lower() == "d" or sys.argv[3].lower() == "dbg" or sys.argv[3].lower() == "true"))
    elif sys.argv[1].lower() == "clean":
        clean_proj(sys.argv[2])
    elif sys.argv[1].lower() == "new":
        new_proj(sys.argv[2])
    elif sys.argv[1].lower() == "editor":
        editor_proj(sys.argv[2])

