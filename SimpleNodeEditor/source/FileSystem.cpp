#include "FileSystem.hpp"

#include <fstream>
#include <system_error>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

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
                if (!retcode) SPDLOG_ERROR("libssh2_userauth_publickey_fromfile failed, retcode[{}]", retcode);
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
    SNELOG_INFO("sshfilesystem List E");
    std::vector<FileEntry> out;
    LIBSSH2_SFTP* sftp = reinterpret_cast<LIBSSH2_SFTP*>(m_sftpSession);
    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_opendir(sftp, path.String().c_str());
    if (!handle) 
    {
        SNELOG_ERROR("fail to opendir, path [{}]", path.m_path.string());
        char * errorMsg;
        int    len = 1024;
        libssh2_session_last_error((LIBSSH2_SESSION *)m_session, &errorMsg, &len, 0);
        if(errorMsg && len > 0) {
            SNELOG_ERROR(" opendir failed: errmsg {})", errorMsg);
        }
        return out;
    }

    // TODO: Uniform with std::filesystem::path
    char buffer[512];
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    while (true) {
        int rc = libssh2_sftp_readdir(handle, buffer, sizeof(buffer), &attrs);
        if (rc > 0) {
            std::string name(buffer, rc);
            if (name == "." || name == "..") continue;
            FileEntry e;
            e.name = name;
            std::string fp = path.m_path.string();
            if (!fp.empty() && fp.back() != '/') fp.push_back('/');
            e.fullPath = fp + name;
            e.isDirectory = ( (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) && ((attrs.permissions & LIBSSH2_SFTP_S_IFMT) == LIBSSH2_SFTP_S_IFDIR) );
            e.size = attrs.filesize;
            out.push_back(std::move(e));
        } else if (rc == 0) {
            break; // end
        } else {
            SNE_ASSERT(false, "error occured when sftp_readdir");
            SNELOG_INFO("error occured when sftp_readdir");
            break; // error
        }
    }
    libssh2_sftp_closedir(handle);
    SNELOG_INFO("sshfilesystem List X");
    return out;
}

std::string SshFileSystem::GetName(const Path& path) 
{
    return path.m_path.filename().string();
}

std::string SshFileSystem::GetParent(const Path& path) 
{
    return path.m_path.parent_path().string();
}

} // namespace FS
} // namespace SimpleNodeEditor
