# Sensor Data 생성 및 시각화 with Raylib

A real-time sensor data visualization tool that reads from an SQLite database and displays the data using raylib.
SQLite 데이터베이스에서 실시간 센서 데이터를 읽어 raylib을 사용하여 시각화하는 도구입니다.

./sensor_simulater 실행시키면 온도,습도,조도 랜덤값을 생성하여 sqlite db에 10초 간격으로 계속생성해주는 역할만 수행합니다.
./sensor_visualizer 실행시키면 시뮬레이터가 계속 생성하는 값을 주기적으로 읽어와서 모니터링 GUI 화면에 갱신 출력하는 역할을 수행합니다.

## Features / 기능

- Real-time sensor data visualization
  - 실시간 센서 데이터 시각화
- Supports temperature, humidity, and illuminance data
  - 온도, 습도, 조도 데이터 지원
- SQLite database backend for data storage
  - SQLite 데이터베이스 백엔드 사용
- Clean, responsive UI with multiple graphs
  - 다중 그래프를 포함한 깔끔한 UI
- Timezone-aware timestamp display (KST)
  - 한국 표준시(KST)로 시간 표시

## Prerequisites / 필수 사항

- C compiler (GCC, Clang, etc.)
  - C 컴파일러 (GCC, Clang 등)
- raylib library
  - raylib 라이브러리
- SQLite3 development files
  - SQLite3 개발 파일

## Installation / 설치

### macOS

```bash
# Install dependencies / 의존성 설치
brew install raylib sqlite3

# Clone the repository / 저장소 복제
git clone https://github.com/yourusername/sqlite-clangapp.git
cd sqlite-clangapp
```

### Ubuntu/Debian

```bash
# Install dependencies / 의존성 설치
sudo apt-get install libraylib-dev libsqlite3-dev

# Clone the repository / 저장소 복제
git clone https://github.com/yourusername/sqlite-clangapp.git
cd sqlite-clangapp
```

## Building / 빌드

### Build all components / 모든 컴포넌트 빌드

```bash
make
```

### Build only the visualizer / 시각화 도구만 빌드

```bash
make visual
```

### Build only the simulator / 시뮬레이터만 빌드

```bash
make sim
```

### Clean build files / 빌드 파일 정리

```bash
make clean
```

## Usage / 사용 방법

### 1. Run the sensor simulator / 센서 시뮬레이터 실행

```bash
./sensor_simulator
```

- Generates random sensor data every 10 seconds
  - 10초마다 랜덤한 센서 데이터 생성
- Data is saved to `sensor_data.db`
  - 데이터는 `sensor_data.db`에 저장됨
- Press `Ctrl+C` to stop
  - `Ctrl+C`로 종료 가능

### 2. Run the visualizer / 시각화 도구 실행

```bash
./sensor_visualizer
```

- Displays real-time graphs of sensor data
  - 센서 데이터를 실시간 그래프로 표시
- Automatically updates when new data is available
  - 새 데이터가 있으면 자동으로 업데이트
- Close the window or press `Ctrl+C` to exit
  - 창을 닫거나 `Ctrl+C`로 종료

### 3. Run both simultaneously / 동시에 실행하기

Open two terminal windows:

**Terminal 1 / 터미널 1**:

```bash
./sensor_simulator
```

**Terminal 2 / 터미널 2**:

```bash
./sensor_visualizer
```

## Project Structure / 프로젝트 구조

- `sensor_visualizer.c` - Main visualization application / 메인 시각화 애플리케이션
- `sensor_simulator.c` - Sensor data simulator / 센서 데이터 시뮬레이터
- `Makefile` - Build configuration / 빌드 설정
- `sensor_data.db` - SQLite database (created automatically) / SQLite 데이터베이스 (자동 생성)
- `README.md` - This file / 이 파일
- `LICENSE` - MIT License / MIT 라이선스

## Database Schema / 데이터베이스 스키마

### sensor_readings 테이블
```sql
CREATE TABLE sensor_readings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME NOT NULL,
    temperature FLOAT NOT NULL,
    humidity FLOAT NOT NULL,
    illuminance FLOAT NOT NULL
);
```

### 테이블 설명
- `id`: 자동 증가하는 고유 식별자
- `timestamp`: 데이터가 기록된 시간 (UTC)
- `temperature`: 섭씨 온도 값
- `humidity`: 습도 값 (%)
- `illuminance`: 조도 값 (lux)

### 샘플 데이터 조회
```sql
-- 최근 5개 데이터 조회
SELECT * FROM sensor_readings ORDER BY timestamp DESC LIMIT 5;

-- 특정 기간 데이터 조회 (예: 최근 1시간)
SELECT * FROM sensor_readings 
WHERE timestamp >= datetime('now', '-1 hour')
ORDER BY timestamp DESC;
```

## License / 라이선스

This project is open source and available under the [MIT License](LICENSE).
이 프로젝트는 [MIT 라이선스](LICENSE) 하에 오픈 소스로 제공됩니다.
