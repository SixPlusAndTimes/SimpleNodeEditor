// #pragma once

// #include <string>
// #include <vector>
// #include <cstdint>
// #include <ctime>

// namespace SimpleNodeEditor {
// class FileSystem
// {
//     class FilePath
//     {
//         public:
//             std::string string();
//     };

//     class FileEntry
//     {

//     };

//     // ssh filesystem 
//     // if you open remote file , ssh will download it from remote machine to local storage
//     // and then 
//     class IFileSystem {
//     public:
//         virtual ~IFileSystem() = default;

//         virtual bool exists(const std::string& path) const = 0;
//         virtual bool isDirectory(const std::string& path) const = 0;
//         virtual std::vector<FileEntry> list(const std::string& path) const = 0;

//         virtual bool createDirectory(const std::string& path) = 0;
//         virtual bool remove(const std::string& path) = 0;

//         virtual bool readAllText(const std::string& path, std::string& out) const = 0;
//         virtual bool writeAllText(const std::string& path, const std::string& data) = 0;

//         virtual std::string join(const std::string& a, const std::string& b) const = 0;
//         virtual std::string getName(const std::string& path) const = 0;
//         virtual std::string getParent(const std::string& path) const = 0;
//         virtual char separator() const = 0;
//     };
// } // namespace FS


// } // namespace SimpleNodeEditor
// #ifndef FILESYSTEM_H
// #define FILESYSTEM_H



// #endif // FILESYSTEM_H