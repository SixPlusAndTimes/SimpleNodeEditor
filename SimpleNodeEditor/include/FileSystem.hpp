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
    stdfs::path m_path;
    Path() = default;
    ~Path() = default;
    Path(const std::string pathName): m_path{pathName} { }
    Path(const stdfs::path& path): m_path{path} { }

    std::string String()
    {
        return m_path.string();
    }

    const std::string String() const
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
        auto pathJoined = (m_path / other.m_path).string();
        std::replace(pathJoined.begin(), pathJoined.end() , '\\' , '/');
        return Path(pathJoined);
    }

};

struct FileEntry {
	std::string name;
	std::string fullPath;
	bool isDirectory = false;
	uint64_t size = 0;
	std::time_t modified = 0;
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


	virtual std::string GetName(const Path& path) = 0;
	virtual std::string GetParent(const Path& path) = 0;
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

	virtual std::string GetName(const Path& path) override;
	virtual std::string GetParent(const Path& path) override;
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

    virtual std::string GetName(const Path& path) override;
    virtual std::string GetParent(const Path& path) override;

private:
    bool Connect();
    void Disconnect();
    bool InitSftp();
    std::string CheckError();

    // Configuration
    std::string m_hostAddr;
    std::string m_port;
    std::string m_username;
    std::string m_password;
    std::string m_publicKey;
    std::string m_privateKey;

    // Connection context
    int  m_socket;
    void* m_session;      // SSH session handle
    void* m_sftpSession; // SFTP session handle
};

} // namespace FS

} // namespace SimpleNodeEditor

#endif // FILESYSTEM_H