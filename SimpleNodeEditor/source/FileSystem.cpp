#include "FileSystem.hpp"

#include <fstream>
#include <system_error>
#include <chrono>

#ifdef _WIN32
#define NOMINMAX
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif
// following 2 file include must behind NOMINMAX macro
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <stdio.h>
#include <string.h>
#include <filesystem>
#include "Log.hpp"
#include "libssh2_setup.h"
#include "libssh2_sftp.h"
#include "libssh2.h"
#include "Common.hpp"
namespace SimpleNodeEditor {
namespace FS
{

// ----------------- LocalFileSystem implementation -----------------
LocalFileSystem::LocalFileSystem()
: IFileSystem(FileSystemType::Local) { }

bool LocalFileSystem::Exists(const Path& path) {
    return stdfs::exists(path.m_pathimpl);
}

bool LocalFileSystem::IsDirectory(const Path& path) {
    std::error_code ec;
    return stdfs::is_directory(path.m_pathimpl, ec) && !ec;
}

std::vector<FileEntry> LocalFileSystem::List(const Path& path){
    std::vector<FileEntry> out;
    if (!stdfs::exists(path.m_pathimpl) || !stdfs::is_directory(path.m_pathimpl)) return out;

    for (auto &p : stdfs::directory_iterator(path.m_pathimpl)) {
        FileEntry e;
        e.m_path = p.path();
        e.m_isDirectory = stdfs::is_directory(p.path());
        if (!e.m_isDirectory) {
            e.m_size = stdfs::file_size(p.path());
        }
        out.push_back(std::move(e));
    }
    return out;
}

std::unique_ptr<std::istream> LocalFileSystem::createInputStream(std::ios_base::openmode mode, const Path& path)
{
    return std::unique_ptr<std::istream>(new std::ifstream(path.String(), mode));
}

std::unique_ptr<std::ostream> LocalFileSystem::createOutputStream(std::ios_base::openmode mode, const Path& path)
{

    return std::unique_ptr<std::ostream>(new std::ofstream(path.String(), mode));
}


// ----------------- SshFileSystem implementation -----------------
SshFileSystem::SshFileSystem( const std::string & host, const std::string& port,
        const std::string & username,
        const std::string & password,
        const std::string & publicKey,
        const std::string & privateKey)
: IFileSystem(FileSystemType::Ssh)
, m_hostAddr(host)
, m_port(port)
, m_username(username)
, m_password(password)
, m_publicKey(publicKey)
, m_privateKey(privateKey)
, m_socket(LIBSSH2_INVALID_SOCKET)
, m_session(nullptr)
, m_sftpSession(nullptr)
{
    Connect();
}

SshFileSystem::SshFileSystem(const SshConnectionInfo& connectionInfo)
: SshFileSystem(connectionInfo.m_hostAddr,
                connectionInfo.m_port,
                connectionInfo.m_username, 
                connectionInfo.m_password,
                connectionInfo.m_publicKey,
                connectionInfo.m_privateKey)
{

}


SshFileSystem::~SshFileSystem()
{
    Disconnect();
}

bool SshFileSystem::IsConnected() const
{
    return m_session != nullptr && m_sftpSession != nullptr && m_socket != LIBSSH2_INVALID_SOCKET;
}

void SshFileSystem::Connect()
{
    #ifdef WIN32
        // Initialize winsock
        WSADATA wsadata;
        if(WSAStartup(MAKEWORD(2, 0), &wsadata)) {
            SNELOG_ERROR("WSAStartup failed with error");
            return;
        }
    #endif

    // Close connection if it is already open
    Disconnect();

    // Initialize libssh2
    if (libssh2_init(0))
    {
        SNELOG_ERROR("libssh init failed");
        return;
    }

    // Try to connect
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_socket == LIBSSH2_INVALID_SOCKET) {
        SNELOG_ERROR("fail to create socket");
        return;
    }
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(static_cast<u_short>(std::atoi(m_port.c_str())));
    InetPtonA(sin.sin_family, m_hostAddr.data(), &sin.sin_addr);
    if(connect(m_socket, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in))) {
        int errcode = WSAGetLastError();
        SNELOG_ERROR("failed to conect to {}, error code {}", m_hostAddr, errcode);
        #ifdef WIN32
                closesocket(m_socket);
        #else
                close(m_socket);
        #endif
    }

