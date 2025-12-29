// the structure of this file:
//
// [SECTION1] implementation of Path and FileEntry
// [SECTION2] draw list helper
// [SECTION3] declearations of IFileSystem 、LocalFileSystem、SshFileSystem and other components in the following order: 
//          [SECTION3.1] IFileSystem
//          [SECTION3.2] LocalFileSystem
//          [SECTION3.3] SShFileSystem
//          [SECTION3.4] SShFileSystem Input&Output streambuffer

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>
#include <memory>
#include <filesystem>
#include "Common.hpp"

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

    virtual std::unique_ptr<std::istream> createInputStream(std::ios_base::openmode mode, const Path& path) const = 0;
    virtual std::unique_ptr<std::ostream> createOutputStream(std::ios_base::openmode mode, const Path& path) = 0;

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
    virtual std::unique_ptr<std::istream> createInputStream(std::ios_base::openmode mode, const Path& path) const;
    virtual std::unique_ptr<std::ostream> createOutputStream(std::ios_base::openmode mode, const Path& path);
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


class SshFileSystem : public IFileSystem, public std::enable_shared_from_this<SshFileSystem>
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
    virtual std::unique_ptr<std::istream> createInputStream(std::ios_base::openmode mode, const Path& path) const;
    virtual std::unique_ptr<std::ostream> createOutputStream(std::ios_base::openmode mode, const Path& path);
    void*   GetSshSessionHandle(){ return m_session;}
    void*   GetSftpSessionHandle(){ return m_sftpSession;}

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

class SshInputStreamBuffer : public std::streambuf
{
public:
    SshInputStreamBuffer(std::shared_ptr<SshFileSystem> fs, const FS::Path & path, std::ios_base::openmode mode, size_t bufferSize = size_kb(32), size_t putBackSize = size_b(128));

    virtual ~SshInputStreamBuffer();

    virtual std::streambuf::int_type underflow() override;
    virtual pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which) override;
    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode which) override;


protected:
    std::shared_ptr<SshFileSystem>   m_fs;         
    const FS::Path                   m_path;       
    const size_t                     m_putbackSize;
    void*                            m_file; // actually LIBSSH2_SFTP*
    std::vector<char>                m_buffer;
};

class OutputStream : public std::ostream
{
public:
    explicit OutputStream(std::streambuf * sb): std::ostream(sb) , m_sb(sb) {}
    virtual ~OutputStream() {m_sb->pubsync(); delete m_sb;}

protected:
    std::streambuf * m_sb;
};

class SshOutputStreamBuffer : public std::streambuf
{
public:
    SshOutputStreamBuffer(std::shared_ptr<SshFileSystem> fs, const FS::Path& path, std::ios_base::openmode mode, size_t bufferSize = size_kb(24));

    virtual ~SshOutputStreamBuffer();

    // Virtual streambuf functions
    virtual std::streambuf::int_type overflow(std::streambuf::int_type value) override;
    virtual int sync() override;
    virtual pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which) override;
    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode which) override;


protected:
    std::shared_ptr<SshFileSystem>         m_fs;
    const FS::Path                         m_path;
    void*                                  m_file; // actually LIBSSH2_SFTP*
    std::vector<char> m_buffer;
};

} // namespace FS

} // namespace SimpleNodeEditor

#endif // FILESYSTEM_H