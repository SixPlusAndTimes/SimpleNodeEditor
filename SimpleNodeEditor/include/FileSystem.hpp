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
// std::filesystem::path is a general class which is not tied to a specific operating system
// so we can reuse std::filesystem::path as our sshfilesystems's "path impl backend" 
// https://en.cppreference.com/w/cpp/filesystem/path.html
struct Path
{
    Path() = default;
    ~Path() = default;
    Path(const Path& other) = default;
    Path& operator=(const Path& other) = default;
    Path(Path&& other): m_pathimpl(std::move(other.m_pathimpl)){ }
    Path& operator=(Path&& other)
    {
        if (this != &other)
        {
            m_pathimpl = std::move(other.m_pathimpl);
        }
        return *this;
    }

    Path(const std::string pathName): m_pathimpl{pathName} { }
    Path(const stdfs::path& path): m_pathimpl{path} { }

    std::string String()
    {
        return m_pathimpl.string();
    }

    const std::string String() const
    {
        return m_pathimpl.string();
    }

    std::string GetFileName() const
    {
        return m_pathimpl.filename().string();
    }
    std::string GetFileName()
    {
        return m_pathimpl.filename().string();
    }
    bool HasParentPath()
    {
        return m_pathimpl.has_parent_path();
    }

    Path ParentPath()
    {
        return Path(m_pathimpl.parent_path());
    }

    Path operator/(const Path& other)
    {
        auto pathJoined = (m_pathimpl / other.m_pathimpl).string();
        std::replace(pathJoined.begin(), pathJoined.end() , '\\' , '/');
        return Path(pathJoined);
    }

    Path operator/(const std::string& other)
    {
        auto pathJoined = (m_pathimpl / Path(other).m_pathimpl).string();
        std::replace(pathJoined.begin(), pathJoined.end() , '\\' , '/');
        return Path(pathJoined);
    }
    
    Path operator/(const std::string& other) const
    {
        auto pathJoined = (m_pathimpl / Path(other).m_pathimpl).string();
        std::replace(pathJoined.begin(), pathJoined.end() , '\\' , '/');
        return Path(pathJoined);
    }

    stdfs::path m_pathimpl;
};

struct FileEntry {
    FS::Path m_path; // include filename
	bool m_isDirectory = false;
	uint64_t m_size = 0;
	std::time_t m_modified = 0;
};

enum class FileSystemType
{
    Local,
    Ssh,
    Unkonwn
};
class IFileSystem {
public:
    IFileSystem(FileSystemType type) : m_type{type} {}
	virtual ~IFileSystem() = default;

	virtual bool Exists(const Path& path) = 0;
	virtual bool IsDirectory(const Path& path) = 0;
	virtual std::vector<FileEntry> List(const Path& path) = 0;

    FileSystemType GetFileSystemType() {return m_type;};
protected:
    FileSystemType m_type;
};

class LocalFileSystem : public IFileSystem
{
public:
    LocalFileSystem();
    virtual ~LocalFileSystem() = default;

    virtual bool Exists(const Path& path) override;
	virtual bool IsDirectory(const Path& path) override;
	virtual std::vector<FileEntry> List(const Path& path) override;
};

struct SshConnectionInfo
{
    std::string m_hostAddr;
    std::string m_port;
    std::string m_username;
    std::string m_password;
    std::string m_publicKey;
    std::string m_privateKey;
};

class SshFileSystem : public IFileSystem
{
public:
    SshFileSystem( 
        const std::string & host, 
        const std::string& port,
        const std::string & username,
        const std::string & password,
        const std::string & publicKey,
        const std::string & privateKey
    );
    
    SshFileSystem(const SshConnectionInfo& connectionInfo);

    virtual ~SshFileSystem();


    virtual bool Exists(const Path& path) override;
    virtual bool IsDirectory(const Path& path) override;
    virtual std::vector<FileEntry> List(const Path& path) override;


    bool IsConnected() const;

private:
    void Connect();
    void Disconnect();
    void CheckError();

    // Configuration
    std::string m_hostAddr;
    std::string m_port;
    std::string m_username;
    std::string m_password;
    std::string m_publicKey;
    std::string m_privateKey;

    // Connection context
    std::int64_t m_socket;
    void*        m_session;      // SSH session handle
    void*        m_sftpSession; // SFTP session handle
};

} // namespace FS

} // namespace SimpleNodeEditor

#endif // FILESYSTEM_H