    // Check if connection has been successfull
    if (m_socket == -1)
    {
        SNELOG_ERROR("Invalid Socket, connect fail");
        return;
    }

    m_session = libssh2_session_init();
    int retcode = 0;
    if (m_session)
    {
        // Open session
        retcode = libssh2_session_handshake((LIBSSH2_SESSION *)m_session, m_socket);
        if (retcode == LIBSSH2_ERROR_SOCKET_NONE)       SPDLOG_ERROR("LIBSSH2_ERROR_SOCKET_NONE");
        if (retcode == LIBSSH2_ERROR_BANNER_SEND)       SPDLOG_ERROR("LIBSSH2_ERROR_BANNER_SEND");
        if (retcode == LIBSSH2_ERROR_KEX_FAILURE)       SPDLOG_ERROR("LIBSSH2_ERROR_KEX_FAILURE");
        if (retcode == LIBSSH2_ERROR_SOCKET_SEND)       SPDLOG_ERROR("LIBSSH2_ERROR_SOCKET_SEND");
        if (retcode == LIBSSH2_ERROR_SOCKET_DISCONNECT) SPDLOG_ERROR("LIBSSH2_ERROR_SOCKET_DISCONNECT");
        if (retcode == LIBSSH2_ERROR_PROTO)             SPDLOG_ERROR("LIBSSH2_ERROR_PROTO");
        if (retcode == LIBSSH2_ERROR_EAGAIN)            SPDLOG_ERROR("LIBSSH2_ERROR_EAGAIN");

        if (retcode == 0)
        {
            // Get fingerprint
            std::string fingerprint(libssh2_hostkey_hash((LIBSSH2_SESSION *)m_session, LIBSSH2_HOSTKEY_HASH_SHA1));

            // Try public key authentication first
            if (stdfs::exists(m_privateKey) && stdfs::exists(m_publicKey))
            {

                retcode = libssh2_userauth_publickey_fromfile(
                    (LIBSSH2_SESSION *)m_session, m_username.c_str(),
                    m_publicKey.c_str(),
                    m_privateKey.c_str(),
                    m_password.c_str()
                );
                if (retcode != 0) SPDLOG_ERROR("libssh2_userauth_publickey_fromfile failed, retcode[{}]", retcode);
            }
            else
            {
                SPDLOG_ERROR("not valid public/private key filepath publickeypath[{}] privatekeypath[{}]", m_publicKey, m_privateKey);
            }

            // Try username/password second
            if (retcode != 0)
            {
                retcode = libssh2_userauth_password((LIBSSH2_SESSION *)m_session, m_username.c_str(), m_password.c_str());
            }

            // Have we connected successfully?
            if (retcode == 0)
            {
                // Set SSH mode to blocking
                libssh2_session_set_blocking((LIBSSH2_SESSION *)m_session, 1); // maybe unblocking is better

                m_sftpSession = libssh2_sftp_init((LIBSSH2_SESSION *)m_session);
                if (!m_sftpSession) {
                    SNELOG_ERROR("Unable to init SFTP session");
                    Disconnect();
                }
            }
            else
            {
                SNELOG_ERROR("Cannot connect to remote, host addr [{}], port [{}]", m_hostAddr, m_port);
                Disconnect();
            }
        }
    }
    if (retcode == 0)
    {
        SNELOG_INFO("Connect Successed, hostaddr[{}]", m_hostAddr);
    }
}

void SshFileSystem::Disconnect()
{
    // Check if session is already closed
    if (!m_session) return;

    // Close SFTP session
    if (m_sftpSession)
    {
        libssh2_sftp_shutdown((LIBSSH2_SFTP *)m_sftpSession);
    }

    // Close SSH session
    libssh2_session_disconnect((LIBSSH2_SESSION *)m_session, "done");
    libssh2_session_free      ((LIBSSH2_SESSION *)m_session);

    // Close socket
    if (m_socket != 0)
    {
        #ifdef WIN32
            closesocket(m_socket);
        #else
            ::close(m_socket);
        #endif
    }

    // Reset state
    m_socket      = 0;
    m_session     = nullptr;
    m_sftpSession = nullptr;

}


