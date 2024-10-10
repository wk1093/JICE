#include <iostream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <cstdint>
#include <fstream>
#include <algorithm>

using nlohmann::json;

namespace fs = std::filesystem;

enum class JsonID {
    Project=0,
    Scene=1,
    Meta=2,
};

enum class AssetType {
    Compile=0,
    Copy=1,
};

struct Asset {
    AssetType type;
    std::string name;
    std::string dst;
    std::string src; // only this is used in Copy mode

    bool operator==(const Asset& other) const {
        return type == other.type && name == other.name && dst == other.dst && src == other.src;
    }
};

static const uint16_t JICE_ENGINE_VERSION = 100;

static const std::string VERSION_CHECK_CPP = R"(
#include <Engine/Version.h>
CHECK_ENGINE_VERSION( )" + std::to_string(JICE_ENGINE_VERSION) + R"( );

using namespace jice;

)";

std::string cify_path(const fs::path& path) {
    return path.lexically_normal().generic_string();
}
std::string cify_path(const std::string& path) {
    return cify_path(fs::path(path));
}

bool endswith(const std::string& str, const std::string& end) {
    if (str.size() < end.size()) {
        return false;
    }
    return str.compare(str.size() - end.size(), end.size(), end) == 0;
}

std::string strip(const std::string& str) {
    // remove leading and trailing whitespace
    size_t start = 0;
    size_t end = str.size();
    while (start < str.size() && std::isspace(str[start])) {
        start++;
    }
    while (end > 0 && std::isspace(str[end-1])) {
        end--;
    }
    return str.substr(start, end-start);
}

std::string indent(const std::string& str, int spaces) {
    // basically replace all '\n' with '\n' + ' ' * spaces
    std::string out;
    for (char c: str) {
        out += c;
        if (c == '\n') {
            out += std::string(spaces, ' ');
        }
    }
    return out;
}

std::string hex_num(uint8_t num) {
    std::ostringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)num;
    return ss.str();
}

std::string name_path(const fs::path& path) {
    // name version of a path that is a valid C++ identifier
    std::string out = "_";
    for (auto part: cify_path(path)) {
        if (part == '/' || part == '\\') {
            out += "__";
        } else if (std::isalnum(part)) {
            out += part;
        } else {
            out += "_" + hex_num(part);
        }
    }
    return out;
}

std::string name_path(const std::string& path) {
    return name_path(fs::path(path));
}

std::string dispatch_string(const std::string& source) {
    // create a string that uses a hash of source
    std::hash<std::string> hasher;
    auto hash = hasher(source);
    std::ostringstream ss;
    ss << "_" << source << "_Dispatcher_" << std::hex << hash;
    return ss.str();
}

json verify_json(json j, JsonID id) {
    if (j.find("engine_version") == j.end()) {
        std::cerr << "Error: JSON missing engine_version" << std::endl;
        return {};
    }
    if (j["engine_version"] != JICE_ENGINE_VERSION) {
        std::cerr << "Error: JSON engine_version mismatch" << std::endl;
        return {};
    }
    if (j.find("data_id") == j.end()) {
        std::cerr << "Error: JSON missing data_id" << std::endl;
        return {};
    }
    if (j["data_id"] != (int)id) {
        std::cerr << "Error: JSON data_id mismatch" << std::endl;
        return {};
    }
    if (j.find("data") == j.end()) {
        std::cerr << "Error: JSON missing data" << std::endl;
        return {};
    }
    return j["data"];
}

bool all_is_int(json j) {
    for (auto& v: j) {
        if (!v.is_number_integer()) {
            return false;
        }
    }
    return true;
}

bool all_is_number(json j) {
    for (auto& v: j) {
        if (!v.is_number()) {
            return false;
        }
    }
    return true;
}

std::string join(json j, std::string sep) {
    std::ostringstream ss;
    for (auto& v: j) {
        ss << v << sep;
    }
    std::string out = ss.str();
    return out.substr(0, out.size() - sep.size());
}

