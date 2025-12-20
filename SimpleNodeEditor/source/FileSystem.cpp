#include "FileSystem.hpp"

#include <fstream>
#include <system_error>
#include <chrono>


namespace SimpleNodeEditor {
namespace FS
{
static std::time_t file_time_to_time_t(const stdfs::file_time_type& ftime) {
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(
        ftime - stdfs::file_time_type::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}

bool LocalFileSystem::Exists(const Path& path) {
    return stdfs::exists(path.m_path);
}

bool LocalFileSystem::IsDirectory(const Path& path) {
    std::error_code ec;
    return stdfs::is_directory(path.m_path, ec) && !ec;
}

std::vector<FileEntry> LocalFileSystem::List(const Path& path){
    std::vector<FileEntry> out;
    if (!stdfs::exists(path.m_path) || !stdfs::is_directory(path.m_path)) return out;

    for (auto &p : stdfs::directory_iterator(path.m_path)) {
        FileEntry e;
        e.fullPath = p.path().string();
        e.name = p.path().filename().string();
        e.isDirectory = stdfs::is_directory(p.path());
        if (!e.isDirectory) {
            std::error_code ec2;
            e.size = stdfs::file_size(p.path(), ec2);
        }
        out.push_back(std::move(e));
    }
    return out;
}


std::string LocalFileSystem::GetName(const Path& path) {
    return stdfs::path(path.m_path).filename().string();
}

std::string LocalFileSystem::GetParent(const Path& path) {
    return stdfs::path(path.m_path).parent_path().string();
}

// char  LocalFileSystem::separator() {
//     return stdfs::path::preferred_separator;
// }

// bool LocalFileSystem::createDirectory(const std::string& path) {
//     std::error_code ec;
//     return stdfs::create_directories(path, ec) && !ec;
// }

//     bool remove(const std::string& path) {
//         std::error_code ec;
//         auto removed = stdfs::remove_all(path, ec);
//         return !ec && removed > 0;
//     }

    // bool readAllText(const std::string& path, std::string& out) const {
    //     std::ifstream ifs(path, std::ios::binary);
    //     if (!ifs) return false;
    //     std::string contents;
    //     ifs.seekg(0, std::ios::end);
    //     auto size = ifs.tellg();
    //     if (size <= 0) { out.clear(); return true; }
    //     contents.resize(static_cast<size_t>(size));
    //     ifs.seekg(0, std::ios::beg);
    //     ifs.read(&contents[0], contents.size());
    //     out.swap(contents);
    //     return true;
    // }

    // bool writeAllText(const std::string& path, const std::string& data) {
    //     std::ofstream ofs(path, std::ios::binary);
    //     if (!ofs) return false;
    //     ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
    //     return bool(ofs);
    // }

    // std::string join(const std::string& a, const std::string& b){
    //     stdfs::path pa(a);
    //     pa /= b;
    //     return pa.string();
    // }


} // namespace FS
} // namespace SimpleNodeEditor
