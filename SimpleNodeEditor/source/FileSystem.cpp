// #include "../include/Filesystem.hpp"

// #include <filesystem>
// #include <fstream>
// #include <system_error>
// #include <chrono>

// namespace fs = std::filesystem;

// namespace sne {

// static std::time_t file_time_to_time_t(const fs::file_time_type& ftime) {
//     using namespace std::chrono;
//     auto sctp = time_point_cast<system_clock::duration>(
//         ftime - fs::file_time_type::clock::now() + system_clock::now());
//     return system_clock::to_time_t(sctp);
// }

// class LocalFileSystem : public IFileSystem {
// public:
//     bool exists(const std::string& path) const override {
//         return fs::exists(path);
//     }

//     bool isDirectory(const std::string& path) const override {
//         return fs::is_directory(path);
//     }

//     std::vector<FileEntry> list(const std::string& path) const override {
//         std::vector<FileEntry> out;
//         std::error_code ec;
//         if (!fs::exists(path, ec) || !fs::is_directory(path, ec)) return out;

//         for (auto &p : fs::directory_iterator(path, ec)) {
//             FileEntry e;
//             e.fullPath = p.path().string();
//             e.name = p.path().filename().string();
//             e.isDirectory = fs::is_directory(p.path(), ec);
//             if (!e.isDirectory) {
//                 std::error_code ec2;
//                 e.size = fs::file_size(p.path(), ec2);
//             }
//             std::error_code ec3;
//             e.modified = file_time_to_time_t(fs::last_write_time(p.path(), ec3));
//             out.push_back(std::move(e));
//         }
//         return out;
//     }

//     bool createDirectory(const std::string& path) override {
//         std::error_code ec;
//         return fs::create_directories(path, ec) && !ec;
//     }

//     bool remove(const std::string& path) override {
//         std::error_code ec;
//         auto removed = fs::remove_all(path, ec);
//         return !ec && removed > 0;
//     }

//     bool readAllText(const std::string& path, std::string& out) const override {
//         std::ifstream ifs(path, std::ios::binary);
//         if (!ifs) return false;
//         std::string contents;
//         ifs.seekg(0, std::ios::end);
//         contents.resize(static_cast<size_t>(ifs.tellg()));
//         ifs.seekg(0, std::ios::beg);
//         ifs.read(&contents[0], contents.size());
//         out.swap(contents);
//         return true;
//     }

//     bool writeAllText(const std::string& path, const std::string& data) override {
//         std::ofstream ofs(path, std::ios::binary);
//         if (!ofs) return false;
//         ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
//         return bool(ofs);
//     }

//     std::string join(const std::string& a, const std::string& b) const override {
//         fs::path pa(a);
//         pa /= b;
//         return pa.string();
//     }

//     std::string getName(const std::string& path) const override {
//         return fs::path(path).filename().string();
//     }

//     std::string getParent(const std::string& path) const override {
//         return fs::path(path).parent_path().string();
//     }

//     char separator() const override {
//         return fs::path::preferred_separator;
//     }
// };

// // Factory helper
// std::unique_ptr<IFileSystem> createLocalFileSystem() {
//     return std::make_unique<LocalFileSystem>();
// }

// } // namespace sne
// #include "FileSystem.hpp"