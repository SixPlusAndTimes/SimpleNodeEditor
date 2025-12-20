#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>
#include <memory>
#include <filesystem>

namespace stdfs = std::filesystem;
namespace SimpleNodeEditor {
namespace FS
{
// std::filesystem::path is a general class which is not tied to localsystem
// so we can reuse this as our sshfilesystems's "path impl backend"
struct Path
{
    stdfs::path m_path;
    Path() = default;
    ~Path() = default;
    Path(const std::string pathName): m_path{pathName} { }
    Path(const stdfs::path& path): m_path{path} { }
    std::string String()
    {
        return m_path.string();
    }
    Path& operator=(const std::string& pathstring)
    {
        m_path = pathstring;
        return *this;
    }
    bool HasParentPath()
    {
        return m_path.has_parent_path();
    }
    Path ParentPath()
    {
        return Path(m_path.parent_path());
    }
    Path operator/(const Path& other)
    {
        return m_path / other.m_path;
    }

};
struct FileEntry {
	std::string name;
	std::string fullPath;
	bool isDirectory = false;
	uint64_t size = 0;
	std::time_t modified = 0;
};

class IFileSystem {
public:
    IFileSystem() = default;
	virtual ~IFileSystem() = default;

	virtual bool exists(const std::string& path) = 0;
	virtual bool isDirectory(const std::string& path) = 0;
	virtual std::vector<FileEntry> list(const std::string& path) = 0;


	virtual std::string getName(const std::string& path) = 0;
	virtual std::string getParent(const std::string& path) = 0;
	// virtual char separator() = 0;

	// virtual std::string join(const std::string& a, const std::string& b) = 0;
    // virtual bool createDirectory(const std::string& path) = 0;
	// virtual bool remove(const std::string& path) = 0;

	// virtual bool readAllText(const std::string& path, std::string& out) const = 0;
	// virtual bool writeAllText(const std::string& path, const std::string& data) = 0;
};

class LocalFileSystem : public IFileSystem
{
public:
    LocalFileSystem() = default;
    virtual ~LocalFileSystem() = default;

    virtual bool exists(const std::string& path) override;
	virtual bool isDirectory(const std::string& path) override;
	virtual std::vector<FileEntry> list(const std::string& path) override;

	virtual std::string getName(const std::string& path) override;
	virtual std::string getParent(const std::string& path) override;
};

} // namespace FS

} // namespace SimpleNodeEditor

#endif // FILESYSTEM_H