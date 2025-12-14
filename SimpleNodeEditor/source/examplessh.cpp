/* Copyright (C) The libssh2 project and its contributors. *
 * Sample doing an SFTP directory listing.
 *
 * The sample code has default values for host name, user name, password and
 * path, but you can specify them on the command line like:
 *
 * $ ./sftpdir 192.168.0.1 user password /tmp/secretdir
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libssh2_setup.h"
#include <libssh2.h>
#include <libssh2_sftp.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <string.h>
#include "Log.hpp"
#include <filesystem>

#if defined(_MSC_VER)
#define LIBSSH2_FILESIZE_MASK "I64u"
#else
#define LIBSSH2_FILESIZE_MASK "llu"
#endif

static const char *pubkey = "C:\\Users\\19269\\.ssh\\id_rsa.pub";
static const char *privkey = "C:\\Users\\19269\\.ssh\\id_rsa";
static const char *username = "root";
static const char *password = "";
// static const char *username = nullptr;
// static const char *password = nullptr;
static const char *sftppath = "/tmp";
// static const std::string s_hostAdd{"169.254.202.122"};
static const std::string s_hostAdd{"172.25.48.190"};

static void kbd_callback(const char *name, int name_len,
                         const char *instruction, int instruction_len,
                         int num_prompts,
                         const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                         LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                         void **abstract)
{
    (void)name;
    (void)name_len;
    (void)instruction;
    (void)instruction_len;
    if(num_prompts == 1) {
        responses[0].text = strdup(password);
        responses[0].length = (unsigned int)strlen(password);
    }
    (void)prompts;
    (void)abstract;
} /* kbd_callback */

/* libssh2 debug callback to print protocol/debug messages to stderr */
static void debug_cb(LIBSSH2_SESSION *sess, int always_display,
                     const char *message, int message_len,
                     const char *language, int language_len, void **abstract)
{
    (void)always_display; (void)language; (void)language_len; (void)abstract;
    fprintf(stderr, "libssh2-debug: %.*s\n", message_len, message);
}

int test_main(int argc, char *argv[])
{
    uint32_t hostaddr;
    libssh2_socket_t sock;
    int i, auth_pw = 0;
    struct sockaddr_in sin;
    const char *fingerprint;
    char *userauthlist;
    int rc;
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_SFTP *sftp_session;
    LIBSSH2_SFTP_HANDLE *sftp_handle;

#ifdef _WIN32
    WSADATA wsadata;

    rc = WSAStartup(MAKEWORD(2, 0), &wsadata);
    if(rc) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", rc);
        return 1;
    }
