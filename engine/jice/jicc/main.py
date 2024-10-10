import json
import os
import sys
import shutil

# data ids
proj_json_id = 0
scene_json_id = 1
meta_json_id = 2
engine_version = 100

version_check_cpp = f"""#include <Engine/Version.h>
CHECK_ENGINE_VERSION({engine_version});\n\nusing namespace jice;\n\n"""

def cify_path(p: str):
    # convert all paths to c++ style and prevent same paths not matching
    # for example: ./stuff -> stuff
    # .\stuff/ -> stuff
    # .\this\that\..\stuff -> this/stuff

    # fix backslashes
    p = p.replace('\\', '/')
    # remove leading ./
    if p.startswith('./'):
        p = p[2:]
    # remove any . paths
    p = p.replace('/./', '/')

    # remove any .. paths
    while '/..' in p:
        i = p.index('/..')
        j = p.rindex('/', 0, i)
        p = p[:j] + p[i+3:]

    # remove any trailing /
    if p.endswith('/'):
        p = p[:-1]

    return p

def name_path(p: str):
    a = cify_path(p).replace('/', '__').replace('-', '_').replace('.', '_').replace(' ', '_')
    if a.startswith('__'):
        name = a[2:]
    return a


def dispatch_algo(s: str):
    return f"{s}_Dispatcher_{str(hex(hash(s))).replace('-', '')[2:]}"


def verify_json(js: dict, j_id: int):
    # should start with { "engine_version": 100, "data_id": 0, "data": { ... } }
    if 'engine_version' not in js:
        print("Error: 'engine_version' not found in project file")
        sys.exit(1)
    if 'data_id' not in js:
        print("Error: 'data_id' not found in project file")
        sys.exit(1)
    if 'data' not in js:
        print("Error: 'data' not found in project file")
        sys.exit(1)
    if js['engine_version'] != engine_version:
        print("Error: Engine version mismatch")
        sys.exit(1)
    if js['data_id'] != j_id:
        print("Error: 'proj.json' is not a project file")
        sys.exit(1)
    return js['data']


def parse_data(data: dict, dat_id: str):
    # turn dictionary into cpp code
    # data types:VecF,VecI,Float,Int,String
    # data = { "asd": [0, 0, 0] } ->
    # attrdata["asd"] = AttrData(std::vector<float>{0, 0, 0});
    total = ""
    for k_i, v in data.items():
        k = f"\"{k_i}\""
        if isinstance(v, list):
            if all(isinstance(x, int) for x in v):
                total += f"{dat_id}[{k}] = AttrData(std::vector<int>{{{', '.join(map(str, v))}}});\n"
            elif all(isinstance(x, float) or isinstance(x, int) for x in v):
                total += f"{dat_id}[{k}] = AttrData(std::vector<float>{{ {', '.join(map(str, v))}}});\n"
            else:
                print(f"Error: Invalid data type in list '{v}'")
                sys.exit(1)
        elif isinstance(v, int):
            total += f"{dat_id}[{k}] = AttrData({v});\n"
        elif isinstance(v, float):
            total += f"{dat_id}[{k}] = AttrData({v});\n"
        elif isinstance(v, str):
            total += f"{dat_id}[{k}] = AttrData(\"{v}\");\n"
        else:
            print(f"Error: Invalid data type '{v}'")
            sys.exit(1)
    return total