std::string parse_attr_data(json j, std::string dat_id) {
    std::ostringstream ss;
    // for pairs in data
    for (auto& [k_i, v]: j.items()) {
        std::string k = '"' + k_i + '"';
        if (v.is_array()) {
            if (all_is_int(v)) {
                ss << dat_id << '[' << k << "] = AttrData(std::vector<int>{" << join(v, ", ") << "});\n";
            } else if (all_is_number(v)) {
                ss << dat_id << '[' << k << "] = AttrData(std::vector<float>{" << join(v, ", ") << "});\n";
            } else {
                std::cout << "Error: unsupported array type, assuming string!!!" << std::endl;
                ss << dat_id << '[' << k << "] = AttrData(std::vector<std::string>{" << join(v, ", ") << "});\n";
            }
        } else if (v.is_number()) {
            ss << dat_id << '[' << k << "] = AttrData(" << v << ");\n";
        } else if (v.is_string()) {
            ss << dat_id << '[' << k << "] = AttrData(\"" << std::string(v) << "\");\n";
        } else {
            std::cerr << "Error: unsupported data type" << std::endl;
        }
    }
    return ss.str();
}

struct JiccCompiler {
    std::string asset_path = "assets";
    std::string script_path = "scripts";
    std::string scene_path = "scenes";
    std::string proj;
    std::string build;
    uint64_t var_count = 0;
    std::vector<std::string> sources;
    std::vector<Asset> assets;


