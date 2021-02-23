/*
    Serial Port Routines - Serial port gateway
  =============================================
  Author: Paul Banks [https://paulbanks.org/]
*/

#ifndef SPG_SERIAL_H
#define SPG_SERIAL_H

int serial_port_open(
    const char* serial_port, int baud_bps, bool is_hw_flow_control);

#endif // End header guard