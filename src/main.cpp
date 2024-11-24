// SPDX-License-Identifier: GPL-3.0-only

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

// for watching files
#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#define WATCH_SUPPORTED 1
#endif
#ifdef _WIN32
#include <windows.h>
#define WATCH_SUPPORTED 1
#define WINDOWS_FORMAT_ERROR(buffer) \
    FormatMessage( \
        FORMAT_MESSAGE_FROM_SYSTEM | \
        FORMAT_MESSAGE_ALLOCATE_BUFFER | \
        FORMAT_MESSAGE_IGNORE_INSERTS, \
        nullptr, \
        GetLastError(), \
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
        (LPTSTR)&buffer, \
        0, \
        nullptr \
    );
#endif
#include <libPLCL.hpp>

#include "CSS.hpp"
#include "Cli.hpp"
#include "HTML.hpp"

using namespace PLCL;

static bool compileFile(const std::filesystem::path &file, const Cli &cli) {
    std::ifstream ifs(file);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
    Config::ConfigRoot config = Config::ConfigRoot::fromString(content);
    std::string extension = file.extension().string();
    std::string output = (cli.output / file.stem()).string();
    if (Generic::iequals(extension, ".p(l)clhtml")) {
        output += ".html";
        std::string result = parseHTML(config, !cli.dontMinify, cli.indent);
        std::ofstream ofs(output);
        ofs << result;
    } else if (Generic::iequals(extension, ".p(l)clcss")) {
        output += ".css";
        std::string result = parseCSS(config, !cli.dontMinify, cli.indent);
        std::ofstream ofs(output);
        ofs << result;
    } else {
        std::cerr << "Unknown extension " << extension << std::endl;
        return false;
    }
    return true;
}

#ifdef __linux
static void handle_events(int fd, const std::vector<int> &wds, const std::vector<std::filesystem::path> &parent_paths, const Cli &cli) {
    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;
    char *ptr;
    while (true) {
        len = read(fd, buf, sizeof(buf));
        if (len == -1 && errno != EAGAIN) {
            std::cerr << "Failed to read inotify events\n" << strerror(errno) << std::endl;
            return;
        }
        if (len <= 0) {
            continue;
        }
        for (size_t i = 0; i < len; i += sizeof(struct inotify_event) + event->len) {
            event = (const struct inotify_event *) &buf[i];
            if (event->mask & IN_MODIFY) {
                for (size_t i = 0; i < wds.size(); i++) {
                    if (event->wd == wds[i]) {
                        if (event->len == 0) {
                            std::cerr << "Event with no name" << std::endl;
                            return;
                        }
                        std::filesystem::path file = parent_paths[i] / event->name;
                        if (std::ranges::find(cli.files, file) != cli.files.end()) {
                            std::cout << "Recompiling " << file.string() << std::endl;
                            compileFile(file, cli);
                        }
                    }
                }
            }
            if (event->mask & IN_IGNORED) {
                std::cout << "File ignored" << std::endl;
            }
        }
    }
}
#endif
#ifdef _WIN32
static void handle_events(const std::map<HANDLE, OVERLAPPED *> &api_data, uint8_t *buffer, const Cli &cli) {
    while (true) {
        for (auto &[handle, overlap] : api_data) {
            DWORD result = WaitForSingleObject(overlap->hEvent, 0);

            if (result == WAIT_OBJECT_0) {
                DWORD transferred;
                if (GetOverlappedResult(handle, overlap, &transferred, FALSE) == 0) {
                    LPTSTR error = nullptr;
                    WINDOWS_FORMAT_ERROR(error)
                    if (error != nullptr) {
                        std::cerr << "Failed to get overlapped result.\n" << error << std::endl;
                        LocalFree(error);
                    }
                    std::exit(EXIT_FAILURE);
                }

                auto event = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(buffer);

                while (true) {
                    DWORD name_length = event->FileNameLength / sizeof(wchar_t);
                    std::wstring ws(event->FileName, name_length);
                    std::string name(ws.begin(), ws.end());

                    char dir_name_buffer[8192];
                    DWORD length = GetFinalPathNameByHandle(handle, dir_name_buffer, 8192, FILE_NAME_NORMALIZED);
                    if (length == 0) {
                        LPTSTR error = nullptr;
                        WINDOWS_FORMAT_ERROR(error)
                        if (error != nullptr) {
                            std::cerr << "Couldn't get path name for " << name << "\n" << error << std::endl;
                            LocalFree(error);
                        }
                        std::exit(EXIT_FAILURE);
                    }
                    if (dir_name_buffer[length] != '\0') length++;

                    std::string dir_name (dir_name_buffer + 4, length - 4);
                    std::filesystem::path file(dir_name);
                    file = file.lexically_normal() / name;

                    if (std::ranges::find_if(cli.files, [&file](const std::filesystem::path &path) {
                        return path == file;
                    }) != cli.files.end()) {
                        std::cout << "Recompiling " << file.string() << std::endl;
                        compileFile(file, cli);
                    }

                    if (event->NextEntryOffset) {
                        *((uint8_t**)&event) += event->NextEntryOffset;
                    } else {
                        break;
                    }
                }
                overlap->hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
                WINBOOL success = ReadDirectoryChangesW(handle,
                                                        buffer,
                                                        1024,
                                                        FALSE,
                                                        FILE_NOTIFY_CHANGE_SIZE,
                                                        nullptr,
                                                        overlap,
                                                        nullptr
                                                        );
                if (success == 0) {
                    LPTSTR error = nullptr;
                    WINDOWS_FORMAT_ERROR(error)
                    if (error != nullptr) {
                        std::cerr << "Failed to watch directory\n" << error << std::endl;
                        LocalFree(error);
                    }
                    std::exit(EXIT_FAILURE);
                }
            }
        }
    }
}
#endif

