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

export function isSupported(): boolean
export function initialize(config: PrinterConfig): true
export function printTest(): true
export type PrintForm = 'receipt' | 'access-pass'
export interface ReceiptData {
  name: string
  offeringType?: string
  amount: number
}
export interface AccessPassData {
  name: string
  department?: string
  qrValue: string
}
export function printJson(form: 'receipt', data: ReceiptData | string): true
export function printJson(form: 'access-pass', data: AccessPassData | string): true
export type FormStep =
  | { type: 'text' | 'qr'; value: string }
  | { type: 'left' | 'center' | 'right' | 'cut' }
  | { type: 'feed'; lines?: number }
export type Form = FormStep[]
export function setForms(forms: Record<string, Form>): true
export function print(form: string, values: Record<string, string | number>): true
export function getPrinterType(): string | null
export function getComPorts(): string[]
export function shutdown(): void