    JiccCompiler(const std::string& proj_path, const std::string& build_path) :
        proj(proj_path), build(build_path) {
        if (!fs::exists(proj)) {
            std::cerr << "Error: Project file not found" << std::endl;
            return;
        }
        if (!fs::exists(build)) {
            fs::create_directories(build);
        } else {
            fs::remove_all(build);
            fs::create_directories(build);
        }
        std::cout << "Compiling project '" << proj << "' to '" << build << "'" << std::endl;
        fs::path json_path(proj);
        json_path = json_path / "proj.json";
        if (!fs::exists(json_path)) {
            std::cerr << "Error: Project file not found" << std::endl;
            return;
        }
        std::ifstream json_file(json_path);
        if (!json_file.is_open()) {
            std::cerr << "Error: Failed to open project file" << std::endl;
            return;
        }
        json j;
        json_file >> j;
        json_file.close();
        parse_proj(j);

        // generate cmakelists for project
        std::ofstream cml(fs::path(build) / "CMakeLists.txt");
        cml << "cmake_minimum_required(VERSION 3.10)\n";
        cml << "project(game_build)\n";
        cml << "# version " << JICE_ENGINE_VERSION << "\n";
        cml << "set(CMAKE_CXX_STANDARD 17)\n\n";
        cml << "add_executable(game ";
        for (auto &src: sources) {
            cml << cify_path(src) << " ";
        }
        cml << ")\n";
        cml << "target_link_libraries(game Engine eogll)\n";
        cml << "add_library(editor_import SHARED ";
        for (auto &src: sources) {
            cml << cify_path(src) << " ";
        }
        cml << ")\n";
        cml << "target_link_libraries(editor_import Engine eogll)\n";
        cml << "add_custom_command(TARGET game POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:game> \"";
        cml << cify_path(fs::path(build) / "out") << "\")\n";
        cml
                << "add_custom_command(TARGET editor_import POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:editor_import> \"";
        cml << cify_path(fs::path(build) / "out") << "\")\n";

        cml.close();

        if (!fs::exists(fs::path(build) / "out")) {
            fs::create_directories(fs::path(build) / "out");
        }
    }

private:
    void parse_assets() {
        // recurse through asset_path
        for (const auto& entry: fs::recursive_directory_iterator(asset_path)) {
            if (entry.is_regular_file()) {
                std::string str = entry.path().string();
                if (endswith(str, ".jmeta")) {
                    continue;
                }
                fs::path rel_root = fs::relative(entry.path().parent_path(), fs::path(asset_path));
                fs::path rel_file = fs::relative(entry.path(), fs::path(asset_path));
                std::string src = cify_path(entry.path());
                AssetType type = AssetType::Compile; // TODO: determine default type

                if (fs::exists(src+".jmeta")) {
                    std::ifstream meta_file(src+".jmeta");
                    json j;
                    meta_file >> j;
                    meta_file.close();
                    j = verify_json(j, JsonID::Meta);
                    if (j.contains("mode")) {
                        if (j["mode"] == "copy") {
                            type = AssetType::Copy;
                        } else if (j["mode"] == "compile") {
                            type = AssetType::Compile;
                        } else {
                            std::cout << "Warning: unknown mode, assuming compile" << std::endl;
                        }
                    }
                } else {
                    std::cout << "Warning: no meta file found, creating one (default = compile)" << std::endl;
                    json dat = {
                            {"engine_version", JICE_ENGINE_VERSION},
                            {"data_id", (int)JsonID::Meta},
                            {"data", {
                                {"mode", "compile"}
                            }}
                    };
                    std::ofstream meta_file(src+".jmeta");
                    meta_file << dat.dump(4);
                    meta_file.close();
                }
                if (type == AssetType::Compile) {
                    std::cout << "ASSET: (compile) '" << rel_file.generic_string() << "'" << std::endl;
                    std::string name = name_path(rel_file);
                    fs::path dst = fs::path(build) / "assets" / rel_file;
                    Asset asset = {type, name, cify_path(dst), cify_path(rel_file)};
                    assets.push_back(asset);
                    if (!fs::exists(dst.parent_path())) {
                        fs::create_directories(dst.parent_path());
                    }
                    std::ifstream src_file(src);
                    std::ofstream dst_file(cify_path(dst)+".h");
                    dst_file << VERSION_CHECK_CPP;
                    dst_file << "#pragma once\n";
                    dst_file << "#include <cstdint>\n\n";
                    dst_file << "const uint8_t " << name << "[" << std::filesystem::file_size(src) << "] = {\n";
                    uint8_t byte;
                    while (src_file.read((char*)&byte, 1)) {
                        dst_file << "0x" << hex_num(byte) << ", ";
                    }
                    dst_file << "};\n";
                    dst_file << "const size_t " << name << "_len = " << std::filesystem::file_size(src) << ";\n";
                    src_file.close();
                    dst_file.close();
                } else if (type == AssetType::Copy) {
                    std::cout << "ASSET: (copy) '" << rel_file.generic_string() << "'" << std::endl;
                    fs::path dst = fs::path(build) / "out" / "assets" / rel_file;
                    if (!fs::exists(dst.parent_path())) {
                        fs::create_directories(dst.parent_path());
                    }

                    Asset asset = {type, name_path(rel_file), cify_path(dst), cify_path(rel_file)};
                    assets.push_back(asset);

                    if (fs::exists(dst)) {
                        fs::remove(dst);
                    }
                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
                } else {
                    std::cerr << "Error: unknown asset type" << std::endl;
                    std::cerr << "Should never happen" << std::endl;
                }
            }
        }
    }