bool SshFileSystem::Exists(const Path& path) 
{
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int rc = libssh2_sftp_stat(reinterpret_cast<LIBSSH2_SFTP*>(m_sftpSession), path.String().c_str(), &attrs);
    return rc == 0;
}

bool SshFileSystem::IsDirectory(const Path& path) 
{
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    if (libssh2_sftp_stat(reinterpret_cast<LIBSSH2_SFTP*>(m_sftpSession), path.String().c_str(), &attrs) != 0) return false;
    return (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) && ((attrs.permissions & LIBSSH2_SFTP_S_IFMT) == LIBSSH2_SFTP_S_IFDIR);
}

std::vector<FileEntry> SshFileSystem::List(const Path& path)
{
    std::vector<FileEntry> out;
    LIBSSH2_SFTP* sftp = reinterpret_cast<LIBSSH2_SFTP*>(m_sftpSession);
    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_opendir(sftp, path.String().c_str());
    if (!handle) 
    {
        SNELOG_ERROR("fail to opendir, path [{}]", path.m_pathimpl.string());
        char * errorMsg;
        int    len = 1024;
        libssh2_session_last_error((LIBSSH2_SESSION *)m_session, &errorMsg, &len, 0);
        if(errorMsg && len > 0) {
            SNELOG_ERROR(" opendir failed: errmsg {})", errorMsg);
        }
        return out;
    }

    char buffer[512];
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    while (true) {
        int rc = libssh2_sftp_readdir(handle, buffer, sizeof(buffer), &attrs);
        if (rc > 0) {
            std::string filename(buffer, rc);
            if (filename == "." || filename == "..") continue;
            FileEntry e;
            e.m_path = path / filename;
            e.m_isDirectory = ( (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) && ((attrs.permissions & LIBSSH2_SFTP_S_IFMT) == LIBSSH2_SFTP_S_IFDIR) );
            e.m_size = attrs.filesize;
            out.push_back(std::move(e));
        } else if (rc == 0) {
            break;
        } else {
            SNELOG_ERROR("error occured when sftp_readdir");
            CheckError();
            break;
        }
    }
    libssh2_sftp_closedir(handle);
    return out;
}

void SshFileSystem::CheckError()
{
    char* errorMsg;
    int    len = 1024;
    libssh2_session_last_error((LIBSSH2_SESSION *)m_session, &errorMsg, &len, 0);
    SNELOG_ERROR("SSH ERROR : {}", errorMsg);
}


std::unique_ptr<std::istream> SshFileSystem::createInputStream(std::ios_base::openmode mode, const Path& path)
{
    if (!IsConnected()) return nullptr;

    return std::make_unique<std::istream>(new SshInputStreamBuffer(shared_from_this(), path, mode));
}

std::unique_ptr<std::ostream> SshFileSystem::createOutputStream(std::ios_base::openmode mode, const Path& path)
{
    if (!IsConnected()) return nullptr;
    SNELOG_INFO("createOutputStream E");
    return std::make_unique<std::ostream>(new SshOutputStreamBuffer(shared_from_this(), path, mode));
}

SshInputStreamBuffer::SshInputStreamBuffer(std::shared_ptr<SshFileSystem> fs, const FS::Path & path, [[maybe_unused]]std::ios_base::openmode mode, size_t bufferSize, size_t putBackSize)
: m_fs(fs)
, m_path(path)
, m_file(nullptr)
, m_putbackSize(std::max(putBackSize, (size_t)1))
, m_buffer(std::max(bufferSize, (size_t)1))
{
    SNELOG_TRACE("SshOutputStreamCreation E");
    if (!m_fs->IsConnected()) 
    {
        SNELOG_ERROR("sshfilesystem disconnected, fail to create SshOuputStreamBuffer");
        return;
    }

    m_file = (void *)libssh2_sftp_open((LIBSSH2_SFTP *)m_fs->GetSftpSessionHandle(), m_path.String().c_str(), LIBSSH2_FXF_READ, 0);
    if (m_file)
    {
        // Initialize read buffer
        char * end = &m_buffer.front() + m_buffer.size();
        setg(end, end, end);
    }
    else
    {
        SNELOG_ERROR("ssh open failed");
        libssh2_sftp_close((LIBSSH2_SFTP_HANDLE *)m_file);
    }
}

