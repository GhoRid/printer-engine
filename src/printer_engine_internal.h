#pragma once

class PrinterBackend;
struct PE_Printer;

PrinterBackend* pe_backend(PE_Printer* printer);
int pe_print_width_dots(const PE_Printer* printer);
int pe_padding_left_dots(const PE_Printer* printer);
int pe_padding_right_dots(const PE_Printer* printer);
int pe_ascii_char_width_dots(const PE_Printer* printer);
