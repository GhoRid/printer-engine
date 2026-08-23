#pragma once

class SerialPort;
struct PE_Printer;

SerialPort* pe_serial_port(PE_Printer* printer);
int pe_print_width_dots(const PE_Printer* printer);