SshInputStreamBuffer::~SshInputStreamBuffer()
{
    if (m_file)
    {
        libssh2_sftp_close((LIBSSH2_SFTP_HANDLE *)m_file);
    }
}

std::streambuf::int_type SshInputStreamBuffer::underflow() 
{
    if (!m_file)
    {
        return traits_type::eof();
    }

    // Check if the buffer is filled
    if (gptr() < egptr())
    {
        // Return next byte from buffer
        return traits_type::to_int_type(*gptr());
    }

    // Prepare buffer
    char * base  = &m_buffer.front();
    char * start = base;

    if (eback() == base)
    {
        std::memmove(base, egptr() - m_putbackSize, m_putbackSize);
        start += m_putbackSize;
    }

    // Refill buffer
    size_t size = m_buffer.size() - (start - base);
    ssize_t n = libssh2_sftp_read((LIBSSH2_SFTP_HANDLE *)m_file, start, size);

    if (n == 0)
    {
        return traits_type::eof();
    }

    // Set buffer pointers
    setg(base, start, start + n);

    // Return next byte
    return traits_type::to_int_type(*gptr());
}

SshInputStreamBuffer::pos_type SshInputStreamBuffer::seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which)
{
    if (way == std::ios_base::beg)
    {
        return seekpos((pos_type)off, which);
    }

    else if (way == std::ios_base::end)
    {
        LIBSSH2_SFTP_ATTRIBUTES attrs;

        if (libssh2_sftp_fstat_ex((LIBSSH2_SFTP_HANDLE *)m_file, &attrs, 0) == 0)
        {
            pos_type pos = (pos_type)attrs.filesize + off;
            return seekpos(pos, which);
        }
    }

    else if (way == std::ios_base::cur)
    {
        pos_type pos = (pos_type)libssh2_sftp_tell64((LIBSSH2_SFTP_HANDLE *)m_file);
        pos += off - (egptr() - gptr());

        return seekpos(pos, which);
    }

    return (pos_type)(off_type)(-1);
}

SshInputStreamBuffer::pos_type SshInputStreamBuffer::seekpos(pos_type pos, std::ios_base::openmode)
{
    // Check file handle
    if (!m_file)
    {
        return (pos_type)(off_type)(-1);
    }

    // Set file position
    libssh2_sftp_seek64((LIBSSH2_SFTP_HANDLE *)m_file, (libssh2_uint64_t)pos);

    // Reset read buffer
    char * end = &m_buffer.front() + m_buffer.size();
    setg(end, end, end);

    // Return new position
    return (pos_type)(off_type)(pos);
}

SshOutputStreamBuffer::SshOutputStreamBuffer(std::shared_ptr<SshFileSystem> fs, const FS::Path& path, std::ios_base::openmode mode, size_t bufferSize)
: m_fs(fs)
, m_path(path)
, m_file(nullptr)
, m_buffer(std::max(bufferSize, (size_t)1))
{
    if (!m_fs->IsConnected()) 
    {
        SNELOG_ERROR("sshfilesystem disconnected, fail to create SshOuputStreamBuffer");
        return;
    }

    // Set opening flags
    unsigned long flags = LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT;
    if (mode & std::ios::app)   flags |= LIBSSH2_FXF_APPEND;
    if (mode & std::ios::trunc) flags |= LIBSSH2_FXF_TRUNC;

    m_file = (void *)libssh2_sftp_open((LIBSSH2_SFTP *)m_fs->GetSftpSessionHandle(), m_path.String().c_str(), flags, 0);

    if (m_file)
    {
        // Initialize write buffer
        char * start = &m_buffer.front();
        char * end   = &m_buffer.front() + m_buffer.size();
        setp(start, end);
    }
    else 
    {
        SNELOG_ERROR("ssh open failed");
        libssh2_sftp_close((LIBSSH2_SFTP_HANDLE *)m_file);
    }
    SNELOG_INFO("SshOutputStreamCreation x");
}

