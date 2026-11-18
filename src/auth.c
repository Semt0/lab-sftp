/**
 * @file auth.c
 * @author Yuhan Zhou (zhouyuhan@pku.edu.cn)
 * @brief SSH authentication layer functionalities.
 * @version 0.1
 * @date 2022-10-05
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "libsftp/auth.h"

#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "libsftp/buffer.h"
#include "libsftp/error.h"
#include "libsftp/libssh.h"
#include "libsftp/logger.h"
#include "libsftp/packet.h"
#include "libsftp/session.h"

/**
 * @brief Request server for user authentication service.
 *
 * @param session
 * @return SSH_OK on success, SSH_ERR on error.
 */
int ssh_request_auth(ssh_session session) {
    int rc;
    uint8_t type;
    char *service;

    rc = ssh_buffer_pack(session->out_buffer, "bs", SSH_MSG_SERVICE_REQUEST,
                         "ssh-userauth");
    rc |= ssh_packet_send(session);
    if (rc != SSH_OK) return rc;

    rc = ssh_packet_receive(session);
    if (rc != SSH_OK) return rc;

    rc = ssh_buffer_unpack(session->in_buffer, "bs", &type, &service);
    if (rc != SSH_OK || type != SSH_MSG_SERVICE_ACCEPT ||
        strcmp(service, "ssh-userauth") != 0) {
        SAFE_FREE(service);
        return SSH_ERROR;
    }

    SAFE_FREE(service);
    return SSH_OK;
}

/**
 * @brief Get password from terminal.
 *
 * @param password
 */
void ssh_get_password(char *password) {
    static struct termios oldt, newt;
    int max_len = 100;
    int i = 0;
    uint8_t c;

    fprintf(stdout, "password: ");
    fflush(stdout);

    /*saving the old settings of STDIN_FILENO and copy settings for resetting*/
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    /*setting the approriate bit in the termios struct*/
    newt.c_lflag &= ~(ECHO);

    /*setting the new bits*/
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    /*reading the password from the console*/
    while ((c = getchar()) != '\n' && c != EOF && i < max_len) {
        password[i++] = c;
    }
    password[i] = '\0';

    /*resetting our old STDIN_FILENO*/
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

/**
 * @brief Send password authentication requests and wait for response.
 * Can only try up to 3 times on wrong password.
 *
 * @param session
 * @param password
 * @return SSH_OK on success, SSH_ERROR on error, SSH_AGAIN on wrong password
 */
int ssh_userauth_password(ssh_session session, const char *password) {
    int rc;
    uint8_t type;
    static int cnt = 0;

    rc = ssh_buffer_pack(session->out_buffer, "bsssbs",
                         SSH_MSG_USERAUTH_REQUEST, session->opts.username,
                         "ssh-connection", "password", 0, password);
    if (rc != SSH_OK) goto error;

    rc = ssh_packet_send(session);
    if (rc != SSH_OK) goto error;

    /**
     * RFC 4252 5.4
     * The SSH server may send an SSH_MSG_USERAUTH_BANNER message at any
     * time after this authentication protocol starts and before
     * authentication is successful.  This message contains text to be
     * displayed to the client user before authentication is attempted.
     *
     */

    while (rc != SSH_ERROR) {
        rc = ssh_packet_receive(session);
        if (rc != SSH_OK) goto error;
        ssh_buffer_get_u8(session->in_buffer, &type);
        switch (type) {
            case SSH_MSG_USERAUTH_BANNER:
                {
                    char *message = NULL;
                    char *language = NULL;
                    rc = ssh_buffer_unpack(session->in_buffer, "ss",
                                           &message, &language);
                    if (rc != SSH_OK) {
                        ssh_set_error(SSH_FATAL, "invalid authentication banner\n");
                        goto error;
                    }

                    for (unsigned char *ch = (unsigned char *)message;
                         *ch != '\0'; ++ch) {
                        if ((*ch < 0x20 && *ch != '\n' && *ch != '\t') ||
                            *ch == 0x7f) {
                            *ch = '?';
                        }
                    }
                    fprintf(stdout, "%s\n", message);
                    SAFE_FREE(message);
                    SAFE_FREE(language);
                    continue;
                }

            case SSH_MSG_USERAUTH_SUCCESS:
                if (ssh_buffer_get_len(session->in_buffer) != 0) {
                    ssh_set_error(SSH_FATAL, "invalid authentication success packet\n");
                    goto error;
                }
                cnt = 0;
                LOG_NOTICE("password authentication succeeded");
                return SSH_OK;

            case SSH_MSG_USERAUTH_PASSWD_CHANGEREQ:
            case SSH_MSG_USERAUTH_FAILURE:
                {
                    if (type == SSH_MSG_USERAUTH_PASSWD_CHANGEREQ) {
                        ssh_set_error(SSH_REQUEST_DENIED,
                                      "server requires a password change; unsupported\n");
                        goto error;
                    }

                    char *methods = NULL;
                    uint8_t partial_success;
                    rc = ssh_buffer_unpack(session->in_buffer, "sb",
                                           &methods, &partial_success);
                    if (rc != SSH_OK) {
                        ssh_set_error(SSH_FATAL, "invalid authentication failure packet\n");
                        goto error;
                    }

                    bool can_retry_password = false;
                    const char *method = methods;
                    while (*method != '\0') {
                        size_t len = strcspn(method, ",");
                        if (len == sizeof("password") - 1 &&
                            memcmp(method, "password", len) == 0) {
                            can_retry_password = true;
                            break;
                        }
                        method += len;
                        if (*method == '\0') break;
                        ++method;
                    }
                    SAFE_FREE(methods);

                    if (partial_success) {
                        ssh_set_error(SSH_REQUEST_DENIED,
                                      "server requires another authentication step; unsupported\n");
                        goto error;
                    }
                    if (!can_retry_password) {
                        ssh_set_error(SSH_REQUEST_DENIED,
                                      "server does not allow another password attempt\n");
                        goto error;
                    }
                    if (++cnt >= 3) {
                        ssh_set_error(SSH_REQUEST_DENIED,
                                      "password authentication failed after three attempts\n");
                        goto error;
                    }

                    ssh_set_error(SSH_REQUEST_DENIED, "password rejected; try again\n");
                    return SSH_AGAIN;
                }

            default:
                if (type == SSH_MSG_IGNORE || type == SSH_MSG_DEBUG) {
                    continue;
                }
                if (type == SSH_MSG_DISCONNECT) {
                    ssh_set_error(SSH_REQUEST_DENIED,
                                  "server disconnected during authentication\n");
                    goto error;
                }
                ssh_set_error(SSH_FATAL, "unexpected authentication message: %u\n",
                              (unsigned int)type);
                goto error;

        }
    }

error:
    ssh_buffer_reinit(session->out_buffer);
    return SSH_ERROR;
}