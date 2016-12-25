#include <string>
#include <atomic>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * This include needed to be updated for my Pi by installing the libi2c-dev package with
 * apt-get.  The header w/ the same name that existed before didn't have the smbus
 * functions defined.
 */
#include <linux/i2c-dev.h>

#include "i2con.hpp"

std::atomic_bool    quit(false);

void handle_sigint(int sig) {
    quit = true;
}

void handle_sigchld(int sig) {
    int saved_errno = errno;

    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {
    }

    errno = saved_errno;
}
/*
 * Setup signal handlers for server.
 *
 * The server just runs forever; waiting for connections.  The user can stop the server
 * with CTRL-C.  This handles the SIGINT by closing the socket (to get the "accept" to
 * error out) and setting quit to true to get main to exit.
 */
int i2con_server_sig_init() {
    struct  sigaction   sa;
    int     err;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigfillset(&sa.sa_mask);
    err = sigaction(SIGINT, &sa, nullptr);

    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handle_sigchld;
    err |= sigaction(SIGCHLD, &sa, nullptr);

    if (err) {
        fprintf(stderr, "i2con_server: failed to install signal handler(s).\n");
    }

    return err;
}

/*
 * Setup server socket to accept connections from client.
 *
 * @return int: Returns server socket or something < 0 if error.
 */
int i2con_server_socket_init() {
    struct addrinfo     hints;
    struct addrinfo     *server_info;
    int                 err;
    int                 socket_fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(nullptr, I2CON_PORT, &hints, &server_info);
    if (err) {
        perror("i2con_server (getaddrinfo)");
        return err;
    }


    for (struct addrinfo *pai = server_info; pai != nullptr; pai = pai->ai_next) {
        char    ipstr[INET6_ADDRSTRLEN];
        int     yes = 1;


        socket_fd = socket(pai->ai_family, pai->ai_socktype, pai->ai_protocol);
        if (socket_fd == -1) {
            perror("i2con_server (socket)");
            continue ;
        }

        err = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (err) {
            perror("i2con_server (setsockopt)");
            close(socket_fd);
            continue ;
        }

        err = bind(socket_fd, pai->ai_addr, pai->ai_addrlen);
        if (err) {
            perror("i2con_server (bind)");
            close(socket_fd);
            continue ;
        }

        break ;
    }

    freeaddrinfo(server_info);

    if (err) {
        socket_fd = err;
    }

    return socket_fd;
}

/*
 * Wrapper for system send function.
 */
int i2con_send(int sock_fd, void *p, size_t len) {
    ssize_t xfer;
    uint8_t *buf = (uint8_t*)p;

    do {
        xfer = send(sock_fd, buf, len, 0);
        buf += xfer;
        len -= xfer;
    } while (xfer > -1 && len > 0);

    return (len == 0) ? 0 : -1;
}

/*
 * Wrapper for system recv function.
 */
int i2con_recv(int sock_fd, void *p, size_t len) {
    uint8_t *buf = (uint8_t*)p;
    ssize_t xfer;

    do {
        /*
         * The send/recv functions will return 0 if the connection is closed by the remote
         * side.  Originally I made the mistake that an unexpected close would produce a
         * negative return...this is not the case.
         */
        xfer = recv(sock_fd, buf, len, 0);
        buf += xfer;
        len -= xfer;
    } while (xfer > 0 && len > 0);

    return (len == 0) ? 0 : -1;
}