int main(int argc, char *argv[]) {
    Cli cli(argc, argv);
    if (cli.version) {
        cli.printVersion();
        return EXIT_SUCCESS;
    }
    if (cli.help) {
        cli.printHelp();
        return EXIT_SUCCESS;
    }
    std::filesystem::create_directories(cli.output);
    auto it = std::ranges::remove_if(cli.files, [](const std::filesystem::path &file) {
        if (!std::filesystem::exists(file)) {
            std::cerr << "File " << file << " does not exist" << std::endl;
            return true;
        }
        if (std::filesystem::is_directory(file)) {
            std::cerr << "File " << file << " is a directory" << std::endl;
            return true;
        }
        std::string extension = file.extension().string();
        if (!Generic::iequals(extension, ".p(l)clhtml") && !Generic::iequals(extension, ".p(l)clcss")) {
            std::cerr << "Unknown extension " << extension << std::endl;
            return true;
        }
        return false;
    });
    cli.files.erase(it.begin(), it.end());
    if (cli.files.empty()) {
        std::cerr << "No valid files to compile" << std::endl;
        return EXIT_FAILURE;
    }
    if (cli.watch) {
#ifndef WATCH_SUPPORTED
        std::cerr << "Watching files is not supported on this platform" << std::endl;
        return EXIT_FAILURE;
#endif
#ifdef __linux__
        int fd = inotify_init1(IN_NONBLOCK);
        std::vector<std::filesystem::path> parent_paths;
        std::vector<int> wds;
        if (fd == -1) {
            std::cerr << "Failed to initialize inotify\n" << strerror(errno) << std::endl;
            return EXIT_FAILURE;
        }
        for (const auto &file : cli.files) {
            if (!std::filesystem::exists(file.parent_path())) {
                std::cerr << "Parent path " << file.parent_path() << " does not exist" << std::endl;
                return EXIT_FAILURE;
            }

            std::filesystem::path parent = file.parent_path();
            if (std::ranges::find(parent_paths, parent) != parent_paths.end()) {
                continue;
            }

            int wd = inotify_add_watch(fd, parent.c_str(), IN_MODIFY);
            if (wd == -1) {
                std::cerr << "Failed to add watch\n" << strerror(errno) << std::endl;
                return EXIT_FAILURE;
            }

            std::cout << "Compiling " << file.string() << std::endl;
            compileFile(file, cli);

            wds.push_back(wd);
            parent_paths.push_back(parent);
        }
        handle_events(fd, wds, parent_paths, cli);
        close(fd);
#endif
#ifdef _WIN32
        std::vector<std::filesystem::path> parent_paths;
        std::map<HANDLE, OVERLAPPED *> api_data;
        alignas(DWORD) uint8_t change_buffer[1024];

        for (const auto &file : cli.files) {
            if (!std::filesystem::exists(file.parent_path())) {
                std::cerr << "Parent path " << file.parent_path() << " does not exist" << std::endl;
                return EXIT_FAILURE;
            }
            std::filesystem::path parent = file.parent_path();
            if (std::ranges::find(parent_paths, parent) != parent_paths.end()) {
                continue;
            }

            HANDLE pathHandle = CreateFile(parent.string().c_str(), GENERIC_READ, FILE_SHARE_VALID_FLAGS, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
            if (pathHandle == INVALID_HANDLE_VALUE) {
                std::cerr << "Failed to open directory " << parent << '\n' << strerror(errno) << std::endl;
                return EXIT_FAILURE;
            }

            OVERLAPPED overlapped;
            overlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

            WINBOOL success = ReadDirectoryChangesW(pathHandle,
                                                    change_buffer,
                                                    1024,
                                                    FALSE,
                                                    FILE_NOTIFY_CHANGE_SIZE,
                                                    nullptr,
                                                    &overlapped,
                                                    nullptr
                                                    );
            if (success == 0) {
                std::cerr << "Failed to watch directory " << parent << "\nError code: " << GetLastError() << std::endl;
                return EXIT_FAILURE;
            }

            std::cout << "Compiling " << file.string() << std::endl;
            compileFile(file, cli);

            api_data.emplace(pathHandle, &overlapped);
        }
        handle_events(api_data, change_buffer, cli);
#endif
        return 0;
    }

    bool failed = false;

    for (const auto &file : cli.files) {
        failed = !compileFile(file, cli);
    }

    if (failed) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