SshOutputStreamBuffer::~SshOutputStreamBuffer()
{
    // Close file
    if (m_file)
    {
        pubsync();
        sync();
        libssh2_sftp_fsync((LIBSSH2_SFTP_HANDLE *)m_file);
        libssh2_sftp_close((LIBSSH2_SFTP_HANDLE *)m_file);
    }
}

// Virtual streambuf functions
std::streambuf::int_type SshOutputStreamBuffer::overflow(std::streambuf::int_type value)
{
    SNELOG_INFO("outputbuffer stream start to overflow E");
    if (!m_file)
    {
        return traits_type::eof();
    }

    // Sync buffer
    if (sync() != 0 || value == traits_type::eof())
    {
        return traits_type::eof();
    }

    char * start = &m_buffer.front();
    char * end   = &m_buffer.front() + m_buffer.size();
    *start = traits_type::to_char_type(value);

    // Update buffer position
    setp(start + 1, end);

    SNELOG_INFO("outputbuffer stream start to overflow X");
    return traits_type::to_int_type(traits_type::to_char_type(value));;

}

int SshOutputStreamBuffer::sync()
{
    // Get size of data in the buffer
    size_t size = pptr() - &m_buffer.front();
    if (size > 0)
    {
        auto res = libssh2_sftp_write((LIBSSH2_SFTP_HANDLE *)m_file, &m_buffer.front(), size);

        switch (res)
        {
            case LIBSSH2_ERROR_ALLOC:
                SNELOG_ERROR("SSH error: LIBSSH2_ERROR_ALLOC");
                break;

            case LIBSSH2_ERROR_SOCKET_SEND:
                SNELOG_ERROR("SSH error: LIBSSH2_ERROR_SOCKET_SEND");
                break;

            case LIBSSH2_ERROR_SOCKET_TIMEOUT:
                SNELOG_ERROR("SSH error: LIBSSH2_ERROR_SOCKET_TIMEOUT");
                break;

            case LIBSSH2_ERROR_SFTP_PROTOCOL:
                SNELOG_ERROR("SSH error: LIBSSH2_ERROR_SFTP_PROTOCOL");
                break;

            default:
                break;
        }

        // Reset write buffer
        char * start = &m_buffer.front();
        char * end   = &m_buffer.front() + m_buffer.size();
        setp(start, end);
    }
    return 0;

}

SshOutputStreamBuffer::pos_type SshOutputStreamBuffer::seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which)
{
    // Sync output first
    sync();

    if (way == std::ios_base::beg)
    {
        return seekpos((pos_type)off, which);
    }
    else if (way == std::ios_base::end)
    {
        LIBSSH2_SFTP_ATTRIBUTES attrs;

        if (libssh2_sftp_fstat_ex((LIBSSH2_SFTP_HANDLE *)m_file, &attrs, 0) == 0)
        {
            pos_type pos = (pos_type)attrs.filesize + off;
            return seekpos(pos, which);
        }
    }
    else if (way == std::ios_base::cur)
    {
        pos_type pos = (pos_type)libssh2_sftp_tell64((LIBSSH2_SFTP_HANDLE *)m_file);
        pos += off;

        return seekpos(pos, which);
    }

    return (pos_type)(off_type)(-1);
}

SshOutputStreamBuffer::pos_type SshOutputStreamBuffer::seekpos(pos_type pos, [[maybe_unused]] std::ios_base::openmode which)
{
    // Sync output first
    sync();

    // Check file handle
    if (!m_file)
    {
        return (pos_type)(off_type)(-1);
    }

    // Set file position
    libssh2_sftp_seek64((LIBSSH2_SFTP_HANDLE *)m_file, (libssh2_uint64_t)pos);

    // Reset write buffer
    char * start = &m_buffer.front();
    char * end   = &m_buffer.front() + m_buffer.size();
    setp(start, end);

    // Return new position
    return (pos_type)(off_type)(pos);
}

} // namespace FS
} // namespace SimpleNodeEditor
