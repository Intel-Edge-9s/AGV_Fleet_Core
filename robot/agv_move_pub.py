import rclpy
from rclpy.node import Node
from std_msgs.msg import String
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
import math, json, threading

class MoveExecutor(Node):
    def __init__(self):
        super().__init__('move_executor')
        # 구독자/발행자 설정
        self.cmd_sub = self.create_subscription(
            String, '/tb3_a/move_command', self.cmd_callback, 10)
        self.vel_pub = self.create_publisher(Twist, '/tb3_a/cmd_vel', 10)
        self.odom_sub = self.create_subscription(
            Odometry, '/tb3_a/odom', self.odom_callback, 10)
        self.done_pub = self.create_publisher(String, '/tb3_a/move_done', 10)

        # 로봇 상태 및 이동 파라미터 초기화
        self.action = None       # 현재 수행 중인 동작 (forward, backward, left, right)
        self.target = 0.0       # 목표 거리(m) 또는 목표 각도(rad)
        self.start_x = 0.0      # 동작 시작 시점의 X 좌표
        self.start_y = 0.0      # 동작 시작 시점의 Y 좌표
        self.start_yaw = 0.0    # 동작 시작 시점의 Yaw(회전) 값
        self.running = False    # 동작 수행 중 여부 플래그
        self.init_set = False   # 동작 시작 시점의 데이터 기록 완료 여부

    def cmd_callback(self, msg):
        """JSON 형태의 이동 명령을 수신하여 동작을 정의함"""
        if self.running: # 이미 동작 중이면 새 명령 무시
            return
            
        data = json.loads(msg.data)
        self.action = data['action']
        self.target = data['value']
        self.running = True
        self.init_set = False # odom_callback에서 시작 지점을 다시 잡도록 리셋
        self.get_logger().info(f'명령 수신: {self.action} / {self.target}')

    def publish_done(self):
        """동작 완료 시 UI나 상위 노드에 완료 알림을 보냄"""
        done_msg = String()
        done_msg.data = self.action
        self.done_pub.publish(done_msg)
        self.get_logger().info(f'완료 알림 발행: {self.action}')

    def odom_callback(self, msg):
        """실시간 오도메트리 데이터를 받아 목표치까지 로봇을 제어함 (핵심 로직)"""
        if not self.running:
            return

        # 현재 위치 및 자세(Yaw) 정보 추출
        x = msg.pose.pose.position.x
        y = msg.pose.pose.position.y
        yaw = self.get_yaw(msg.pose.pose.orientation)

        # 1. 동작 시작 시점의 위치 기록 (최초 1회)
        if not self.init_set:
            self.start_x = x
            self.start_y = y
            self.start_yaw = yaw
            self.init_set = True
            return

        twist = Twist()
        
        # 2. 직선 이동 제어 (Forward / Backward)
        if self.action in ('forward', 'backward'):
            # 유클리드 거리를 사용하여 실제 이동한 거리를 계산
            traveled = math.sqrt((x - self.start_x)**2 + (y - self.start_y)**2)
            error = self.target - traveled # 남은 거리

            if error > 0.01: # 오차 범위 1cm 밖이면 계속 이동
                # 남은 거리에 비례하여 속도를 조절하되 최소/최대 속도 제한 (P-제어 기초)
                vel = max(0.05, min(error * 0.2, 0.15))
                twist.linear.x = vel if self.action == 'forward' else -vel
                self.vel_pub.publish(twist)
            else: # 목표 도달 시 정지 및 완료 처리
                self.vel_pub.publish(Twist())
                self.running = False
                # 즉시 완료 알림을 보내면 관성에 의해 위치가 틀어질 수 있으므로 0.5초 대기 후 전송
                threading.Timer(0.5, self.publish_done).start()

        # 3. 회전 이동 제어 (Left / Right)
        elif self.action in ('left', 'right'):
            # 현재 각도와 시작 각도의 차이를 구하고 정규화(-pi ~ pi)
            if self.action == 'left':
                diff = self.normalize(yaw - self.start_yaw)
            else:
                diff = self.normalize(self.start_yaw - yaw)
            
            if diff < 0: # 회전 방향성 보정
                diff = 0.0
                
            error = self.target - diff # 남은 회전량

            if error > 0.02: # 오차 범위 약 1.1도(0.02 rad) 밖이면 계속 회전
                # 남은 각도에 비례하여 회전 속도 조절
                vel = max(0.1, min(error * 1.5, 0.3))
                twist.angular.z = vel if self.action == 'left' else -vel
                self.vel_pub.publish(twist)
            else: # 목표 회전량 도달 시 정지
                self.vel_pub.publish(Twist())
                self.running = False
                threading.Timer(0.5, self.publish_done).start()

    def get_yaw(self, q):
        """쿼터니언 데이터를 오일러 각도(Yaw)로 변환"""
        siny = 2.0 * (q.w * q.z + q.x * q.y)
        cosy = 1.0 - 2.0 * (q.y * q.y + q.z * q.z)
        return math.atan2(siny, cosy)

    def normalize(self, angle):
        """각도의 차이가 -PI ~ PI 범위를 넘지 않도록 보정 (회전 오차 계산 필수)"""
        while angle > math.pi:
            angle -= 2 * math.pi
        while angle < -math.pi:
            angle += 2 * math.pi
        return angle

def main():
    rclpy.init()
    node = MoveExecutor()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
