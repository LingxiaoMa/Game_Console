#include <TFT_eSPI.h>  

#include <esp_now.h>
#include <WiFi.h>

#define CHANNELA 8
#define CHANNELB 9
TFT_eSPI tft = TFT_eSPI();  
//structure of message
typedef struct struct_message {
  bool left;
  bool right;
  bool up;
  bool down;
  bool a;
  bool b;
} struct_message;
//init of message
struct_message myDataA;
struct_message myDataB;

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// config AP SSID
void configDeviceAP() {
  const char *SSID = "Slave_1";
  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNELA, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    Serial.print("AP CHANNEL "); Serial.println(WiFi.channel());
  }
}

// 游戏相关参数
int paddleWidth = 40;
int paddleHeight = 10;
int paddleX, paddleY;
int ballX, ballY;
int ballDX = 2, ballDY = -2;
int ballRadius = 2;

int score =0;
enum GameState { PLAYING, GAME_OVER, WIN };
GameState gameState = PLAYING;


// 屏幕尺寸
int screenWidth = 160;
int screenHeight = 300;

//按键
const int left = 0;
const int right = 14;

// 砖块参数
const int numRows = 3;
const int numCols = 5;
int brickWidth = 23;
int brickHeight = 10;
bool bricks[numRows][numCols];

// 初始化挡板和球的位置
void initGame() {
  paddleX = screenWidth / 2 - paddleWidth / 2;
  paddleY = screenHeight - 20;

  ballX = screenWidth / 2;
  ballY = screenHeight / 2;

  score = 0;

  // 初始化砖块
  for (int i = 0; i < numRows; i++) {
    for (int j = 0; j < numCols; j++) {
      bricks[i][j] = true;
    }
  }
}

// 绘制挡板
void drawPaddle() {
  tft.fillRect(paddleX, paddleY, paddleWidth, paddleHeight, TFT_BLUE);
}

// 绘制球
void drawBall() {
  tft.fillCircle(ballX, ballY, ballRadius, TFT_RED);
}

// 绘制砖块
void drawBricks() {
  for (int i = 0; i < numRows; i++) {
    for (int j = 0; j < numCols; j++) {
      if (bricks[i][j]) {
        int brickX = j * (brickWidth + 5) + 10;
        int brickY = i * (brickHeight + 5) + 30;
        tft.fillRect(brickX, brickY, brickWidth, brickHeight, TFT_GREEN);
      }
    }
  }
}

void checkGameEnd() {
  // 检查球是否掉出屏幕
  if (ballY - ballRadius > screenHeight) {
    gameState = GAME_OVER;
  }

  // 检查是否所有砖块都被击碎
  bool allBricksDestroyed = true;
  for (int i = 0; i < numRows; i++) {
    for (int j = 0; j < numCols; j++) {
      if (bricks[i][j]) {
        allBricksDestroyed = false;
        break;
      }
    }
  }
  if (allBricksDestroyed) {
    gameState = WIN;
  }
}
void renderResultScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, screenHeight / 2 - 20);

  if (gameState == GAME_OVER) {
    tft.println("Game Over");
  } else if (gameState == WIN) {
    tft.println("You Win!");
  }

  tft.setTextSize(1);
  tft.setCursor(20, screenHeight / 2 + 20);
  tft.print("Your Score: ");
  tft.println(score);
  tft.println("Press any button to restart");
}
void restartGame() {
  if (digitalRead(left) == LOW || digitalRead(right) == LOW) {
    gameState = PLAYING;
    initGame();  // 重新初始化游戏
    tft.fillScreen(TFT_BLACK);  // 清空屏幕
    drawPaddle();
    drawBall();
    drawBricks();
  }
}


// 处理球的移动和碰撞检测
void updateBall() {
  // 擦除之前的球
  tft.fillCircle(ballX, ballY, ballRadius, TFT_BLACK);

  // 移动球
  ballX += ballDX;
  ballY += ballDY;

  // 撞到边缘反弹
  if (ballX - ballRadius < 0 || ballX + ballRadius > screenWidth) {
    ballDX = -ballDX;
  }
  if (ballY - ballRadius < 0) {
    ballDY = -ballDY;
  }

  // 撞到挡板反弹
  if (ballY + ballRadius >= paddleY && ballX >= paddleX && ballX <= paddleX + paddleWidth) {
    ballDY = -ballDY;
  }

  // 撞到砖块
  for (int i = 0; i < numRows; i++) {
    for (int j = 0; j < numCols; j++) {
      if (bricks[i][j]) {
        int brickX = j * (brickWidth + 5) + 10;
        int brickY = i * (brickHeight + 5) + 30;
        // 检查球是否撞到了砖块
        if (ballX + ballRadius >= brickX && ballX - ballRadius <= brickX + brickWidth &&
            ballY + ballRadius >= brickY && ballY - ballRadius <= brickY + brickHeight) {

          // 擦除砖块
          bricks[i][j] = false;
          score++;
          tft.fillRect(brickX, brickY, brickWidth, brickHeight, TFT_BLACK);

          // 判断碰撞方向
          if (ballY - ballRadius <= brickY || ballY + ballRadius >= brickY + brickHeight) {
            // 如果球的顶部或底部撞到了砖块的上下边缘
            ballDY = -ballDY;
          }
          else if (ballX - ballRadius <= brickX || ballX + ballRadius >= brickX + brickWidth) {
            // 如果球的左右边撞到了砖块的左右边缘
            ballDX = -ballDX;
          }
        }
      }
    }
  }

  // 绘制新的球位置
  drawBall();
}

// 处理挡板移动
void updatePaddle() {
  // 简单实现左右挡板移动，可用按钮或触摸屏增强交互
  tft.fillRect(paddleX, paddleY, paddleWidth, paddleHeight, TFT_BLACK);  // 擦除之前的挡板

  if (myDataA.left== true) {
    paddleX -= 5;
    if (paddleX < 0) {
      paddleX = 0;
    }
  }

  if (myDataA.right == true) {
    paddleX += 5;
    if (paddleX + paddleWidth > screenWidth) {
      paddleX = screenWidth - paddleWidth;
    }
  }
  myDataA.left=false;
  myDataA.right=false;
  drawPaddle();
}

void checkEnd(){
  if(ballY<paddleY){
    
  }
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *Data, int data_len) {
  // char macStr[18];
  // snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
  //          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // Serial.print("Last Packet Recv from: "); Serial.println(macStr);


  memcpy(&myDataA, Data, sizeof(myDataA));


  // Serial.print("Last Packet Recv Data: ");
  // Serial.println("");

}

void setup() {


  Serial.begin(115200);
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);

  //handle message

  // pinMode(left, INPUT_PULLUP);
  // pinMode(right, INPUT_PULLUP);

  tft.init();
  tft.setRotation(0);  // 适合纵向模式
  tft.fillScreen(TFT_BLACK);  // 清空屏幕

  initGame();  // 初始化游戏状态

  // 绘制游戏的初始状态
  drawPaddle();
  drawBall();
  drawBricks();
}

void loop() {
  if (gameState == PLAYING) {
    updatePaddle();  // 更新挡板位置
    updateBall();    // 更新球的位置和碰撞检测
    checkGameEnd();  // 检查游戏是否结束
  } else {
    renderResultScreen();  // 渲染游戏结果界面
    restartGame();         // 等待按键来重启游戏
  }

  delay(30);  // 控制帧速率
}


