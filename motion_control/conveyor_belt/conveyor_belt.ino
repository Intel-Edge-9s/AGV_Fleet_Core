const int START_SENSOR = 2; // 시작 지점 적외선 센서
const int END_SENSOR = 3;   // 종료 지점 적외선 센서
const int MOTOR_IA = 9;     // 모터 드라이버 IA
const int MOTOR_IB = 10;    // 모터 드라이버 IB

bool isRunning = false;     // 컨베이어 벨트 상태 변수

void setup() {
  // put your setup code here, to run once:
  pinMode(START_SENSOR, INPUT);
  pinMode(END_SENSOR, INPUT);
  pinMode(MOTOR_IA, OUTPUT);
  pinMode(MOTOR_IB, OUTPUT);
  
  // 처음에는 정지 상태
  stopConveyor();
  
}

void loop() {
  // 적외선 센서는 보통 물체가 감지되면 LOW(0) 신호를 보냅니다.
  int startVal = digitalRead(START_SENSOR);
  int endVal = digitalRead(END_SENSOR);

  // [우선순위 1] 도착 센서 먼저 체크 (Stop Priority)
  if (endVal == LOW) { 
    stopConveyor();
    isRunning = false;
    // 도착 지점에 물체가 있으면 아래 조건(시작)은 아예 검사도 안 하고 넘어감
  } 
  // [우선순위 2] 도착 센서에 아무것도 없을 때만 시작 센서 체크
  else if (startVal == LOW) {
    startConveyor();
    isRunning = true;
  }
}

void startConveyor() {
  digitalWrite(MOTOR_IA, HIGH);
  digitalWrite(MOTOR_IB, LOW);
}

void stopConveyor() {
  digitalWrite(MOTOR_IA, LOW);
  digitalWrite(MOTOR_IB, LOW);
}
