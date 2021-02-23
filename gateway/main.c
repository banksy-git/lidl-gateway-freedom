/*
    Serial port gateway for Silvercrest (Lidl) Smart Home Gateway
  =================================================================
  Author: Paul Banks [https://paulbanks.org/]

*/
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "serialgateway.h"
#include "serial.h"

#define DEFAULT_SERIAL_PORT "/dev/ttyS1"
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
    uint32_t baud_bps = 115200;
    char* serial_port = strdup(DEFAULT_SERIAL_PORT);
    opterr = 0;

    int c;
    while ((c = getopt (argc, argv, "fp:d:b:")) != -1) {
        switch(c) {
            case 'f':
                is_hardware_flow_control = false;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                free(serial_port);
                serial_port = strdup(optarg);
                break;
            case 'b':
                baud_bps = atoi(optarg);
                break;
            case '?':
            default:
                _error_exit("Unknown args");
        }
    }

    fprintf(stderr, "serialgateway: port %d, serial=%s, baud=%d, flow=%s\n",
            port, serial_port, baud_bps, (is_hardware_flow_control)?"HW":"sw");

    _serial_fd = serial_port_open(serial_port, baud_bps,
                                  is_hardware_flow_control);
    if (_serial_fd==-1) {
        _error_exit("Could not open serial port.");
    }

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
