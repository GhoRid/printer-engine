# printer-engine

Windows/macOS에서 사용할 수 있는 C++ 기반 프린터 엔진입니다.

Node.js에서는 `@ghorid/printer-engine` 패키지를 통해 사용할 수 있습니다.

## 프로젝트 구조

```text
printer-engine/
├── app/                 # Windows GUI 앱
├── include/             # 외부 공개 헤더
├── node/                # Node.js N-API 바인딩
├── prebuilds/           # npm 배포용 Windows 네이티브 바이너리
├── src/                 # printer-engine 구현
├── tests/               # 테스트 코드
├── CMakeLists.txt
├── package.json
└── package-lock.json
```

# 빌드

## Windows

프로젝트 루트에서 실행합니다.

### CMake 설정

```powershell
cmake -S . -B build
```

32비트
cmake -S . -B build-win32 -A Win32
cmake --build build-win32 --config Release --target printer_engine_app

### Release 빌드

```powershell
cmake --build build --config Release
```

### Windows 앱만 빌드

```powershell
cmake --build build --config Release --target printer_engine_app
```

Visual Studio Generator를 사용하는 경우 실행 파일은 일반적으로 다음 위치에 생성됩니다.

```text
build/Release/printer_engine_app.exe
```

빌드를 처음부터 다시 하고 싶다면 기존 `build` 폴더를 삭제한 뒤 다시 빌드합니다.

```powershell
Remove-Item -Recurse -Force build

cmake -S . -B build
cmake --build build --config Release
```

---

## macOS

프로젝트 루트에서 실행합니다.

### CMake 설정

Release 빌드 타입을 지정합니다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

### 빌드

```bash
cmake --build build
```

처음부터 다시 빌드하려면:

```bash
rm -rf build

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

`ninja: no work to do.`가 출력되는 경우 오류가 아니라 기존 빌드 결과가 최신 상태라는 의미입니다.

> `printer_engine_app`이 Win32 API를 사용하는 Windows 전용 앱이라면 macOS에서는 해당 앱을 빌드할 수 없습니다.
> macOS 빌드는 공통 C++ 엔진과 macOS용 코드의 빌드 확인에 사용합니다.

# Node.js 패키지

npm 패키지:

```text
@ghorid/printer-engine
```

## 의존성 설치

프로젝트 루트에서:

```bash
npm install
```

## Windows Node Native Addon 빌드

현재 npm 패키지의 네이티브 바이너리는 Windows용으로 제공합니다.

### 32비트

```powershell
npm run build:win32
```

생성된 바이너리:

```text
prebuilds/win32-ia32/printer_engine_node.node
```

### 64비트

```powershell
npm run build:win64
```

생성된 바이너리:

```text
prebuilds/win32-x64/printer_engine_node.node
```

### 32비트 + 64비트 전체 빌드

```powershell
npm run build:windows
```

두 빌드가 완료되면 다음 파일들이 존재해야 합니다.

```text
prebuilds/
├── win32-ia32/
│   └── printer_engine_node.node
└── win32-x64/
    └── printer_engine_node.node
```

## Node 테스트

```powershell
npm test
```

# npm 배포

## 1. npm 로그인

```bash
npm login
```

로그인 상태 확인:

```bash
npm whoami
```

## 2. 버전 변경

npm에는 이미 배포된 동일 버전을 다시 배포할 수 없습니다.

예를 들어 현재 버전이:

```json
{
  "version": "0.1.0"
}
```

이라면 다음 배포 전에 버전을 올립니다.

패치 버전:

```bash
npm version patch
```

예:

```text
0.1.0
→
0.1.1
```

기능 추가 버전:

```bash
npm version minor
```

예:

```text
0.1.0
→
0.2.0
```

메이저 버전:

```bash
npm version major
```

예:

```text
0.1.0
→
1.0.0
```

## 3. Windows 네이티브 바이너리 빌드

npm 배포 전에 Windows에서 실행합니다.

```powershell
npm run build:windows
```

## 4. 배포 파일 확인

실제 npm에 어떤 파일이 포함되는지 확인합니다.

```bash
npm pack --dry-run
```

주요 포함 파일:

```text
node/index.js
node/index.d.ts
prebuilds/win32-ia32/printer_engine_node.node
prebuilds/win32-x64/printer_engine_node.node
```

## 5. npm 배포

```bash
npm publish --access public
```

`@ghorid/printer-engine`은 scoped package이므로 public 배포를 위해 `--access public`을 사용합니다.

현재 `package.json`에도 다음 설정이 있습니다.

```json
{
  "publishConfig": {
    "access": "public"
  }
}
```

따라서 이후에는 다음 명령어만 사용해도 됩니다.

```bash
npm publish
```

배포 시 `prepublishOnly` 스크립트가 자동 실행되며 Windows 32비트/64비트 prebuild 파일이 존재하는지 확인합니다.

# npm 배포 순서 요약

```text
코드 수정
   ↓
Windows에서 Node Native Addon 빌드
   ↓
npm run build:windows
   ↓
npm test
   ↓
npm version patch
   ↓
npm pack --dry-run
   ↓
npm publish
```

일반적인 패치 배포라면:

```powershell
npm run build:windows
npm test
npm version patch
npm pack --dry-run
npm publish
```
