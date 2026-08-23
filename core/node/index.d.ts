export interface PrinterConfig {
  printerType?: 'AUTO' | 'BIXOLON' | 'EPSON'
  port?: string
  baudRate?: number
  dataBits?: number
  stopBits?: 1 | 2
  parity?: number
  dpi?: number
  printWidthDots?: number
}

export function initialize(config: PrinterConfig): true
export function printTest(): true
export function getPrinterType(): string | null
export function shutdown(): void
