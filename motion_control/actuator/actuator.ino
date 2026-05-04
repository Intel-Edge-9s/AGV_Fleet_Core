const int SENSOR_PIN = 7; // 적외선 센서 신호선
const int PWM_PIN = 9;    // MDD10A PWM
const int DIR_PIN = 10;   // MDD10A DIR

void setup() {
  pinMode(SENSOR_PIN, INPUT);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  
  analogWrite(PWM_PIN, 0);
} 

void loop() {
  // 적외선 센서에 물체가 감지되면 (LOW일 때 작동)
  if (digitalRead(SENSOR_PIN) == LOW) {
    
    // 물체가 완전히 멈추거나 위치를 잡을 때까지 대기
    delay(500); 

    // 엑추에이터 1세트 작동 (전진 후 바로 후진)
    executeOneSet();

    // 무한 반복 방지: 물체가 센서 앞에서 치워질 때까지 대기
    while (digitalRead(SENSOR_PIN) == LOW) {
      delay(100); 
    }
    
    delay(1000); // 다음 감지 전 시스템 안정화 시간
  }
}

// 엑추에이터 1세트 동작 함수
void executeOneSet() {
  // [전진]
  digitalWrite(DIR_PIN, HIGH);
  analogWrite(PWM_PIN, 255);
  delay(9000); // 전진 시간 (9초)

  // [잠시 정지]
  analogWrite(PWM_PIN, 0);
  delay(300); 

  // [후진]
  digitalWrite(DIR_PIN, LOW);
  analogWrite(PWM_PIN, 255);
  delay(9000); // 후진 시간 (9초)

  // [최종 정지]
  analogWrite(PWM_PIN, 0);
}