#endif


    username = "root";

    // hostaddr = htonl(0x7F000001);
    hostaddr = inet_addr(s_hostAdd.data());
    SNELOG_INFO("usert input addr[{}]", s_hostAdd);

    rc = libssh2_init(0);
    if(rc) {
        fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
        return 1;
    }

    /*
     * The application code is responsible for creating the socket
     * and establishing the connection
     */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == LIBSSH2_INVALID_SOCKET) {
        fprintf(stderr, "failed to create socket.\n");
        // goto shutdown;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = hostaddr;
    if(connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in))) {
        int errcode = WSAGetLastError();
        SNELOG_ERROR("failed to conect to {}, error code {}", hostaddr, errcode);
        // goto shutdown;
    }

    /* Create a session instance */
    session = libssh2_session_init();
    if(!session) {
        SNELOG_ERROR("Could not initialize SSH session.");
        // goto shutdown;
    }

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    rc = libssh2_session_handshake(session, sock);
    if(rc) {
        SNELOG_ERROR("Failure establishing SSH session: %d\n", rc);
        // goto shutdown;
    }

    /* Enable libssh2 debug callback and trace to see auth/signature details */
    // libssh2_session_callback_set(session, LIBSSH2_CALLBACK_DEBUG, (void*)debug_cb);
    // libssh2_trace(session, LIBSSH2_TRACE_CONN | LIBSSH2_TRACE_KEX | LIBSSH2_TRACE_AUTH);

    // /* Prefer rsa-sha2-512 for RSA key signing if server supports it */
    // libssh2_session_method_pref(session, LIBSSH2_METHOD_SIGN_ALGO,
    //                            "rsa-sha2-256,ssh-rsa");
    // const char *prefs = libssh2_session_methods(session, LIBSSH2_METHOD_SIGN_ALGO);
    // if(prefs) {
    //     SNELOG_INFO("Sign algorithm preferences: {}", prefs);
    // }

    /* At this point we have not yet authenticated.  The first thing to do
     * is check the hostkey's fingerprint against our known hosts Your app
     * may have it hard coded, may go to a file, may present it to the
     * user, that's your call
     */

    /* check what authentication methods are available */
    // userauthlist = libssh2_userauth_list(session, username,
    //                                      (unsigned int)strlen(username));

    if (!std::filesystem::exists(privkey))
    {
        SPDLOG_ERROR("not valid filepath {}", privkey);
    }
    if (!std::filesystem::exists(pubkey))
    {
        SPDLOG_ERROR("not valid filepath {}", pubkey);
    }
    int ret = libssh2_userauth_publickey_fromfile(session, username,
                                            pubkey, privkey,
                                            password);
    if(ret) {
        char *err_msg = NULL;
        int err_msg_len = 0;
        /* Ask libssh2 for a more detailed error message */
        libssh2_session_last_error(session, &err_msg, &err_msg_len, 0);
        if(err_msg && err_msg_len > 0) {
            SNELOG_ERROR("Authentication by public key failed: {} (errcode {}) username[{}], strlen(username)={}", err_msg, ret, username, strlen(username));
        }
        else {
            SNELOG_ERROR("Authentication by public key failed (no detailed message available).");
        }
        goto shutdown;
    }
    else {
        SNELOG_INFO("Authentication by public key succeeded.");
    }

    sftp_session = libssh2_sftp_init(session);

    if(!sftp_session) {
        SNELOG_ERROR("Unable to init SFTP session");
        goto shutdown;
    }

    /* Since we have not set non-blocking, tell libssh2 we are blocking */
    libssh2_session_set_blocking(session, 1);

    fprintf(stderr, "libssh2_sftp_opendir().\n");
    /* Request a dir listing via SFTP */
    sftp_handle = libssh2_sftp_opendir(sftp_session, sftppath);
    if(!sftp_handle) {
        fprintf(stderr, "Unable to open dir with SFTP\n");
        goto shutdown;
    }

    fprintf(stderr, "libssh2_sftp_opendir() is done, now receive listing.\n");
    do {
        char mem[512];
        char longentry[512];
        LIBSSH2_SFTP_ATTRIBUTES attrs;

        /* loop until we fail */
        rc = libssh2_sftp_readdir_ex(sftp_handle, mem, sizeof(mem),
                                     longentry, sizeof(longentry), &attrs);
        if(rc > 0) {
            /* rc is the length of the file name in the mem
               buffer */

            if(longentry[0] != '\0') {
                printf("%s\n", longentry);
            }
            else {
                if(attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
                    /* this should check what permissions it
                       is and print the output accordingly */
                    printf("--fix----- ");
                }
                else {
                    printf("---------- ");
                }

                if(attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) {
                    printf("%4d %4d ", (int) attrs.uid, (int) attrs.gid);
                }
                else {
                    printf("   -    - ");
                }

                if(attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) {
                    printf("%8" LIBSSH2_FILESIZE_MASK " ", attrs.filesize);
                }

                printf("%s\n", mem);
            }
        }
        else {
            break;
        }

    } while(1);

    libssh2_sftp_closedir(sftp_handle);
    libssh2_sftp_shutdown(sftp_session);

shutdown:

    if(session) {
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
    }

    if(sock != LIBSSH2_INVALID_SOCKET) {
        shutdown(sock, 2);
        LIBSSH2_SOCKET_CLOSE(sock);
    }

    fprintf(stderr, "all done\n");

    libssh2_exit();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
