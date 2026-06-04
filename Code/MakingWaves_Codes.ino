#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050.h>

MPU6050 mpu;

#define BTN_PIN     4
#define VIBRO_PIN   3
#define SCREEN_W   128
#define SCREEN_H    64
#define OLED_RESET  -1
#define OLED_ADDR  0x3C

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);

#define SLEEP_AFTER     18000UL
#define LONG_PRESS_MS     600UL
#define DOUBLE_PRESS_MS   380UL
#define DUR_CONNECTED    4000UL
#define DUR_HAPPY        1800UL
#define DUR_EAT          2500UL
#define DUR_SING         3000UL
#define DUR_HEART        2200UL
#define DUR_SURPRISED     900UL
#define DUR_SHY          2000UL
#define DUR_PLAYING      2500UL

// For watch
int startHour   = 1;
int startMinute = 23;
unsigned long clockStartMillis = 0;

// Detect wave
#define WAVE_THRESHOLD   2000  
#define WAVE_COUNT_NEED  3    
#define WAVE_WINDOW      2000UL 

int16_t prevAx = 0, prevAy = 0, prevAz = 0;
int     waveCount     = 0;
unsigned long firstWaveTime = 0;

// Conditions 
enum PetState {
  IDLE, HAPPY, CONNECTED, EATING,
  SINGING, HEART, SURPRISED, SLEEPING, SHY, PLAYING
};
PetState petState = IDLE;

unsigned long stateAt  = 0;
unsigned long lastAct  = 0;
bool btnPrev      = HIGH;
unsigned long btnDown     = 0;
unsigned long lastRelease = 0;
bool waitDouble   = false;
int  actionIndex  = 0;

// Movement of Cat
int catOffsetX = 0;
int catTargetX = 0;
unsigned long lastWanderMove   = 0;
unsigned long lastWanderTarget = 0;

// Vibration
void vibrate(int ms) {
  digitalWrite(VIBRO_PIN, HIGH);
  delay(ms);
  digitalWrite(VIBRO_PIN, LOW);
}
void vibratePattern(int n, int on, int off) {
  for (int i = 0; i < n; i++) {
    vibrate(on);
    if (i < n - 1) delay(off);
  }
}

// Detect wave (left to right)
void detectWave() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  long delta = abs(ay - prevAy) * 2 + abs(ax - prevAx);
  prevAx = ax; prevAy = ay; prevAz = az;

  if (delta > WAVE_THRESHOLD) {
    if (waveCount == 0) firstWaveTime = millis();
    waveCount++;
  }

  if (waveCount > 0 && millis() - firstWaveTime > WAVE_WINDOW) {
    waveCount = 0;
  }

  if (waveCount >= WAVE_COUNT_NEED && petState != CONNECTED) {
    petState  = CONNECTED;
    stateAt   = millis();
    lastAct   = millis();
    waveCount = 0;
    vibratePattern(3, 200, 150);
  }
}

// Movement 
int breathDy() {
  float t = (float)(millis() % 2400) / 2400.0f;
  return (int)(1.0f * sin(t * 2.0f * PI));
}

void updateCatWander() {
  unsigned long now = millis();
  if (now - lastWanderTarget > 2000) {
    catTargetX = random(-8, 9);
    lastWanderTarget = now;
  }
  if (now - lastWanderMove > 180) {
    if (catOffsetX < catTargetX)      catOffsetX++;
    else if (catOffsetX > catTargetX) catOffsetX--;
    lastWanderMove = now;
  }
}

