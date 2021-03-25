/*
    Serial port gateway for Silvercrest (Lidl) Smart Home Gateway
  =================================================================
  Author: Paul Banks [https://paulbanks.org/]

*/
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "serialgateway.h"
#include "serial.h"

#define DEFAULT_SERIAL_PORT "/dev/ttyS1"
#define BUF_SIZE 1024

#define OOB_HW_FLOW_OFF 0x10
#define OOB_HW_FLOW_ON 0x11

static fd_set _master_read_set;
static fd_set _master_except_set;
static fd_set _read_fd_set;
static fd_set _except_fd_set;
struct serial_settings {
    bool is_hardware_flow_control; // True if hardware flow control
    uint32_t baud_bps; // Baud rate. Must be valid (see serial.h)
    char* device; // Path to serial device
};
static struct serial_settings _serial_settings;

static int _serial_fd = -1;
static int _connection_fd = -1;
static int _buf[BUF_SIZE];


#ifdef ENABLE_DEBUGLOG
#define LOG_DEBUG(format, ...) fprintf(stderr, format "\n", __VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif

int sockatmark(int fd)
{
    int r;
    return ioctl(fd, SIOCATMARK, &r) == -1 ? -1 : r;
}

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
        FD_CLR(_connection_fd, &_read_fd_set);
        FD_CLR(_connection_fd, &_master_except_set);
        FD_CLR(_connection_fd, &_except_fd_set);
        _connection_fd = -1;
    }
}

static void _open_serial_port()
{
    if (_serial_fd != -1) {
        FD_CLR(_serial_fd, &_master_read_set);
        FD_CLR(_serial_fd, &_read_fd_set);
        close(_serial_fd);
    }
    _serial_fd = serial_port_open(_serial_settings.device,
                                  _serial_settings.baud_bps,
                                  _serial_settings.is_hardware_flow_control);
    if (_serial_fd==-1) {
        _error_exit("Could not open serial port.");
    }
    FD_SET(_serial_fd, &_master_read_set);
}

static void _handle_oob_command()
{
    char oob_op;
    size_t len = recv(_connection_fd, &oob_op, 1, MSG_OOB);
    if (len==1) {
        switch (oob_op) {
            case OOB_HW_FLOW_OFF:
                fprintf(stderr, "Flow control OFF\n");
                _serial_settings.is_hardware_flow_control = false;
                _open_serial_port();
                break;
            case OOB_HW_FLOW_ON:
                fprintf(stderr, "Flow control ON\n");
                _serial_settings.is_hardware_flow_control = true;
                _open_serial_port();
                break;
            default:
                fprintf(stderr, "Unknown OOB command %d\n", oob_op);
        }
    }
    FD_SET(_connection_fd, &_master_except_set);
}

int main(int argc, char** argv)
{
    FD_ZERO (&_master_read_set);
    FD_ZERO (&_master_except_set);
    uint16_t port = 8888;
    _serial_settings.is_hardware_flow_control = true;
    _serial_settings.baud_bps = 115200;
    _serial_settings.device = strdup(DEFAULT_SERIAL_PORT);
    opterr = 0;

    signal(SIGPIPE, SIG_IGN);

    int c;
    while ((c = getopt (argc, argv, "fp:d:b:")) != -1) {
        switch(c) {
            case 'f':
                _serial_settings.is_hardware_flow_control = false;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                free(_serial_settings.device);
                _serial_settings.device = strdup(optarg);
                break;
            case 'b':
                _serial_settings.baud_bps = atoi(optarg);
                break;
            case '?':
            default:
                _error_exit("Unknown args");
        }
    }

    fprintf(stderr, "serialgateway " VERSION
                    ": port %d, serial=%s, baud=%d, flow=%s\n",
            port, _serial_settings.device, _serial_settings.baud_bps,
            (_serial_settings.is_hardware_flow_control)?"HW":"sw");

    _open_serial_port();

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

    FD_SET (listen_sock, &_master_read_set);

    int i;
    for(;;)
    {
        _read_fd_set = _master_read_set;
        _except_fd_set = _master_except_set;
        if (select (FD_SETSIZE, &_read_fd_set, NULL, &_except_fd_set, NULL) < 0)
        {
            _error_exit("select");
        }

        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &_except_fd_set)) {
                FD_CLR(i, &_master_except_set);
                if (_connection_fd==i && sockatmark(i)==1) {
                    LOG_DEBUG("Socket exceptfd %d", 1);
                    _handle_oob_command();
                }
            }

            if (FD_ISSET (i, &_read_fd_set)) {
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
                         "Connect from host %s fd=%d\n",
                         inet_ntoa (clientname.sin_addr), new);
                    int enable = 1;
                    if (setsockopt(new, SOL_SOCKET, SO_KEEPALIVE,
                                   &enable, sizeof(enable)) < 0) {
                        fprintf (stderr, "Failed to set SO_KEEPALIVE");
                    }
                    FD_SET (new, &_master_read_set);
                    FD_SET (new, &_master_except_set);
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

                        if (sockatmark(_connection_fd)==1) {
                            _handle_oob_command();
                        }

                    }

                } else {
                     fprintf(stderr, "Bug: Closing orphaned fd %d.\n", i);
                     close(i);
                     FD_CLR(i, &_master_read_set);
                     FD_CLR(i, &_master_except_set);
                }

            }
        }

    }

}
