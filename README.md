# aircleaner



Baseline : https://github.com/Makerfabs/Project_Touch-Screen-Camera


20210620
- 버튼 순서 변경
- 팬 모터 강도에 따른 상이한 출력으로 변경
- Setting 버튼 클릭시 다른 화면으로 전환, 복귀 기능  추가
- WIFI가 연결 되었을 때만 날짜(시간) 출력 기능 추가
- 추가적인 GPIO 사용 가능성 확인 (UART 통신용)
- 추가적인 I2C 가능성 확인 (Pan Motor와 LED)

20210617
- 코드 최적화
- 배경화면 수정
- User Interface 제작
- 미세먼지 측정 센서와 터치 디스플레이 결합 (RX 35, TX 36)
- Wifi가 연결되어야만 사용 가능한 시스템 변경 (Not Connected 환경에서 API 제외하고 동작 가능)

20210610
- 기존 코드 인계