    void parse_proj(json j) {
        std::ostringstream inc_sec;
        inc_sec << VERSION_CHECK_CPP;
        inc_sec << "#include <Engine/internal/Engine.h>\n";
#ifdef SPLASH_DELAY
        inc_sec += "#include <chrono>\n";
#endif
        std::ostringstream src_main_sec;

        json dat = verify_json(j, JsonID::Project);

        std::string con_id, con_name, con_desc, con_ver, con_auth;

        if (dat.find("id") == dat.end()) {
            std::cerr << "Error: Project missing id" << std::endl;
            return;
        }
        con_id = dat["id"];

        if (dat.find("name") == dat.end()) {
            con_name = con_id;
            std::cout << "Warning: Project missing name, using id" << std::endl;
        } else
            con_name = dat["name"];

        if (dat.find("description") == dat.end()) {
            con_desc = "";
            std::cout << "Warning: Project missing description" << std::endl;
        } else
            con_desc = dat["description"];

        if (dat.find("version") == dat.end()) {
            con_ver = "0.0.1";
            std::cout << "Warning: Project missing version, using 0.0.1" << std::endl;
        } else
            con_ver = dat["version"];

        if (dat.find("author") == dat.end()) {
            con_auth = "";
            std::cout << "Warning: Project missing author" << std::endl;
        } else
            con_auth = dat["author"];

        // sanitize id: alnum + underscore + hyphen, no leading/trailing hyphen, no leading digits
        std::string con_id_san;
        for (char c: con_id) {
            // first
            if (con_id_san.empty()) {
                if (std::isalnum(c) || c == '_') {
                    con_id_san += c;
                } else {
                    std::cout << "Warning: Project id contains invalid characters, removing" << std::endl;
                }
            } else {
                if (std::isalnum(c) || c == '_' || c == '-') {
                    con_id_san += c;
                } else {
                    std::cout << "Warning: Project id contains invalid characters, removing" << std::endl;
                }
            }
        }
        if (con_id_san.empty()) {
            con_id_san = "project";
            std::cout << "Warning: Project id is empty, using 'project'" << std::endl;
        }

        // sanitize name and description: any printable ascii, only space for whitespace (no newlines, tabs or anything else)
        std::string con_name_san, con_desc_san;
        for (char c: con_name) {
            if (std::isprint(c)) {
                con_name_san += c;
            } else {
                std::cout << "Warning: Project name contains invalid characters, removing" << std::endl;
            }
        }
        for (char c: con_desc) {
            if (std::isprint(c)) {
                con_desc_san += c;
            } else {
                std::cout << "Warning: Project description contains invalid characters, removing" << std::endl;
            }
        }
        con_name_san = strip(con_name_san);
        con_desc_san = strip(con_desc_san);
        if (con_name_san.empty()) {
            con_name_san = "Project";
            std::cout << "Warning: Project name is empty, using 'Project'" << std::endl;
        }
        if (con_desc_san.empty()) {
            con_desc_san = "No description";
            std::cout << "Warning: Project description is empty, using 'No description'" << std::endl;
        }

        // sanitize version: digits + dots, no leading/trailing dots, no multiple dots

        std::string con_ver_san;
        bool dot = false;
        for (char c: con_ver) {
            if (std::isdigit(c)) {
                con_ver_san += c;
                dot = false;
            } else if (c == '.' && !dot) {
                con_ver_san += c;
                dot = true;
            } else {
                std::cout << "Warning: Project version contains invalid characters, removing" << std::endl;
            }
        }
        if (con_ver_san.empty()) {
            con_ver_san = "0.0.1";
            std::cout << "Warning: Project version is empty, using '0.0.1'" << std::endl;
        }

        // sanitize author: any printable ascii, only space for whitespace (no newlines, tabs or anything else)
        std::string con_auth_san;
        for (char c: con_auth) {
            if (std::isprint(c)) {
                con_auth_san += c;
            } else {
                std::cout << "Warning: Project author contains invalid characters, removing" << std::endl;
            }
        }
        con_auth_san = strip(con_auth_san);
        if (con_auth_san.empty()) {
            con_auth_san = "Unknown";
            std::cout << "Warning: Project author is empty, using 'Unknown'" << std::endl;
        }

        src_main_sec << "auto* engine = new Engine(";
        src_main_sec << '"' + con_id_san + '"';
        src_main_sec << ", ";
        src_main_sec << '"' + con_name_san + '"';
        src_main_sec << ", ";
        src_main_sec << '"' + con_desc_san + '"';
        src_main_sec << ", ";
        src_main_sec << '"' + con_ver_san + '"';
        src_main_sec << ", ";
        src_main_sec << '"' + con_auth_san + '"';
        src_main_sec << ");\n";

        json content;
        if (dat.find("content") == dat.end()) {
            content = {};
        } else {
            content = dat["content"];
        }
        if (content.find("asset_path") != content.end()) {
            asset_path = content["asset_path"];
        }
        if (content.find("script_path") != content.end()) {
            script_path = content["script_path"];
        }
        if (content.find("scene_path") != content.end()) {
            scene_path = content["scene_path"];
        }
        if (fs::path(asset_path).is_relative()) {
            asset_path = cify_path(fs::path(proj) / asset_path);
        }
        if (fs::path(script_path).is_relative()) {
            script_path = cify_path(fs::path(proj) / script_path);
        }
        if (fs::path(scene_path).is_relative()) {
            scene_path = cify_path(fs::path(proj) / scene_path);
        }

        if (!fs::exists(asset_path)) {
            std::cerr << "Warning: Asset path not found" << std::endl;
        }
        if (!fs::exists(script_path)) {
            std::cerr << "Warning: Script path not found" << std::endl;
        }
        if (!fs::exists(scene_path)) {
            std::cerr << "Warning: Scene path not found" << std::endl;
        }

        parse_assets();

        bool splash_enabled = false;

        if (content.find("splash_screen") != content.end()) {
            json splash = dat["splash_screen"];
            if (splash.find("enabled") != splash.end()) {
                splash_enabled = splash["enabled"];
            }
            if (splash_enabled && splash.find("image") != splash.end()) {
                std::string img = splash["image"];
                // if name_path(img) is not in assets, print error
                bool found = false;
                Asset the_asset;
                for (auto &asset: assets) {
                    if (asset.type == AssetType::Compile && asset.name == name_path(img)) {
                        found = true;
                        the_asset = asset;
                        break;
                    }
                }
                if (!found) {
                    std::cerr << "Error: Splash screen image not found in assets" << std::endl;
                    splash_enabled = false;
                }
                // remove the_asset from assets
                assets.erase(std::remove(assets.begin(), assets.end(), the_asset), assets.end());

                src_main_sec << "// Splash enabled\n";
                std::string b = cify_path(fs::relative(the_asset.dst, fs::path(build)));
                std::string c = the_asset.src;
                inc_sec << "#include \"" + b + ".h\"\n";
                src_main_sec << "engine->addAsset(\"" + c + "\", CompiledAsset(" + the_asset.name + ", " + the_asset.name + "_len));\n";
                src_main_sec << "if (isCompiled) engine->beginSplash(\"" + cify_path(img) + "\");\n";
            } else if (splash_enabled) {
                std::cerr << "Error: Splash screen enabled but no image specified" << std::endl;
                splash_enabled = false;
            }
        }

        std::vector<std::string> scripts;
        if (fs::exists(script_path)) {
            for (const auto& entry: fs::recursive_directory_iterator(script_path)) {
                if (endswith(entry.path().string(), ".cpp")) {
                    scripts.push_back(cify_path(entry.path()));
                }
            }
        } else {
            std::cerr << "Warning: Script path not found" << std::endl;
        }

        for (auto &script: scripts) {
            parse_script(script);
            std::string scr_path = cify_path(fs::relative(fs::path(script), fs::path(script_path)));
            // turn the .cpp at the end to .h
            scr_path = scr_path.substr(0, scr_path.size() - 4);
            inc_sec << "#include \"scripts/" + scr_path + ".h\"\n";
            src_main_sec << "engine->registerScript(\"" + scr_path + "\", " + dispatch_string(scr_path) + ");\n";
        }

        std::vector<std::string> scenes;
        if (fs::exists(scene_path)) {
            for (const auto& entry: fs::recursive_directory_iterator(scene_path)) {
                if (endswith(entry.path().string(), ".json")) {
                    scenes.push_back(cify_path(entry.path()));
                }
            }
        } else {
            std::cerr << "Warning: Scene path not found" << std::endl;
        }

        for (auto &scene: scenes) {
            parse_scene(scene);
            std::string scn_path = cify_path(fs::relative(fs::path(scene), fs::path(scene_path)));
            // turn the .json at the end to .h
            scn_path = scn_path.substr(0, scn_path.size() - 5);
            // scenes are not allowed to be in subdirectories
            // if we detect a subdirectory, print error
            if (scn_path.find('/') != std::string::npos) {
                std::cerr << "Error: Scene file in subdirectory" << std::endl;
                continue;
            }
            inc_sec << "#include \"scenes/" + scn_path + ".h\"\n";
            src_main_sec << "engine->addScene(\"" + scn_path + "\", new " + scn_path + "(engine));\n";
        }

        for (auto &asset: assets) {
            if (asset.type == AssetType::Compile) {
                std::string b = cify_path(fs::relative(fs::path(asset.dst), fs::path(build)));
                std::string c = asset.src;
                inc_sec << "#include \"" + b + ".h\"\n";
                src_main_sec << "engine->addAsset(\"" + c + "\", CompiledAsset(" + asset.name + ", " + asset.name + "_len));\n";
            } else if (asset.type == AssetType::Copy) {
                src_main_sec << "engine->addAsset(\"" + asset.src + "\", CopiedAsset(\"" + asset.src + "\"));\n";
            } else {
                std::cerr << "Error: unknown asset type" << std::endl;
                std::cerr << "Should never happen" << std::endl;
            }
        }

        src_main_sec << "engine->loadConfig(\"config.json\");\n";
        if (splash_enabled) {
            src_main_sec << "if (isCompiled) {\n";
#ifdef SPLASH_DELAY
            src_main_sec << "    auto start = std::chrono::high_resolution_clock::now();\n";
            src_main_sec << "    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() < 3) {}\n";
#endif
            src_main_sec << "    engine->endSplash();\n";
        }

        src_main_sec << "engine->run();\n";
        src_main_sec << "delete engine;\n";
        src_main_sec << "return 0;\n";

        std::ofstream main_file(fs::path(build) / "main.cpp");
        main_file << "#ifdef _WIN32\n";
        main_file << "#define EXPORT __declspec(dllexport)\n";
        main_file << "#else\n";
        main_file << "#define EXPORT\n";
        main_file << "#endif\n";
        main_file << inc_sec.str();
        main_file << "\n\nextern \"C\" EXPORT int mainGame(bool isCompiled) {\n    ";
        main_file << indent(src_main_sec.str(), 4);
        main_file << "\n}\n";
        main_file << "\nint main() {\n    ";
        main_file << "return mainGame(true);\n";
        main_file << "}\n";
        main_file.close();

    }