class Jice:
    def __init__(self, proj: str, build: str):
        self.asset_path = 'assets'
        self.script_path = 'scripts'
        self.scene_path = 'scenes'
        self.proj = proj
        self.build = build
        self.var_count = 0
        self.sources = []
        self.comp_assets = []
        self.copy_assets = []
        print(f"Compiling project: '{proj}', to '{build}'")
        if not os.path.exists(proj):
            print(f"Error: Project directory '{proj}' does not exist")
            sys.exit(1)
        projjsonf = os.path.join(proj, 'proj.json')
        if not os.path.exists(projjsonf):
            print(f"Error: Project file '{projjsonf}' does not exist")
            sys.exit(1)
        if os.path.exists(build):
            # if os.listdir(build):
            #     print(f"Error: Build directory '{build}' is not empty")
            #     sys.exit(1)
            pass
        else:
            os.makedirs(build)

        with open(projjsonf, 'r') as f:
            projjson = json.load(f)


        self.parse_proj(projjson)

        with open(os.path.join(build, "CMakeLists.txt"), 'w') as f:
            backsl = '\\'
            f.write(f"cmake_minimum_required(VERSION 3.6)\nproject(game_build)\n\nset(CMAKE_CXX_STANDARD 17)\n\n" +
                    f"add_executable(game {' '.join(self.sources).replace(backsl, '/')}" +
                    f")\n\ntarget_link_libraries(game Engine eogll)\n#version {engine_version}\n")
            # for the editor to be able to render the game, we compile it, but instead of running it in a new window, we run it in the editor window by importing a shared library and calling it's functions
            f.write(f"add_library(editor_import SHARED {' '.join(self.sources).replace(backsl, '/')})\n")
            f.write(f"target_link_libraries(editor_import Engine eogll)\n")
            # makes it so it copies the ouput exe to the project dir/build/out
            f.write(f"add_custom_command(TARGET game POST_BUILD COMMAND ${{CMAKE_COMMAND}} -E copy $<TARGET_FILE:game> " +
                    f"\"{os.path.join(self.build, 'out').replace(backsl, '/')}/\" COMMENT \"Copying to output directory\")\n")
            f.write(f"add_custom_command(TARGET editor_import POST_BUILD COMMAND ${{CMAKE_COMMAND}} -E copy $<TARGET_FILE:editor_import> " +
                    f"\"{os.path.join(self.build, 'out').replace(backsl, '/')}/\" COMMENT \"Copying to output directory\")\n")

        if not os.path.exists(os.path.join(self.build, 'out')):
            os.makedirs(os.path.join(self.build, 'out'))


    def parse_assets(self):
        # for all files in asset path, copy to build/out/assets, preserving directory structure
        # then convert each file to a .h containing the file data
        # for example assets/stuff/my_image.png -> build/out/assets/stuff/my_image.png.h
        # inside my_image.png.h: const char stuff__my_image_png[] = { ... };
        for root, dirs, files in os.walk(self.asset_path):
            for file in files:
                if file.endswith('.jmeta'):
                    continue
                src = os.path.join(root, file)
                rel_root = os.path.relpath(root, self.asset_path)

                mode = 'compile'

                if os.path.exists(src + '.jmeta'):
                    dat = verify_json(json.load(open(src + '.jmeta', 'r')), meta_json_id)
                    if 'mode' in dat:
                        mode = dat['mode'].lower()
                else:
                    # create a basic .jmeta file
                    dat = {"engine_version": engine_version, "data_id": meta_json_id, "data": {"mode": mode}}
                    with open(src + '.jmeta', 'w') as f:
                        json.dump(dat, f, indent=4)
                    print(f"Warning: No .jmeta file found for '{src}', creating one (defualt mode: '{mode}')")

                if mode == 'compile':
                    print(f"ASSET: Compiling '{src}'")
                    name = name_path(os.path.join(rel_root, file))
                    dst = os.path.join(self.build, 'assets', rel_root, file)
                    self.comp_assets.append((name, dst, cify_path(os.path.join(rel_root, file))))
                    if not os.path.exists(os.path.dirname(dst)):
                        os.makedirs(os.path.dirname(dst))
                    with open(src, 'rb') as f:
                        data = f.read()
                    # convert to .h
                    with open(dst + '.h', 'w') as f:
                        f.write(version_check_cpp)
                        f.write(f"#pragma once\n")
                        f.write("#include <cstdint>\n\n")
                        f.write(f"const uint8_t {name}[{len(data)}] = ")
                        f.write("{" + ", ".join(map(str, data)) + "};\n")
                        f.write(f"const size_t {name}_len = {len(data)};\n")
                elif mode == 'copy':
                    print(f"ASSET: Copying '{src}'")
                    # copy file to build/out/assets with same directory structure
                    dst = os.path.join(self.build, 'out', 'assets', rel_root, file)
                    if not os.path.exists(os.path.dirname(dst)):
                        os.makedirs(os.path.dirname(dst))

                    self.copy_assets.append(cify_path(os.path.join(rel_root, file)))

                    shutil.copy(src, dst)
                else:
                    print(f"Error: Invalid mode '{mode}' in '{src}.jmeta'")
                    sys.exit(1)


    def parse_proj(self, projjson: dict):
        inc_sec = version_check_cpp
        inc_sec += "#include <Engine/internal/Engine.h>\n"
        inc_sec += "#include <chrono>\n"
        src_main_sec = ""

        dat = verify_json(projjson, proj_json_id)
        if 'id' not in dat:
            print("Error: 'id' not found in project file")
            sys.exit(1)
        con_id = dat['id']
        if 'name' not in dat:
            con_name = con_id
            print(f"Warning: 'name' not found in project file, using '{con_name}'")
        else:
            con_name = dat['name']
        if 'description' not in dat:
            con_desc = ""
            print("Warning: 'description' not found in project file")
        else:
            con_desc = dat['description']
        if 'version' not in dat:
            con_ver = "0.0.1"
            print(f"Warning: 'version' not found in project file, using '{con_ver}'")
        else:
            con_ver = dat['version']
        if 'author' not in dat:
            con_auth = ""
            print("Warning: 'author' not found in project file")
        else:
            con_auth = dat['author']

        # prevent malicious names/ids/descriptions/authors
        # id is alnum + underscore + hyphen, no leading/trailing hyphens, no leading digits
        for c in con_id:
            if not (c.isalnum() or c == '_' or c == '-'):
                print(f"Error: Invalid id '{con_id}'")
                sys.exit(1)
        if con_id[0].isdigit():
            print(f"Error: Invalid id '{con_id}'")
            sys.exit(1)
        if con_id[0] == '-' or con_id[-1] == '-':
            print(f"Error: Invalid id '{con_id}'")
            sys.exit(1)
        # name is any printable ascii character
        for c in con_name:
            if not c.isprintable() or c == '"' or c == '\\':
                print(f"Error: Invalid name '{con_name}'")
                sys.exit(1)
        # description is any printable ascii character
        for c in con_desc:
            if not c.isprintable() or c == '"' or c == '\\':
                print(f"Error: Invalid description '{con_desc}'")
                sys.exit(1)
        # version is digits + dots
        for c in con_ver:
            if not (c.isdigit() or c == '.'):
                print(f"Error: Invalid version '{con_ver}'")
                sys.exit(1)
        # author is any printable ascii character
        for c in con_auth:
            if not c.isprintable() or c == '"' or c == '\\':
                print(f"Error: Invalid author '{con_auth}'")
                sys.exit(1)

        src_main_sec += (f"auto* engine = new Engine(\"{con_id}\", \"{con_name}\", \"{con_desc}\", " +
                         f"\"{con_ver}\", \"{con_auth}\");\n")

        if 'content' not in dat:
            content = {}
        else:
            content = dat['content']
        if 'asset_path' in content:
            self.asset_path = content['asset_path']
        if 'script_path' in content:
            self.script_path = content['script_path']
        if 'scene_path' in content:
            self.scene_path = content['scene_path']
        if not os.path.isabs(self.asset_path):
            self.asset_path = os.path.join(self.proj, self.asset_path)
        if not os.path.isabs(self.script_path):
            self.script_path = os.path.join(self.proj, self.script_path)
        if not os.path.isabs(self.scene_path):
            self.scene_path = os.path.join(self.proj, self.scene_path)

        if not os.path.exists(self.asset_path):
            print(f"Warning: Asset path '{self.asset_path}' does not exist")

        self.parse_assets()

        splash_enabled = False

        if 'splash_screen' in dat:
            splash = dat['splash_screen']
            if 'enabled' in splash:
                if splash['enabled']:
                    if 'image' in splash:
                        img = splash['image']
                        if not name_path(img) in [a[0] for a in self.comp_assets]:
                            print(f"Error: 'image' '{img}' not found in assets")
                            sys.exit(1)
                        splash_enabled = True
                        src_main_sec += "// Splash enabled\n"
                        # perform image loading here instead of later
                        asset = [a for a in self.comp_assets if a[0] == name_path(img)][0]
                        if not asset:
                            print(f"Error: 'image' '{img}' not found in assets")
                            sys.exit(1)
                        self.comp_assets.remove(asset)
                        b = os.path.relpath(asset[1], self.build).replace('\\', '/')
                        c = asset[2].replace('\\', '/')
                        inc_sec += f"#include \"{b}.h\"\n"
                        src_main_sec += f"engine->addAsset(\"{c}\", CompiledAsset({asset[0]}, {asset[0]}_len));\n"
                        src_main_sec += f"if (isCompiled) engine->beginSplash(\"{cify_path(img)}\");\n"
                    else:
                        print("Error: 'image' not found in splash screen")
                        sys.exit(1)

        # search for scripts
        scripts = []
        if os.path.exists(self.script_path):
            for root, dirs, files in os.walk(self.script_path):
                for file in files:
                    if file.endswith('.cpp'):
                        scripts.append(file)
        else:
            print(f"Warning: Script path '{self.script_path}' does not exist")

        for s in scripts:
            self.parse_script(s)
            inc_sec += f"#include \"scripts/{s[:-4]}.h\"\n"
            src_main_sec += f"engine->registerScript(\"{s[:-4]}\", {dispatch_algo(s[:-4])});\n"

        # search for scenes
        scenes = []
        if os.path.exists(self.scene_path):
            for root, dirs, files in os.walk(self.scene_path):
                for file in files:
                    if file.endswith('.json'):
                        scenes.append(file)
        else:
            print(f"Warning: Scene path '{self.scene_path}' does not exist")

        for s in scenes:
            self.parse_scene(s)
            inc_sec += f"#include \"scenes/{s[:-5]}.h\"\n"
            src_main_sec += f"engine->addScene(\"{s[:-5]}\", new {s[:-5]}(engine));\n"

        for a in self.comp_assets:
            b = os.path.relpath(a[1], self.build).replace('\\', '/')
            c = a[2].replace('\\', '/')
            inc_sec += f"#include \"{b}.h\"\n"
            src_main_sec += f"engine->addAsset(\"{c}\", CompiledAsset({a[0]}, {a[0]}_len));\n"

        for a in self.copy_assets:
            src_main_sec += f"engine->addAsset(\"{a}\", CopiedAsset(\"{a}\"));\n"

        src_main_sec += "engine->loadConfig(\"config.json\");\n"
        if splash_enabled:
            src_main_sec += "if (isCompiled) {\n"
            src_main_sec += "    auto start = std::chrono::high_resolution_clock::now();\n"
            src_main_sec += "    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() < 5) {}\n"
            src_main_sec += "    engine->endSplash();\n"
            src_main_sec += "}\n"


        src_main_sec += "engine->run();\nreturn 0;\n"
        # if win32 export dll
        final = "#ifdef _WIN32\n#define EXPORT __declspec(dllexport)\n#else\n#define EXPORT\n#endif\n\n"
        final += inc_sec + "\n\nextern \"C\" EXPORT int mainGame(bool isCompiled) {\n    "
        final += src_main_sec.replace('\n', '\n    ')
        final += "\n}"
        final += "\n\nint main() {\n    return mainGame(true);\n}\n"

        with open(os.path.join(self.build, 'main.cpp'), 'w') as f:
            f.write(final)
            self.sources.append('main.cpp')

    def parse_script(self, script: str):
        print(f"SCRIPT: {script}")

        # source
        final = version_check_cpp
        final += f"#include " + f"\"{script[:-4]}.h\"\n"
        with open(os.path.join(self.script_path, script), 'r') as f:
            final += f.read()
        final += (f"\nScriptInterface* {dispatch_algo(script[:-4])}(Engine* e, GameObject* o) {{\n    return new " +
                  f"{script[:-4]}(e, o);\n}}\n")
        if not os.path.exists(os.path.join(self.build, 'scripts')):
            os.makedirs(os.path.join(self.build, 'scripts'))
        with open(os.path.join(self.build, 'scripts', f'{script[:-4]}.cpp'), 'w') as f:
            f.write(final)
            self.sources.append(os.path.join('scripts', f'{script[:-4]}.cpp'))

        # header
        final = "#pragma once\n" + version_check_cpp
        final += f"#include <Engine/Script.h>\n\n"
        final += f"ScriptInterface* {dispatch_algo(script[:-4])}(Engine* e, GameObject* o);\n"
        with open(os.path.join(self.build, 'scripts', f'{script[:-4]}.h'), 'w') as f:
            f.write(final)
            self.sources.append(os.path.join('scripts', f'{script[:-4]}.h'))

    def parse_scene(self, scene: str):
        inc_sec = version_check_cpp
        inc_sec += f"#include \"{scene[:-5]}.h\"\n"
        src_con_sec = ""
        src_set_sec = ""
        src_upd_sec = ""
        print(f"SCENE: {scene}")
        sp = os.path.join(self.scene_path, scene)
        if not os.path.exists(sp):
            print(f"Error: Scene file '{sp}' does not exist")
            sys.exit(1)
        with open(sp, 'r') as f:
            scenejson = json.load(f)

        dat = verify_json(scenejson, scene_json_id)

        if 'name' not in dat:
            print(f"Error: 'name' not found in scene file '{sp}'")
            sys.exit(1)
        if dat['name'] != scene[:-5]:
            print(f"Error: Scene file '{sp}' name does not match")
            sys.exit(1)
        src_con_sec += f"this->name = \"{dat['name']}\";\n"

        if '3d' not in dat:
            src_con_sec += "this->mode = SceneMode::Dimension2D;\n"
        else:
            if dat['3d']:
                src_con_sec += "this->mode = SceneMode::Dimension3D;\n"
            else:
                src_con_sec += "this->mode = SceneMode::Dimension2D;\n"

        if 'content' not in dat:
            content = {}
        else:
            content = dat['content']

        # list of objects
        self.var_count = 0
        for obj in content:
            src_con_sec, src_set_sec, src_upd_sec = self.parse_object(obj, src_con_sec, src_set_sec, src_upd_sec)

        src_set_sec += "Scene::Setup();\n"
        src_upd_sec += "Scene::Update();\n"

        final = inc_sec + f"\n{scene[:-5]}::{scene[:-5]}(Engine* e) : Scene(e) {{\n    "
        final += src_con_sec.replace('\n', '\n    ')
        final += "}\n\n"
        final += f"void {scene[:-5]}::Setup() {{\n    "
        final += src_set_sec.replace('\n', '\n    ')
        final += "}\n\n"
        final += f"void {scene[:-5]}::Update() {{\n    "
        final += src_upd_sec.replace('\n', '\n    ')
        final += "}\n"
        if not os.path.exists(os.path.join(self.build, 'scenes')):
            os.makedirs(os.path.join(self.build, 'scenes'))
        with open(os.path.join(self.build, 'scenes', f'{scene[:-5]}.cpp'), 'w') as f:
            f.write(final)
            self.sources.append(os.path.join('scenes', f'{scene[:-5]}.cpp'))

        # header
        final = "#pragma once\n" + version_check_cpp
        final += f"#include <Engine/internal/Engine.h>\n"
        final += f"#include <Engine/internal/Scene.h>\n\n"
        final += (f"class {scene[:-5]} : public Scene {{\npublic:\n    explicit {scene[:-5]}" +
                  "(Engine* e);\n    void Setup() override;\n    void Update() override;\n};\n")

        with open(os.path.join(self.build, 'scenes', f'{scene[:-5]}.h'), 'w') as f:
            f.write(final)
            self.sources.append(os.path.join('scenes', f'{scene[:-5]}.h'))

    def parse_object(self, obj: dict, con: str, setup: str, upd: str):
        if 'id' not in obj:
            print("Error: 'id' not found in object")
            sys.exit(1)
        # check if id is valid
        for c in obj['id']:
            if not (c.isalnum() or c == '_' or c == '-'):
                print(f"Error: Invalid id '{obj['id']}'")
                sys.exit(1)
        if obj['id'][0].isdigit():
            print(f"Error: Invalid id '{obj['id']}'")
            sys.exit(1)
        if obj['id'][0] == '-' or obj['id'][-1] == '-':
            print(f"Error: Invalid id '{obj['id']}'")
            sys.exit(1)
        go_id = f"_GameObject_p_{self.var_count}"
        self.var_count += 1
        con += f"GameObject* {go_id} = new GameObject(\"{obj['id']}\");\n"
        if 'attributes' in obj:
            for attr in obj['attributes']:
                if 'type' not in attr:
                    print("Error: 'type' not found in attribute")
                    sys.exit(1)
                attrd_id = f"_AttributeData_{self.var_count}"
                self.var_count += 1
                con += f"AttributeData {attrd_id};\n"
                if attr['type'] == 'script':
                    if 'location' not in attr:
                        print("Error: 'location' not found in attribute")
                        sys.exit(1)
                    con += (f"{go_id}->addAttribute(new Attribute(e->dispatchScript(\"{attr['location']}\", {go_id})" +
                            f", {attrd_id}, \"{attr['location']}\"));\n")
                elif attr['type'] == 'builtin':
                    if 'id' not in attr:
                        print("Error: 'id' not found in attribute")
                        sys.exit(1)
                    if 'data' in attr:
                        con += parse_data(attr['data'], attrd_id)
                    con += f"{go_id}->addAttribute(new Attribute(e, \"{attr['id']}\", {attrd_id}));\n"
                else:
                    print(f"Error: Invalid attribute type '{attr['type']}'")
                    sys.exit(1)
        if 'children' in obj:
            for child in obj['children']:
                con, setup, upd = self.parse_object(child, con, setup, upd)
        con += f"this->addObject({go_id});\n"

        return con, setup, upd


if __name__ == '__main__':
    # usage:
    # python main.py <proj_dir> <build_dir>
    if len(sys.argv) != 3:
        print('Usage: python main.py <proj_dir> <build_dir>')
        sys.exit(1)
    else:
        Jice(sys.argv[1], sys.argv[2])
