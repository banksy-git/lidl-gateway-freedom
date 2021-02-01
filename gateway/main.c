/*
    Serial port gateway for Silvercrest (Lidl) Smart Home Gateway
  =================================================================
  Author: Paul Banks [https://paulbanks.org/]

*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>

#define SERIAL_PORT "/dev/ttyS1"
#define BUF_SIZE 1024

static fd_set _master_read_set;
static int _serial_fd;
static int _connection_fd = -1;
static int _buf[BUF_SIZE];

#ifdef ENABLE_DEBUGLOG
#define LOG_DEBUG(format, ...) fprintf(stderr, format "\n", __VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif

static void _error_exit(const char* msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static int _open_serial_port()
{
    int fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd<0) {
        _error_exit("Could not open serial port " SERIAL_PORT);
    }
    fcntl(fd, F_SETFL, 0);

    struct termios options;
    if (tcgetattr (fd, &options) != 0) {
        _error_exit("Could not get TTY attribs");
    }

    cfsetispeed (&options, B115200);
    cfsetospeed (&options, B115200);
    options.c_cflag |= (CLOCAL | CREAD);

    // 8N1, hardware flow control
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag |= CRTSCTS;

    // Raw input and output
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // No input or output processing
    options.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP |
                         INLCR | IGNCR | ICRNL | IXON | IXOFF | IUCLC | IXANY |
                         IMAXBEL);

    options.c_oflag &= ~OPOST;

    options.c_cc[VMIN] = 1;

    if (tcsetattr (fd, TCSANOW, &options) != 0) {
        _error_exit("Could not set TTY attribs");
    }

    return fd;
}

static void  _close_connectionfd()
{
    if (_connection_fd >= 0) {
        fprintf(stderr, "Closing existing connection\n");
        shutdown(_connection_fd, SHUT_RDWR);
        close(_connection_fd);
        FD_CLR(_connection_fd, &_master_read_set);
        _connection_fd = -1;
    }
}

int main()
{

    _serial_fd = _open_serial_port();

    // Create listening socket
    int listen_sock;
    struct sockaddr_in name;
    listen_sock = socket (PF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        _error_exit("socket");
    }

    int enable = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,
                   &enable, sizeof(int)) < 0) {
        _error_exit("setsockopt(SO_REUSEADDR) failed");
    }

    name.sin_family = AF_INET;
    name.sin_port = htons (8888);
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (listen_sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
        _error_exit("bind");
    }

    if (listen (listen_sock, 1) < 0) {
        _error_exit("listen");
    }

    FD_ZERO (&_master_read_set);
    FD_SET (listen_sock, &_master_read_set);
    FD_SET (_serial_fd, &_master_read_set);
    int i;
    for(;;)
    {
        fd_set read_fd_set = _master_read_set;
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            _error_exit("select");
        }

        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &read_fd_set)) {
                if (i==listen_sock) {
                    struct sockaddr_in clientname;
                    socklen_t size = sizeof (clientname);
                    int new = accept (listen_sock,
                                      (struct sockaddr *) &clientname, &size);
                    if (new < 0) {
                        continue;
                    }
                    _close_connectionfd();
                    fprintf (stderr,
                         "Connect from host %s\n",
                         inet_ntoa (clientname.sin_addr));
                    FD_SET (new, &_master_read_set);
                    _connection_fd = new;

                } else if (i==_serial_fd) {
                    ssize_t len = read(_serial_fd, _buf, BUF_SIZE);
                    if (len<=0) {
                        _error_exit("read serial");
                    }
                    LOG_DEBUG("SERIAL_READ: %d bytes", len);
                    if (_connection_fd >= 0) {
                        if (write(_connection_fd, _buf, len) < 0) {
                            _close_connectionfd();
                        }
                    }

                } else if (i==_connection_fd) {
                    ssize_t len = read(_connection_fd, _buf, BUF_SIZE);
                    if (len<=0) {
                        _close_connectionfd();
                    } else {
                        LOG_DEBUG("   TCP_READ: %d bytes", len);
                        if (write(_serial_fd, _buf, len) < 0) {
                            _error_exit("write serial");
                        }
                    }
                }

            }
        }

    }

}