// Drawing Cat
void drawCat(int cx, int cy, int eyeType, int dy) {
  int y = cy + dy;

  // Ear
  display.fillTriangle(cx-14, y-4, cx-6,  y-4, cx-10, y-11, SSD1306_WHITE);
  display.fillTriangle(cx+6,  y-4, cx+14, y-4, cx+10, y-11, SSD1306_WHITE);
  display.fillTriangle(cx-12, y-4, cx-9,  y-4, cx-10, y-8,  SSD1306_BLACK);
  display.fillTriangle(cx+9,  y-4, cx+12, y-4, cx+10, y-8,  SSD1306_BLACK);

  // Face
  display.fillRoundRect(cx-18, y-7, 36, 28, 15, SSD1306_WHITE);

  // Body
  display.fillRoundRect(cx-11, y+18, 22, 10, 5, SSD1306_WHITE);

  // Feet
  display.drawLine(cx-8, y+25, cx-8, y+28, SSD1306_BLACK);
  display.drawLine(cx-4, y+25, cx-4, y+28, SSD1306_BLACK);
  display.drawLine(cx+4, y+25, cx+4, y+28, SSD1306_BLACK);
  display.drawLine(cx+8, y+25, cx+8, y+28, SSD1306_BLACK);

  // Tail
  int tx0=cx+13, ty0=y+22, tx1=cx+20, ty1=y+22, tx2=cx+18, ty2=y+14;
  for (int i=0; i<=14; i++) {
    float t=i/14.0f, nt=1-t;
    int bx=(int)(nt*nt*tx0+2*nt*t*tx1+t*t*tx2);
    int by=(int)(nt*nt*ty0+2*nt*t*ty1+t*t*ty2);
    display.fillCircle(bx, by, 1, SSD1306_WHITE);
  }

  // eyes
  int lx=cx-10, rx=cx+10, ey=y+7, my=y+10;
  switch (eyeType) {
    case 0:
      display.fillRect(lx-1, ey-1, 2, 2, SSD1306_BLACK);
      display.fillRect(rx-1, ey-1, 2, 2, SSD1306_BLACK);
      break;
    case 1:
      display.drawLine(lx-3, ey+1, lx,   ey-1, SSD1306_BLACK);
      display.drawLine(lx,   ey-1, lx+3, ey+1, SSD1306_BLACK);
      display.drawLine(rx-3, ey+1, rx,   ey-1, SSD1306_BLACK);
      display.drawLine(rx,   ey-1, rx+3, ey+1, SSD1306_BLACK);
      break;
    case 2:
      display.drawLine(lx-2, ey, lx+2, ey, SSD1306_BLACK);
      display.drawLine(rx-2, ey, rx+2, ey, SSD1306_BLACK);
      break;
    case 3:
      display.drawLine(lx-3, ey, lx+3, ey, SSD1306_BLACK);
      display.drawLine(rx-3, ey, rx+3, ey, SSD1306_BLACK);
      break;
    case 4:
      display.drawRect(lx-1, ey-1, 3, 3, SSD1306_BLACK);
      display.drawRect(rx-1, ey-1, 3, 3, SSD1306_BLACK);
      break;
    case 5:
      display.fillCircle(lx-1, ey-1, 1, SSD1306_BLACK);
      display.fillCircle(lx+1, ey-1, 1, SSD1306_BLACK);
      display.fillTriangle(lx-3, ey, lx+3, ey, lx, ey+3, SSD1306_BLACK);
      display.fillCircle(rx-1, ey-1, 1, SSD1306_BLACK);
      display.fillCircle(rx+1, ey-1, 1, SSD1306_BLACK);
      display.fillTriangle(rx-3, ey, rx+3, ey, rx, ey+3, SSD1306_BLACK);
      break;
    case 6:
      display.fillRect(lx-1, ey-1, 2, 2, SSD1306_BLACK);
      display.drawLine(rx-2, ey, rx+2, ey, SSD1306_BLACK);
      break;
    case 7:
      display.drawLine(lx-3, ey+1, lx,   ey-1, SSD1306_BLACK);
      display.drawLine(lx,   ey-1, lx+3, ey+1, SSD1306_BLACK);
      display.drawLine(rx-3, ey+1, rx,   ey-1, SSD1306_BLACK);
      display.drawLine(rx,   ey-1, rx+3, ey+1, SSD1306_BLACK);
      break;
    case 8:
      display.fillRect(lx-1, ey, 2, 2, SSD1306_BLACK);
      display.fillRect(rx-1, ey, 2, 2, SSD1306_BLACK);
      break;
    case 9:
      display.fillRect(lx-1, ey-1, 2, 2, SSD1306_BLACK);
      display.drawLine(rx-3, ey, rx+3, ey, SSD1306_BLACK);
      break;
  }

  // Mouth
  if (eyeType == 4) {
    display.drawCircle(cx, my, 2, SSD1306_BLACK);
  } else if (eyeType == 6) {
    display.fillCircle(cx, my, 3, SSD1306_BLACK);
    display.fillCircle(cx, my+1, 1, SSD1306_WHITE);
  } else if (eyeType == 3) {
    display.drawLine(cx-2, my, cx+2, my, SSD1306_BLACK);
  } else {
    display.drawLine(cx-4, my-1, cx-2, my+1, SSD1306_BLACK);
    display.drawLine(cx-2, my+1, cx,   my-1, SSD1306_BLACK);
    display.drawLine(cx,   my-1, cx+2, my+1, SSD1306_BLACK);
    display.drawLine(cx+2, my+1, cx+4, my-1, SSD1306_BLACK);
  }

  // Blush
  if (eyeType == 8 || eyeType == 5) {
    display.drawLine(cx-16, y+11, cx-12, y+10, SSD1306_BLACK);
    display.drawLine(cx-15, y+14, cx-11, y+13, SSD1306_BLACK);
    display.drawLine(cx+12, y+10, cx+16, y+11, SSD1306_BLACK);
    display.drawLine(cx+11, y+13, cx+15, y+14, SSD1306_BLACK);
  }
}