void i2con_server_process(int client_socket) {
    i2cmd_t     i2cmd;
    int         i2c_fd;
    bool        connected = false;
    ssize_t     err;
    ssize_t     res;
    char        filename[100];

    // Wait for first i2c request...
    err = i2con_recv(client_socket, &i2cmd, sizeof(i2cmd));
    if (err || i2cmd.cmd == I2CON_CMD_DISCONN) {
        return ;
    }

    /*
     * Once the first i2c request is received, open a file to the i2c device for the bus
     * specified in the request.  This bus is used for the duration of the connection.
     */
    sprintf(filename, "/dev/i2c-%d", i2cmd.bus);
    i2c_fd = open(filename, O_RDWR);
    if (i2c_fd < 0) {
        /*
         * The i2c device at the requested bus couldn't be opened (probably because
         * there's no i2c device at the specified bus.  Return an error to the client and
         * close connection.
         */
        i2cmd.sts = I2CON_STS_BADBUS;
        i2con_send(client_socket, &i2cmd, sizeof(i2cmd));

        return ;
    }

    /*
     * Keep looping until an error occurs or client disconnects (either explicitly with a
     * DISCONN comman or the socket closes.  Note that 'err' is initially 0 from above code.
     */
    while (!quit && !err) {
        /*
         * Select device on i2c bus.  If this fails, the server just waits for the next
         * command.
         */
        err = ioctl(i2c_fd, I2C_SLAVE, i2cmd.adr);
        if (err) {
            i2cmd.sts = I2CON_STS_BADADR;
            i2con_send(client_socket, &i2cmd, sizeof(i2cmd));
        }
        else {
            switch (i2cmd.cmd) {
                case I2CON_CMD_RD8:
                    res = i2c_smbus_read_byte_data(i2c_fd, i2cmd.reg);
                    i2cmd.sts = (res >= 0) ? I2CON_STS_OK : I2CON_STS_IOERR;
                    i2cmd.b = (uint8_t)res;
                    err = i2con_send(client_socket, &i2cmd, sizeof(i2cmd));
                    break ;

                case I2CON_CMD_WR8:
                    res = i2c_smbus_write_byte_data(i2c_fd, i2cmd.reg, i2cmd.b);
                    i2cmd.sts = (res >= 0) ? I2CON_STS_OK : I2CON_STS_IOERR;
                    err = i2con_send(client_socket, &i2cmd, sizeof(i2cmd));
                    break ;

                case I2CON_CMD_RD16:
                    res = i2c_smbus_read_word_data(i2c_fd, i2cmd.reg);
                    i2cmd.sts = (res >= 0) ? I2CON_STS_OK : I2CON_STS_IOERR;
                    i2cmd.w = ntohs((uint16_t)res);
                    i2con_send(client_socket, &i2cmd, sizeof(i2cmd));
                    break ;

                case I2CON_CMD_WR16:
                    res = i2c_smbus_write_word_data(i2c_fd, i2cmd.reg, ntohs(i2cmd.w));
                    i2cmd.sts = (res >= 0) ? I2CON_STS_OK : I2CON_STS_IOERR;
                    i2con_send(client_socket, &i2cmd, sizeof(i2cmd));
                    break ;

                default:
                    err = -1;
                    break ;
            }
        }

        // Wait for the next request.
        err = i2con_recv(client_socket, &i2cmd, sizeof(i2cmd));
        if (i2cmd.cmd == I2CON_CMD_DISCONN) {
            // Client is done.
            break ;
        }
    }

    close(i2c_fd);
    close(client_socket);
}


int main() {
    int err;
    int socket_fd;
    int i2c_fd;

    err = i2con_server_sig_init();
    if (err) {
        return -1;
    }

    socket_fd = i2con_server_socket_init();
    if (socket_fd < 0) {
        return -2;
    }

    if (listen(socket_fd, 10) == -1) {
        perror("i2con_server (listent)");
        return -3;
    }

    printf("i2con_server: Listening for i2c requests (press CTRL-C to stop server)...\n");
    while (quit == false) {
        struct sockaddr_storage client_addr;
        int                     client_socket_fd;
        char                    client_str[INET6_ADDRSTRLEN];
        socklen_t               sin_size = sizeof(client_addr);
        pid_t                   pid;

        printf("i2con_server: Waiting for connection...\n");
        client_socket_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &sin_size);
        if (client_socket_fd < 0) {
            perror("OMG!!!\n");
        }

        /*
         * Get the string version of the client address.  This is just for informational
         * purposes.
         */
        if (client_addr.ss_family == AF_INET) {
            struct sockaddr_in  *sa = (struct sockaddr_in*)&client_addr;
            inet_ntop(client_addr.ss_family, &sa->sin_addr, client_str, INET6_ADDRSTRLEN);
        }
        else {
            struct sockaddr_in6  *sa = (struct sockaddr_in6*)&client_addr;
            inet_ntop(client_addr.ss_family, &sa->sin6_addr, client_str, INET6_ADDRSTRLEN);
        }

        pid = fork();
        if (pid == 0) {
            /*
             * Child process
             */
            close(socket_fd);
            printf("Connected to %s\n", client_str);
            i2con_server_process(client_socket_fd);
            printf("Disconnected from %s\n", client_str);

            return 0;
        }
        else if (pid < 0){
            /*
             * The fork call failed.  Close the client socket since no process was created
             * to service requests.
             */
            close(client_socket_fd);
        }
        else {
            /*
             * Still in parent process; close client socket (not used by parent).
             */
            close(client_socket_fd);
        }
    }

    close(socket_fd);
}

