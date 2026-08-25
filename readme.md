# printer-engine

C++ 기반 프린터 엔진입니다.

Node.js에서는 `@ghorid/printer-engine` 패키지로 사용할 수 있습니다.

## Node.js에서 사용

Windows 32비트(`ia32`)와 64비트(`x64`) Node.js를 지원합니다.

```bash
npm install @ghorid/printer-engine
```

포트를 조회하고 프린터를 초기화한 뒤 기본 양식을 출력합니다.

```js
const printer = require("@ghorid/printer-engine");

const ports = printer.getComPorts();

// ['COM3 - Communications Port (COM3)', 'COM7 - USB Serial Device (COM7)']

if (ports.length === 0) {
  throw new Error("사용 가능한 COM 포트가 없습니다.");
}

const port = ports[0].split(" - ", 1)[0];

printer.initialize({
  printerType: "AUTO",
  port,
  baudRate: 115200,
  dataBits: 8,
  stopBits: 1,
  parity: 0,
  dpi: 203,
  printWidthDots: 576,
  paddingLeftDots: 24,
  paddingRightDots: 24,
  asciiCharWidthDots: 12,
});

try {
  printer.printTest();

  printer.printJson("receipt", {
    name: "홍길동",
    offeringType: "감사헌금",
    amount: 10000,
  });

  printer.printJson("access-pass", {
    name: "홍길동",
    department: "청년부",
    qrValue: "ABC123",
  });
} finally {
  printer.shutdown();
}
```

사용자 양식은 `initialize()` 이후 `shutdown()` 전에 등록하며, `{{이름}}` 형태의 자리표시자를 사용할 수 있습니다.

```js
printer.setForms({
  greeting: [
    { type: "center" },
    { type: "text", value: "{{name}}님 환영합니다." },
    { type: "feed", lines: 2 },
    { type: "cut" },
  ],
});

printer.print("greeting", {
  name: "홍길동",
});
```

이미지는 PNG, JPEG, BMP 등 Windows Imaging Component가 지원하는 파일을 사용할 수 있습니다.
`setForms()`를 호출할 때 한 번 흑백 비트맵으로 변환하여 캐시하므로, 반복 출력할 때
이미지 디코딩 비용이 다시 발생하지 않습니다. `width`를 생략하면 원본 너비를 사용하며,
프린터의 출력 가능 너비(dot)를 넘지 않도록 지정해야 합니다. `threshold`는 0~255이고
기본값은 160입니다.

```js
printer.setForms({
  receipt: [
    { type: "center" },
    { type: "image", value: "./assets/logo.png", width: 240 },
    { type: "text", value: "{{name}}" },
    { type: "cut" },
  ],
});
```

`text` 단계는 `printWidthDots`에서 좌우 패딩을 뺀 영역을 기준으로 자동 줄바꿈됩니다.
앞선 `left`, `center`, `right` 단계에 따라 각 줄도 같은 콘텐츠 영역 안에서 정렬됩니다.
왼쪽/오른쪽 값을 한 행에 배치하려면 `columns`를 사용합니다. 오른쪽 값이 길면
오른쪽 열의 폭에 맞춰 줄바꿈되며, 나뉜 모든 줄은 오른쪽 끝에 정렬됩니다.

```js
printer.setForms({
  receipt: [
    { type: "columns", left: "{{itemName}}", right: "{{amount}}" },
    { type: "feed" },
    { type: "cut" },
  ],
});
```

`initialize()`를 다시 호출하면 기존 포트를 닫고 새 설정으로 연결합니다. 사용이 끝나면 `shutdown()`을 호출하세요.

## C++ 빌드

### macOS

```bash
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release
cmake --build build-macos
```

`printer_engine_app`은 Win32 API 기반 Windows 전용 앱이므로 macOS에서는 생성되지 않습니다.

## Node.js Native Addon

의존성 설치:

```bash
npm install
```

32비트:

```powershell
npm run build:win32
```

64비트:

```powershell
npm run build:win64
```

32비트 + 64비트:

```powershell
npm run build:windows
```

생성 파일:

```text
prebuilds/
├── win32-ia32/
│   └── printer_engine_node.node
└── win32-x64/
    └── printer_engine_node.node
```

테스트:

```powershell
npm test
```

## npm 배포

npm 로그인:

```bash
npm login
```

배포 전 Windows Native Addon 빌드:

```powershell
npm run build:windows
```

버전 증가:

```bash
npm version patch
```

배포 파일 확인:

```bash
npm pack --dry-run
```

배포:

```bash
npm publish
```

일반적인 배포 순서:

```powershell
npm run build:windows
npm test
npm version patch
npm pack --dry-run
npm publish
```

## 참고

- `Win32` = 32비트
- `x64` = 64비트
- `printer_engine.lib`는 빌드용 정적 라이브러리이므로 앱 배포 시 필요하지 않습니다.
- Windows 앱 배포 시 기본적으로 `printer_engine_app.exe`를 사용합니다.

## Windows 앱 빌드

### Windows 32비트

```powershell
cmake -S . -B build-win32 -A Win32
cmake --build build-win32 --config Release --target printer_engine_app
```

실행 파일:

```text
build-win32/Release/printer_engine_app.exe
```

### Windows 64비트

```powershell
cmake -S . -B build-win64 -A x64
cmake --build build-win64 --config Release --target printer_engine_app
```

실행 파일:

```text
build-win64/Release/printer_engine_app.exe
```
