// Khai báo thư viện
#include <Wire.h> //Thư viện hỗ trợ giao tiếp I2C giữa Arduino và các thiết bị như màn hình LCD.
#include <LiquidCrystal_I2C.h> //Thư viện điều khiển màn hình LCD sử dụng giao tiếp I2C
#include <Servo.h> //Thư viện để điều khiển servo motor
/*____________________________________________________________________*/
// Định nghĩa các chân quan trọng
#define tien_motor_1 11               // chân IN - 1 của Module L298
#define lui_motor_1  10              // chân IN - 2 của Module L298
#define tien_motor_2 6              // chân IN - 3 của Module L298
#define lui_motor_2  5             // chân IN - 4 của Module L298
#define enA  7
#define enB  2 //Chân điều khiển tốc độ động cơ bằng PWM
#define servo 3 //Chân điều khiển Servo motor
#define congtac 13 //Chân gắn công tắc gạt để chuyển chế độ lái
/*____________________________________________________________________*/                 
// Khai báo biến
//uint8_t là kiểu số nguyên 8-bit (0 đến 255), dùng để tiết kiệm bộ nhớ
uint8_t data_from_arduino; //Dữ liệu nhận từ máy tính gửi qua Serial
uint8_t control_motor; //Giá trị để điều khiển hướng của xe
uint8_t driveModeSwitchtruoc,driveModeSwitch; //Theo dõi trạng thái công tắc trước và sau khi nhấn
uint8_t trangThaiNutNhan; //Trạng thái của chế độ điều khiển (0: tay, 1: tự lái)
uint8_t VTA,ESP;
// VTA – Tốc độ xe dưới dạng phần trăm
// ESP – Góc đánh lái của servo (0–180 độ)
uint16_t kqADC0,kqADC1; //Kết quả đọc từ cảm biến analog (A0, A1)
Servo myServo; //Tạo đối tượng servo
LiquidCrystal_I2C lcd(0x27, 16, 2); //Tạo màn hình LCD I2C (địa chỉ 0x27, 2 dòng, 16 cột)
/*____________________________________________________________________*/
// Thiết lập ban đầu cho các chân Arduino và khởi tạo giao tiếp Serial với máy tính
void setup() {
  Serial.begin(57600); //Khởi động giao tiếp Serial với tốc độ 57600 bps.
  Serial.setTimeout(1); //Thiết lập thời gian chờ tối đa để đọc dữ liệu Serial là 1ms.
  myServo.attach(servo); //Gán chân servo (số 3) cho đối tượng servo myServo
  lcd.begin(); //Khởi động màn hình LCD.
  lcd.backlight(); //Bật đèn nền của màn hình LCD.
  pinMode(congtac,INPUT_PULLUP); //Cấu hình chân công tắc là đầu vào và bật điện trở kéo lên.
  pinMode(tien_motor_1,OUTPUT);  //Cấu hình các chân điều khiển motor là đầu ra.
  pinMode(lui_motor_1,OUTPUT); 
  pinMode(tien_motor_2,OUTPUT); 
  pinMode(lui_motor_2,OUTPUT);
  pinMode(enA,OUTPUT); 
  pinMode(enB,OUTPUT);
  digitalWrite(tien_motor_1,LOW); //Đảm bảo tất cả các động cơ đều đang tắt khi khởi động.
  digitalWrite(lui_motor_1,LOW);
  digitalWrite(tien_motor_2,LOW);
  digitalWrite(lui_motor_2,LOW);
}
/*____________________________________________________________________*/
void loop() {
  driveModeSwitchtruoc = driveModeSwitch; //lưu lại trạng thái công tắc trước đó
  driveModeSwitch = digitalRead(13); //Đọc trạng thái hiện tại của công tắc.
  if(driveModeSwitchtruoc==1 && driveModeSwitch==0){//Nếu công tắc chuyển từ HIGH → LOW, đảo chế độ điều khiển:
    trangThaiNutNhan = 1-trangThaiNutNhan; //Nếu đang là 0 thì thành 1, nếu là 1 thì thành 0.
  }
  if(trangThaiNutNhan == 0){ //nếu này = 0 thì điều khiển thủ công
    lcd.setCursor(0,0);
    lcd.print("Self-Driving:OFF"); //Hiển thị "tự lái tắt" trên dòng đầu LCD
    kqADC0 = analogRead(A0); //Đọc giá trị cảm biến tốc độ.
    VTA = (uint32_t)kqADC0*100/1023; //Tính phần trăm tốc độ (chuyển từ 0–1023 thành 0–100%)
    dithang(kqADC0/4); //Gọi hàm đi thẳng, dùng giá trị làm tốc độ (0–255)
    kqADC1 = analogRead(A1); //Đọc giá trị góc lái từ cảm biến
    ESP = map(kqADC1,0,1023,0,180); //Chuyển giá trị 0–1023 thành góc servo 0–180
    myServo.write(ESP); //Gửi lệnh đánh lái cho servo motor
    lcd.setCursor(0,1);
    lcd.print("VTA:");lcd.print(VTA);lcd.print("%");
    lcd.print(" ESP:");hienThi1Byte(ESP);
  } //Hiển thị tốc độ và góc lái trên dòng 2 của màn hình LCD
  if(trangThaiNutNhan == 1){ //Nếu trangThaiNutNhan == 1: chế độ tự lái
    lcd.setCursor(0,0);
    lcd.print("Self-Driving:ON "); //Hiển thị trạng thái tự lái
    data_from_arduino = Serial.readString().toInt(); //Nhận dữ liệu góc lái từ máy tính
    control_motor = steering_angle(data_from_arduino); //Gọi hàm đánh lái theo góc nhận được
    
    if( control_motor == 90 ){
      dithang(255);
      lcd.setCursor(0,1);
      lcd.print("Go straight     "); 
    }
    else if( control_motor == 45 ){
      disangphai(150);
      lcd.setCursor(0,1);
      lcd.print("Turn right      ");
    }
    else if( control_motor == 135 ){
      disangtrai(150);
      lcd.setCursor(0,1);
      lcd.print("Turn left       ");
    }  
  } //Tùy theo góc 90 (thẳng), 45 (rẽ phải), 135 (rẽ trái) → điều khiển hướng xe tương ứng.
  delay(10); //Tạm dừng chương trình 10ms để tránh chạy quá nhanh.
}
/*____________________________________________________________________*/
// Điều khiển vô lăng đánh lái tự động mô phỏng bằng motor Servo
uint8_t steering_angle(uint8_t data_from_arduino){ //hàm trả về gốc đánh lái cho servo dựa trên dữ liệu nhận được từ máy tính (qua Serial)
  uint8_t steering_angle;
  if( data_from_arduino == 90 ){
    steering_angle = 90;
    myServo.write(steering_angle);
  }
  else if( data_from_arduino == 45 ){
    steering_angle = 45;
    myServo.write(steering_angle);
  }
  else if( data_from_arduino == 135 ){
    steering_angle = 135;
    myServo.write(steering_angle);
  }
  return steering_angle;
} //nếu giá trị nhận là 90, 45 hoặc 135, servo sẽ quay đến đúng góc tương ứng
//Sau đó, góc này được trả về để phần loop() sử dụng tiếp
/*____________________________________________________________________*/
// Điều khiển trạng thái của xe ( Đi thẳng, Rẽ Phải, Rẽ Trái, Dừng lại )
void dithang(uint8_t VTA){ //Điều khiển xe đi thẳng với tốc độ VTA (0–255). Sử dụng PWM để điều khiển tốc độ qua chân enA và enB.
  analogWrite(enA,VTA);
  analogWrite(enB,VTA);
  digitalWrite(tien_motor_1,HIGH); //→ chạy tiến motor 1   
  digitalWrite(lui_motor_1,LOW);
  digitalWrite(tien_motor_2,LOW); //→ chạy lùi motor 2
  digitalWrite(lui_motor_2,HIGH);      
} //→ Hai động cơ quay ngược chiều nhau để xe tiến thẳng
void disangphai(uint8_t VTA){ //Rẽ phải bằng cách chỉ kích hoạt motor 1
  resetdongco(); //gọi để dừng mọi moto trước
  analogWrite(enA,VTA);
  digitalWrite(tien_motor_1,HIGH); //Chạy tiến motor 1 (bên trái), để xe xoay về bên phải
  digitalWrite(lui_motor_1,LOW);
  digitalWrite(tien_motor_2,LOW);
  digitalWrite(lui_motor_2,LOW);  
}
void disangtrai(uint8_t VTA){ //Rẽ trái bằng cách chỉ kích hoạt motor 2
  resetdongco();
  analogWrite(enB,VTA);
  digitalWrite(tien_motor_1,LOW);
  digitalWrite(lui_motor_1,LOW);
  digitalWrite(tien_motor_2,LOW);
  digitalWrite(tien_motor_2,LOW); //Chạy lùi motor 2 (bên phải), xe xoay sang trái
  digitalWrite(lui_motor_2,HIGH);
}
void resetdongco(){ //Dừng toàn bộ động cơ: đưa tất cả chân điều khiển về mức 0 (LOW)
  digitalWrite(tien_motor_1,0);
  digitalWrite(lui_motor_1,0);
  digitalWrite(tien_motor_2,0);
  digitalWrite(lui_motor_2,0);
}
void hienThi1Byte(uint8_t dem){ //hàm in giá trị góc (ESP) lên LCD canh lề đều (vd:thêm dấu * cho đủ chiều dài)
  if(dem<10){lcd.print(dem);lcd.print("*  ");} //Nếu <10 → thêm * (2 dấu cách)
  else if(10<dem<100){lcd.print(dem);lcd.print("* ");} //Nếu <100 → thêm * (1 dấu cách)
  else {lcd.print(dem);lcd.print("*");} //Nếu ≥100 → chỉ thêm *
}//->đảm bảo thông tin in lên LCD dễ nhìn, không bị lệch dòng
