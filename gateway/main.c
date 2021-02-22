/*
    Serial port gateway for Silvercrest (Lidl) Smart Home Gateway
  =================================================================
  Author: Paul Banks [https://paulbanks.org/]

*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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

static void _set_status_led(bool is_on)
{
    int fd = open("/proc/led1", O_WRONLY);
    if (fd < 0) {
        return;
    }
    write(fd, (is_on) ? "1\n" : "0\n", 2);
    close(fd);
}

static void _error_exit(const char* msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static int _open_serial_port(bool is_hw_flow_control)
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
    if (is_hw_flow_control) {
        options.c_cflag |= CRTSCTS;
    }

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
        _set_status_led(0);
        fprintf(stderr, "Closing existing connection\n");
        shutdown(_connection_fd, SHUT_RDWR);
        close(_connection_fd);
        FD_CLR(_connection_fd, &_master_read_set);
        _connection_fd = -1;
    }
}

int main(int argc, char** argv)
{

    bool is_hardware_flow_control = true;
    uint16_t port = 8888;
    opterr = 0;

    int c;
    while ((c = getopt (argc, argv, "fp:")) != -1) {
        switch(c) {
            case 'f':
                is_hardware_flow_control = false;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case '?':
            default:
                _error_exit("Unknown args");
        }
    }

    fprintf(stderr, "serialgateway: port %d, flow=%s\n",
            port, (is_hardware_flow_control)?"HW":"sw");

    _serial_fd = _open_serial_port(is_hardware_flow_control);

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
    name.sin_port = htons (port);
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
                    _set_status_led(1);
                    fprintf (stderr,
                         "Connect from host %s\n",
                         inet_ntoa (clientname.sin_addr));
                    int enable = 1;
                    if (setsockopt(new, SOL_SOCKET, SO_KEEPALIVE,
                                   &enable, sizeof(enable)) < 0) {
                        fprintf (stderr, "Failed to set SO_KEEPALIVE");
                    }
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
                } else {
                     fprintf(stderr, "Bug: Closing orphaned fd %d.\n", i);
                     close(i);
                     FD_CLR(i, &_master_read_set);
                }

            }
        }

    }

}