    void parse_script(const std::string& scr_loc) {
        if (!fs::exists(scr_loc)) {
            std::cerr << "Error: Script file not found" << std::endl;
            return;
        }
        fs::path file = fs::relative(fs::path(scr_loc), fs::path(script_path));
        std::cout << "SCRIPT: '" << file.generic_string() << "'\n";
        fs::path out = fs::path(build) / "scripts" / file;
        file.replace_extension("");
        std::string outsrc = cify_path(file);

        // replace .cpp with .h
        fs::path out_h = out;
        out_h.replace_extension(".h");

        if (!fs::exists(out.parent_path())) {
            fs::create_directories(out.parent_path());
        }
        std::ifstream scr_file(scr_loc);
        std::ofstream out_file(out);
        out_file << VERSION_CHECK_CPP;
        out_file << "#include " << '"' << cify_path(out_h) << '"' << "\n";
        // put all contents of scr_loc into out_file
        out_file << scr_file.rdbuf();
        scr_file.close();
        out_file << "ScriptInterface* " << dispatch_string(outsrc) << "(Engine* e, GameObject* o) {\n";
        out_file << "    return new " << outsrc << "(e, o);\n";
        out_file << "}\n";
        out_file.close();
        sources.push_back(cify_path(out));

        out_file.open(out_h);
        out_file << "#pragma once\n";
        out_file << VERSION_CHECK_CPP;
        out_file << "#include <Engine/Script.h>\n";
        out_file << "ScriptInterface* " << dispatch_string(outsrc) << "(Engine* e, GameObject* o);\n";
        out_file.close();
        sources.push_back(cify_path(out_h));
    }
    void parse_scene(const std::string& scn_loc) {
        std::ostringstream inc_sec;
        std::ostringstream src_con_sec;
        std::ostringstream src_set_sec;
        std::ostringstream src_upd_sec;
        inc_sec << VERSION_CHECK_CPP;

        fs::path rel_loc = fs::relative(fs::path(scn_loc), fs::path(scene_path));
        std::string scene_name = rel_loc.replace_extension("").generic_string();
        std::cout << "SCENE: '" << rel_loc.generic_string() << "'\n";

        json j;
        std::ifstream scn_file(scn_loc);
        scn_file >> j;
        scn_file.close();

        inc_sec << "#include \"" << cify_path(rel_loc.replace_extension(".h")) << "\"\n";
        if (!fs::exists(scn_loc)) {
            std::cerr << "Error: Scene file not found" << std::endl;
            return;
        }
        if (!fs::exists(fs::path(build) / "scenes")) {
            fs::create_directories(fs::path(build) / "scenes");
        }
        fs::path out = fs::path(build) / "scenes" / rel_loc;
        out.replace_extension(".cpp");
        fs::path out_h = out;
        out_h.replace_extension(".h");

        json dat = verify_json(j, JsonID::Scene);

        if (dat.find("name") == dat.end()) {
            std::cerr << "Error: Scene missing name" << std::endl;
            return;
        }
        if (dat["name"] != scene_name) {
            std::cerr << "Error: Scene name mismatch" << std::endl;
            return;
        }

        src_con_sec << "this->name = \"" << scene_name << "\";\n";

        if (dat.find("3d") == dat.end())
            src_con_sec << "this->mode = SceneMode::Dimension2D;\n";
        else {
            if (dat["3d"])
                src_con_sec << "this->mode = SceneMode::Dimension3D;\n";
            else
                src_con_sec << "this->mode = SceneMode::Dimension2D;\n";
        }

        json content;

        if (dat.find("content") == dat.end())
            content = {};
        else
            content = dat["content"];

        if (!content.is_array()) {
            std::cerr << "Error: Scene content is not an array!" << std::endl;
            return;
        }

        var_count = 0;
        for (auto& obj: content) {
            parse_object(obj, src_con_sec, src_set_sec, src_upd_sec);
        }

        src_set_sec << "Scene::Setup();\n";
        src_upd_sec << "Scene::Update();\n";

        std::ofstream out_file(out);
        out_file << inc_sec.str();
        out_file << '\n' << scene_name << "::" << scene_name << "(Engine* e) : Scene(e) {\n    ";
        out_file << indent(src_con_sec.str(), 4);
        out_file << "}\n\n";
        out_file << "void " << scene_name << "::Setup() {\n    ";
        out_file << indent(src_set_sec.str(), 4);
        out_file << "}\n\n";
        out_file << "void " << scene_name << "::Update() {\n    ";
        out_file << indent(src_upd_sec.str(), 4);
        out_file << "}\n";
        out_file.close();
        sources.push_back(cify_path(out));

        out_file.open(out_h);
        out_file << "#pragma once\n";
        out_file << VERSION_CHECK_CPP;
        out_file << "#include <Engine/internal/Scene.h>\n";
        out_file << "#include <Engine/internal/Engine.h>\n";
        out_file << "class " << scene_name << " : public Scene {\n";
        out_file << "public:\n";
        out_file << "    explicit " << scene_name << "(Engine* e);\n";
        out_file << "    void Setup() override;\n";
        out_file << "    void Update() override;\n";
        out_file << "};\n";
        out_file.close();
        sources.push_back(cify_path(out_h));
    }

