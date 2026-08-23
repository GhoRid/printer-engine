# printer-engine

C++ 기반 프린터 엔진입니다.
Node.js에서는 `@ghorid/printer-engine` 패키지로 사용할 수 있습니다.

## 빌드

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
