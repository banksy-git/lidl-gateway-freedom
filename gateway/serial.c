/*
    Serial Port Routines - Serial port gateway
  =============================================
  Author: Paul Banks [https://paulbanks.org/]
*/

#include "serialgateway.h"

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define BAUD_CASE(b) case b: return B ## b;
#define INVALID_BAUD (~0)

static speed_t _baud_to_bits(int baud_bps)
{
    switch (baud_bps)
    {
        BAUD_CASE(0);
        BAUD_CASE(50);
        BAUD_CASE(75);
        BAUD_CASE(110);
        BAUD_CASE(134);
        BAUD_CASE(150);
        BAUD_CASE(200);
        BAUD_CASE(300);
        BAUD_CASE(600);
        BAUD_CASE(1200);
        BAUD_CASE(1800);
        BAUD_CASE(2400);
        BAUD_CASE(4800);
        BAUD_CASE(9600);
        BAUD_CASE(19200);
        BAUD_CASE(38400);
        BAUD_CASE(57600);
        BAUD_CASE(115200);
        BAUD_CASE(230400);
        BAUD_CASE(460800);
        BAUD_CASE(500000);
        BAUD_CASE(576000);
        BAUD_CASE(921600);
        BAUD_CASE(1000000);
        BAUD_CASE(1152000);
        BAUD_CASE(1500000);
        BAUD_CASE(2000000);
        BAUD_CASE(2500000);
        BAUD_CASE(3000000);
        BAUD_CASE(3500000);
        BAUD_CASE(4000000);
        default:
            errno = EINVAL;
            LOG_ERROR("Invalid baud rate: %d", baud_bps);
            return INVALID_BAUD;
    }
}

int serial_port_open(
    const char* serial_port,
    int baud_bps,
    bool is_hw_flow_control)
{
    speed_t baud_bits = _baud_to_bits(baud_bps);
    if (baud_bits==INVALID_BAUD) {
        return -1;
    }

    int fd = open(serial_port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd<0) {
        LOG_ERROR("Could not open serial port %s", serial_port);
        goto error;
    }
    fcntl(fd, F_SETFL, 0);

    struct termios options;
    if (tcgetattr (fd, &options) != 0) {
        LOG_ERROR("Could not get TTY attributes. Err %d", errno);
        goto error;
    }

    cfsetispeed (&options, baud_bits);
    cfsetospeed (&options, baud_bits);
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
        LOG_ERROR("Could not set TTY attributes. Err %d.", errno);
        goto error;
    }

    return fd;

error:
    if (fd != -1) {
        close(fd);
    }
    return -1;
}