    std::string parse_object(json obj, std::ostringstream& src_con_sec, std::ostringstream& src_set_sec, std::ostringstream& src_upd_sec, bool child=false) {
        if (obj.find("id") == obj.end()) {
            std::cerr << "Error: Object missing id" << std::endl;
            return "";
        }
        // same as main id sanitization
        std::string obj_id = obj["id"];
        std::string obj_id_san;
        for (char c: obj_id) {
            if (obj_id_san.empty()) {
                if (std::isalnum(c) || c == '_') {
                    obj_id_san += c;
                } else {
                    std::cout << "Warning: Object id contains invalid characters, removing" << std::endl;
                }
            } else {
                if (std::isalnum(c) || c == '_') {
                    obj_id_san += c;
                } else {
                    std::cout << "Warning: Object id contains invalid characters, removing" << std::endl;
                }
            }
        }
        if (obj_id_san.empty()) {
            obj_id_san = "object";
            std::cout << "Warning: Object id is empty, using 'object'" << std::endl;
        }

        std::string go_id = "_GameObject_p_" + std::to_string(var_count++);

        src_con_sec << "auto* " << go_id << " = new GameObject(\"" << obj_id_san << "\");\n";
        if (obj.find("attributes") != obj.end()) {
            if (!obj["attributes"].is_array()) {
                std::cerr << "Error: Object attributes is not an array!" << std::endl;
                return "";
            }
            for (auto& attr: obj["attributes"]) {
                if (attr.find("type") == attr.end()) {
                    std::cerr << "Error: Attribute missing type" << std::endl;
                    return "";
                }
                std::string attrd_id = "_AttributeData_" + std::to_string(var_count++);
                src_con_sec << "AttributeData " << attrd_id << ";\n";
                if (attr["type"] == "script") {
                    if (attr.find("location") == attr.end()) {
                        std::cerr << "Error: Script attribute missing location" << std::endl;
                        return "";
                    }
                    src_con_sec << go_id << "->addAttribute(new Attribute(e->dispatchScript(\""
                        << std::string(attr["location"]) << "\", " << go_id << "), " << attrd_id << ", \""
                        << std::string(attr["location"]) << "\"));\n";
                } else if (attr["type"] == "builtin") {
                    if (attr.find("id") == attr.end()) {
                        std::cerr << "Error: Builtin attribute missing id" << std::endl;
                        return "";
                    }
                    if (attr.find("data") != attr.end()) {
                        src_con_sec << parse_attr_data(attr["data"], attrd_id);
                    }
                    src_con_sec << go_id << "->addAttribute(new Attribute(e, \"" << std::string(attr["id"]) << "\", " << attrd_id << "));\n";
                } else {
                    std::cerr << "Error: Unknown attribute type" << std::endl;
                    return "";
                }
            }
        }

        if (obj.find("children") != obj.end()) {
            if (!obj["children"].is_array()) {
                std::cerr << "Error: Object children is not an array!" << std::endl;
                return "";
            }
            for (auto& child: obj["children"]) {
                std::string cid = parse_object(child, src_con_sec, src_set_sec, src_upd_sec, true);
                src_con_sec << go_id << "->addObject(" << cid << ");\n";
            }

            if (!child) {
                src_con_sec << "this->addObject(" << go_id << ");\n";
            }


        }

        return go_id;

    }

};

int main(int argc, char* argv[]) {
    std::cout << "JICC Compiler v2" << std::endl;
//    JiccCompiler jicc("C:/Users/wyatt/Desktop/structure/projects/testproject/", "C:/Users/wyatt/Desktop/structure/projects/testproject/build/");
    // compile from args
    if (argc < 3) {
        std::cerr << "Usage: jicc <project_path> <build_path>" << std::endl;
        return 1;
    }
    JiccCompiler jicc(argv[1], argv[2]);

    return 0;
}