// Graphic Helper
void drawHeart(int x, int y, int r) {
  display.fillCircle(x-r/2, y, r/2, SSD1306_WHITE);
  display.fillCircle(x+r/2, y, r/2, SSD1306_WHITE);
  display.fillTriangle(x-r, y, x+r, y, x, y+r, SSD1306_WHITE);
}

// Wave shape
void drawWave(int x, int y) {
  display.drawLine(x,    y,   x+4,  y-4, SSD1306_WHITE);
  display.drawLine(x+4,  y-4, x+8,  y,   SSD1306_WHITE);
  display.drawLine(x+8,  y,   x+12, y-4, SSD1306_WHITE);
  display.drawLine(x+12, y-4, x+16, y,   SSD1306_WHITE);
}

void drawStar(int x, int y) {
  display.drawLine(x-3, y, x+3, y, SSD1306_WHITE);
  display.drawLine(x, y-3, x, y+3, SSD1306_WHITE);
  display.drawLine(x-2, y-2, x+2, y+2, SSD1306_WHITE);
  display.drawLine(x+2, y-2, x-2, y+2, SSD1306_WHITE);
}
void drawNote(int x, int y) {
  display.fillRect(x, y, 2, 7, SSD1306_WHITE);
  display.fillRect(x+5, y-3, 2, 7, SSD1306_WHITE);
  display.fillRect(x, y, 7, 2, SSD1306_WHITE);
  display.fillCircle(x, y+8, 3, SSD1306_WHITE);
  display.fillCircle(x+5, y+5, 3, SSD1306_WHITE);
}
void drawFood(int x, int y) {
  display.fillRoundRect(x-8, y+4, 16, 4, 2, SSD1306_WHITE);
  display.fillCircle(x, y, 4, SSD1306_WHITE);
  display.fillCircle(x-4, y+1, 3, SSD1306_WHITE);
  display.fillCircle(x+4, y+1, 3, SSD1306_WHITE);
}
void drawZZZ(unsigned long el) {
  int f = (el/500) % 3;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  if (f >= 0) { display.setCursor(58, 6); display.print("z"); }
  if (f >= 1) { display.setCursor(65, 2); display.print("z"); }
  if (f >= 2) { display.setCursor(72, 0); display.print("Z"); }
}
void drawMsg(const char* msg) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int len = strlen(msg);
  display.setCursor((128-len*6)/2, 56);
  display.print(msg);
}
void drawClock() {
  unsigned long elapsedSec = (millis()-clockStartMillis)/1000UL;
  int totalMinutes = startHour*60 + startMinute + (int)(elapsedSec/60);
  totalMinutes = totalMinutes % 1440;
  int h = totalMinutes/60;
  int m = totalMinutes%60;
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", h, m);
  display.drawRoundRect(88, 0, 36, 14, 3, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(91, 3);
  display.print(timeStr);
}

// Rendering
const int CX = 62;
const int CY = 25;

void renderIDLE() {
  if ((millis()-lastAct) > SLEEP_AFTER) {
    petState = SLEEPING; stateAt = millis(); return;
  }
  updateCatWander();
  int dy = breathDy();
  bool blink = ((millis()%4200) < 130);
  display.clearDisplay();
  drawCat(CX+catOffsetX, CY, blink?2:0, dy);
  drawClock();
  display.display();
}

void renderHAPPY() {
  unsigned long el = millis()-stateAt;
  float p = (float)el/DUR_HAPPY;
  int dy = (int)(-8.0f*sin(p*PI));
  int sc = (p<0.5f)?(int)(p*10):(int)((1-p)*10);
  display.clearDisplay();
  drawCat(CX, CY, 1, dy);
  if (sc>0) drawStar(22, 8);
  if (sc>3) drawStar(98, 12);
  if (sc>6) drawStar(58, 2);
  drawMsg("yay!! :D");
  drawClock();
  display.display();
  if (el>=DUR_HAPPY) { petState=IDLE; lastAct=millis(); }
}

// ★ CONNECTED - Making Waves main interaction
void renderCONNECTED() {
  unsigned long el = millis()-stateAt;
  float p = (float)el/DUR_CONNECTED;
  int dy=0, eye=4;
  if (p < 0.25f) {
    eye=4; dy=(el<200)?-7:0;
  } else if (p < 0.65f) {
    eye=1;
    float pp=(p-0.25f)/0.4f;
    dy=(int)(-10.0f*sin(pp*PI*3));
  } else {
    eye=5; dy=breathDy();
  }
  display.clearDisplay();
  drawCat(CX, CY, eye, dy);

  // Animation for waves
  if (p>0.15f) {
    drawWave(6,  8);
    drawWave(6,  18);
    drawWave(100, 8);
    drawWave(100, 18);
  }
  if (p>0.4f) {
    drawHeart(18,  4, 5);
    drawHeart(108, 4, 5);
    drawWave(6,  28);
    drawWave(100, 28);
  }
  if (p>0.6f) {
    drawHeart(60, 2, 5);
    drawWave(40, 4);
    drawWave(74, 4);
  }

  if (p<0.25f)      drawMsg("wave detected!");
  else if (p<0.6f)  drawMsg("making waves~ :)");
  else              drawMsg("hi there! :3");

  drawClock();
  display.display();
  if (el>=DUR_CONNECTED) { petState=IDLE; lastAct=millis(); }
}

void renderEATING() {
  unsigned long el = millis()-stateAt;
  int phase=(el/280)%2, dy=phase?-2:0;
  display.clearDisplay();
  drawCat(CX, CY, 7, dy);
  drawFood(CX+30, CY+10);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(90, 38);
  display.print(phase?"nom!":"nom~");
  drawMsg("munch munch :3");
  drawClock();
  display.display();
  if (el>=DUR_EAT) { petState=IDLE; lastAct=millis(); }
}

void renderSINGING() {
  unsigned long el = millis()-stateAt;
  int phase=(el/380)%3, dy=(phase==1)?-4:0;
  display.clearDisplay();
  drawCat(CX, CY, 6, dy);
  drawNote(98, 16);
  if (phase>0) drawNote(10, 10);
  if (phase>1) drawNote(106, 26);
  const char* lyrics[]={"la~ la~ la~","meow~ meow~","sing with me!"};
  drawMsg(lyrics[phase]);
  drawClock();
  display.display();
  if (el>=DUR_SING) { petState=IDLE; lastAct=millis(); }
}

void renderHEART() {
  unsigned long el = millis()-stateAt;
  int ph=(el/320)%2;
  float p=(float)el/DUR_HEART;
  display.clearDisplay();
  drawCat(CX, CY, 5, breathDy());
  if (ph==0) { drawHeart(18,6,6); drawHeart(100,16,6); }
  else       { drawHeart(14,2,8); drawHeart(102,16,8); }
  if (p>0.5f) drawHeart(60,2,5);
  drawMsg("<3  love u  <3");
  drawClock();
  display.display();
  if (el>=DUR_HEART) { petState=IDLE; lastAct=millis(); }
}

void renderSURPRISED() {
  unsigned long el = millis()-stateAt;
  int dy=(el<200)?-6:0;
  display.clearDisplay();
  drawCat(CX, CY, 4, dy);
  display.setTextSize(2); display.setTextColor(SSD1306_WHITE);
  display.setCursor(84, 12); display.print("!");
  drawClock();
  display.display();
  if (el>=DUR_SURPRISED) { petState=HAPPY; stateAt=millis(); }
}

void renderSHY() {
  unsigned long el = millis()-stateAt;
  int ph=(el/400)%2;
  display.clearDisplay();
  drawCat(CX, CY, 8, breathDy());
  drawMsg(ph?"///(>///<)///":"s-so cute...");
  drawClock();
  display.display();
  if (el>=DUR_SHY) { petState=IDLE; lastAct=millis(); }
}

void renderPLAYING() {
  unsigned long el = millis()-stateAt;
  int frame=(el/180)%4;
  int dy[]={-4,-8,-4,0};
  display.clearDisplay();
  drawCat(CX, CY, 9, dy[frame]);
  display.fillCircle(CX+32, CY+10, 5, SSD1306_WHITE);
  display.drawCircle(CX+32, CY+10, 6, SSD1306_WHITE);
  display.drawLine(CX+26, CY+6, CX+16, CY-2, SSD1306_WHITE);
  const char* msgs[]={"wheee!!","boing!","play~","yay!"};
  drawMsg(msgs[frame]);
  drawClock();
  display.display();
  if (el>=DUR_PLAYING) { petState=IDLE; lastAct=millis(); }
}

void renderSLEEPING() {
  unsigned long el = millis()-stateAt;
  display.clearDisplay();
  drawCat(CX, CY, 3, breathDy());
  drawZZZ(el);
  drawMsg("zzzz...");
  drawClock();
  display.display();
}

// Buttons
void handleButton() {
  bool btnNow = digitalRead(BTN_PIN);
  if (btnPrev==HIGH && btnNow==LOW) btnDown=millis();
  if (btnPrev==LOW && btnNow==HIGH) {
    unsigned long held=millis()-btnDown;
    unsigned long sinceL=millis()-lastRelease;
    lastRelease=millis(); lastAct=millis();
    if (petState==SLEEPING) {
      petState=SURPRISED; stateAt=millis();
      vibrate(100); btnPrev=btnNow; return;
    }
    if (held>=LONG_PRESS_MS) {
      petState=CONNECTED; stateAt=millis();
      vibratePattern(3,80,60);
    } else if (sinceL<DOUBLE_PRESS_MS && waitDouble) {
      petState=HEART; stateAt=millis();
      waitDouble=false; vibratePattern(2,60,80);
    } else {
      waitDouble=true;
    }
  }
  if (waitDouble && (millis()-lastRelease)>=DOUBLE_PRESS_MS) {
    waitDouble=false;
    actionIndex=(actionIndex+1)%5;
    switch(actionIndex){
      case 0: petState=HAPPY;   break;
      case 1: petState=EATING;  break;
      case 2: petState=SINGING; break;
      case 3: petState=SHY;     break;
      case 4: petState=PLAYING; break;
    }
    stateAt=millis(); vibrate(70);
  }
  btnPrev=btnNow;
}

// Setup
void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(VIBRO_PIN, OUTPUT);
  digitalWrite(VIBRO_PIN, LOW);

  Wire.begin(6, 7);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (true) { delay(1000); }
  }

  mpu.initialize();
  mpu.setSleepEnabled(false);
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_8);

  int16_t gx, gy, gz;
  mpu.getMotion6(&prevAx, &prevAy, &prevAz, &gx, &gy, &gz);

  randomSeed(analogRead(A0));

  // Start screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20); display.print("Making Waves!");
  display.setCursor(22, 34); display.print("wave at me! :3");
  display.display();

  vibratePattern(2, 100, 80);
  delay(2000);

  lastAct = millis();
  stateAt = millis();
  clockStartMillis = millis();
}

// Loop
void loop() {
  detectWave();
  handleButton();

  if ((petState==IDLE) && (millis()-lastAct)>SLEEP_AFTER) {
    petState=SLEEPING; stateAt=millis();
  }

  switch (petState) {
    case IDLE:      renderIDLE();      break;
    case HAPPY:     renderHAPPY();     break;
    case CONNECTED: renderCONNECTED(); break;
    case EATING:    renderEATING();    break;
    case SINGING:   renderSINGING();   break;
    case HEART:     renderHEART();     break;
    case SURPRISED: renderSURPRISED(); break;
    case SLEEPING:  renderSLEEPING();  break;
    case SHY:       renderSHY();       break;
    case PLAYING:   renderPLAYING();   break;
  }

  delay(35);
}