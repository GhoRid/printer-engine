/** 프린터 드라이버 선택값. AUTO는 연결된 프린터를 자동 감지합니다. */
export type PrinterType = 'AUTO' | 'BIXOLON' | 'EPSON'
export type ActivePrinterType = Exclude<PrinterType, 'AUTO'>
/** none, odd, even, mark, space 순서입니다. */
export type SerialParity = 0 | 1 | 2 | 3 | 4

export interface PrinterConfig {
  /** 기본값: AUTO */
  printerType?: PrinterType
  /** 연결할 COM 포트 이름. 예: COM3 */
  port?: string
  /** 기본값: 115200 */
  baudRate?: number
  /** 기본값: 8 */
  dataBits?: 5 | 6 | 7 | 8
  /** 기본값: 1 */
  stopBits?: 1 | 2
  /** 기본값: 0 */
  parity?: SerialParity
  /** 기본값: 203 */
  dpi?: number
  /** 출력 가능한 최대 가로 너비(dot). 기본값: 576 */
  printWidthDots?: number
  /** 콘텐츠 왼쪽 여백(dot). 기본값: 24 */
  paddingLeftDots?: number
  /** 콘텐츠 오른쪽 여백(dot). 기본값: 24 */
  paddingRightDots?: number
  /** 자동 줄바꿈 계산에 쓰는 영문/숫자 한 글자의 폭(dot). 기본값: 12 */
  asciiCharWidthDots?: number
}

/** 현재 OS와 CPU 아키텍처에서 네이티브 기능을 사용할 수 있는지 확인합니다. */
export function isSupported(): boolean
/** 프린터 연결을 초기화합니다. 실패하면 예외를 던집니다. */
export function initialize(config: PrinterConfig): true
/** 연결 정보가 포함된 테스트 영수증을 출력합니다. */
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
export interface BuiltInFormDataMap {
  receipt: ReceiptData
  'access-pass': AccessPassData
}
/** 내장 양식을 객체 또는 JSON 문자열로 출력합니다. */
export function printJson<FormName extends PrintForm>(
  form: FormName,
  data: BuiltInFormDataMap[FormName] | string,
): true

export interface TextFormStep {
  type: 'text'
  /** `{{key}}` 플레이스홀더를 사용할 수 있습니다. */
  value: string
}
export interface QrFormStep {
  type: 'qr'
  /** `{{key}}` 플레이스홀더를 사용할 수 있습니다. */
  value: string
}
export interface ImageFormStep {
  type: 'image'
  /** process.cwd() 기준 PNG, JPEG, BMP, GIF 또는 TIFF 파일 경로입니다. */
  value: string
  /** 출력 너비(dot). 세로 길이는 원본 비율에 맞춰 계산됩니다. */
  width?: number
  /** 흑백 변환 임계값(0~255). 기본값: 160 */
  threshold?: number
}
export interface AlignFormStep {
  type: 'left' | 'center' | 'right'
}
export interface ColumnsFormStep {
  type: 'columns'
  /** 왼쪽 정렬할 내용. `{{key}}` 플레이스홀더를 사용할 수 있습니다. */
  left: string
  /** 오른쪽 정렬할 내용. 길면 오른쪽 열 안에서 줄바꿈됩니다. */
  right: string
}
export interface FeedFormStep {
  type: 'feed'
  /** 용지를 넘길 줄 수. 기본값: 1 */
  lines?: number
}
export interface CutFormStep {
  type: 'cut'
}

/** setForms()에서 사용할 수 있는 모든 출력 단계입니다. */
export type FormStep =
  | TextFormStep
  | QrFormStep
  | ImageFormStep
  | AlignFormStep
  | ColumnsFormStep
  | FeedFormStep
  | CutFormStep
export type Form = FormStep[]
export type Forms = Record<string, Form>
export type FormValue = string | number
export type FormValues = Record<string, FormValue>

/** 사용자 양식을 등록합니다. 이미지는 이때 변환되어 메모리에 캐시됩니다. */
export function setForms(forms: Forms): true
/** 등록한 양식에 플레이스홀더 값을 적용하여 출력합니다. */
export function print(form: string, values: FormValues): true
/** 초기화된 실제 프린터 종류입니다. 초기화 전에는 null입니다. */
export function getPrinterType(): ActivePrinterType | null
/** 사용할 수 있는 COM 포트의 이름과 설명 목록입니다. */
export function getComPorts(): string[]
/** 프린터 연결을 닫습니다. 여러 번 호출해도 안전합니다. */
export function shutdown(): void
