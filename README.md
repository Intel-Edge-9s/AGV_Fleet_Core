## 🚛 Logistics-Automation-Mini-Project

### 🛠 개발 배경
<table>
  <tr>
    <td align="center"><img src="./다운로드/autonomous-mobile-robots-amr-marketsss.webp" width="100%" /></td>
    <br><b>자율 이동 로봇 시장 규모 및 점유율 보고서, 2035년</b>
    <td align="center"><img src="./다운로드/us-autonomous-mobile-robots-market-sizess.webp" width="100%" /></td>
  </tr>
</table>

### 📝 한 줄 요약

---

## 📅 프로젝트 개요
- **프로젝트 명:** AMR FLEET CORE
- **수행 기간:** 2026.04.20 ~ 2026.04.27
- **주요 기능**
  - **1:** TurtleBot 2대 기반 다중 물류 로봇 제어
  - **2:** A* 알고리즘 기반 경로 탐색 및 장애물 회피
  - **3:** 우선순위, 임계구역, 재탐색, 대피소 기반 충돌 예외 처리
  - **4:** Qt 관제 UI와 MariaDB를 통한 실시간 모니터링 및 작업 이력 관리

---

## 🛠 기술 스택
| 분류 | 기술 Stack |
| :--- | :--- |
| **Languages** | C++, Python |
| **Communication** | ROS2, Wi-Fi, Serial |
| **Frameworks** | Qt 6 |
| **Database** | MariaDB |
| **Hardware/OS** | TurtleBot3, Raspberry Pi, Ubuntu 22.04 |

---

## 📂 디렉토리 구조


---

## 🔍 상세 기능 설명

---

## 🖼 시연 화면

---

## 🎬 시연 영상



---

## ⚠️ 보완점 및 향후 과제
- **Pose 및 좌표 재보정**  
  LiDAR 센서를 활용해 양옆, 전후 거리를 측정하고 목표 좌표 도착 후 위치 및 각도 오차를 자동 보정하는 기능을 추가할 예정이다.

- **카메라 및 QR 코드 인식**  
  OpenCV 기반 QR 코드 인식을 통해 위치 검증 및 화물 정보를 확인하고, 인식 결과에 따라 원하는 좌표로 이동하는 로직으로 확장할 예정이다.

- **Grid Map 생성 자동화**  
  현재 하드코딩된 Grid Map 구조를 개선하여, TurtleBot을 수동 조작하면서 이동 가능한 좌표를 기록하고 파일로 저장하는 기능을 적용할 예정이다.

- **실시간 장애물 대응 및 동적 재탐색**  
  주행 중 예기치 않은 장애물이나 경로 차단 상황이 발생할 경우, 현재 위치 기준으로 경로를 다시 계산하는 기능을 강화할 예정이다.

- **다중 로봇 확장성 개선**  
  현재 2대 기준으로 검증한 충돌 회피 및 작업 스케줄링 구조를 3대 이상의 TurtleBot 환경에서도 안정적으로 동작하도록 확장할 예정이다.
---

## 💁‍♂️ 팀원

| 이름 | 역할 | 담당 파트 |
|----------|----------|----------|
| 김준기 | 팀장 | |
| 허준형 | 부팀장 | DB 설계, 컨베이어 벨트 구동 제어, QT & 터틀 봇 연동 작업 |
| 정구빈 | Frontend/Firmware | Qt 기반 통합 관제 대시보드 구현 및 엑추에이터 구동 제어 |
| 박준서 | | 테스트 커밋 |
