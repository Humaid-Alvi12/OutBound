#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ILI9341 SPI Pin definitions for ESP32-S3
#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  -1  
#define TFT_MOSI 11
#define TFT_SCK  12
#define TFT_MISO 13

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Input Pins
#define ENC_CLK    4   // Rotary Encoder Clock (Roll only)
#define ENC_DT     5   // Rotary Encoder Data
#define BTN_SELECT 2   // Pin 2: Select / Open App / Dino Jump / Space Shoot
#define BTN_EXTRA  3   // Pin 3: Dino Duck / Space Move Right
#define BTN_SKIP   7   // Skip Song / Video Record / Delete Photo Button
#define BTN_BACK   8   // Dedicated Back to Menu Button

// App States
enum AppState { 
  STATE_MENU, 
  STATE_MUSIC, 
  STATE_CAMERA, 
  STATE_PHOTOS, 
  STATE_VIEW_PHOTO, 
  STATE_GAMES_MENU, 
  STATE_DINO_RUN, 
  STATE_SPACE_RUNNER 
};
AppState currentAppState = STATE_MENU;

// Main Menu Setup
int currentMenu = 0;
const int totalMenus = 4;
const char* menuItems[totalMenus] = {"Music", "Camera", "Photos", "Games"};
int lastStateCLK;
unsigned long lastButtonPress = 0;
int previousMenu = 0;

// Games Submenu Setup
int currentGameMenu = 0;
const int totalGames = 2;
const char* gameItems[totalGames] = {"Dino Run", "Space Runner"};
int previousGameMenu = 0;

// Music App Variables
int currentSong = 0;
const int totalSongs = 3;
const char* songTitles[totalSongs] = {"Cyberdeck Lofi", "Synthwave Runner", "Neon Highway"};
int songDuration = 210;    
int currentProgress = 45;  
bool isPlaying = true;

// Camera App Variables
float zoomLevel = 1.0;     
bool isRecording = false;
unsigned long videoStartTime = 0;

// Photos App Variables
int currentPhotoIndex = 0;
const int totalPhotos = 3;
const char* photoNames[totalPhotos] = {"IMG_001.JPG", "IMG_002.JPG", "IMG_003.JPG"};
bool photoDeleted[totalPhotos] = {false, false, false};
bool deleteConfirmActive = false;

// ==================== GAME VARIABLES ====================
// Dino Run Variables
int dinoY = 165;
bool isJumping = false;
bool isDucking = false;
int dinoVy = 0;
int dinoScore = 0;
int obstacleX = 320;
bool dinoGameOver = false;
unsigned long lastDinoTick = 0;

// Space Runner Variables
int playerShipX = 140;
int enemyX = 160;
int enemyY = 40;
int bulletX = -10;
int bulletY = -10;
bool bulletActive = false;
int spaceScore = 0;
int spaceLives = 3;
bool spaceGameOver = false;
unsigned long lastSpaceTick = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting OutBound OS...");

  // Initialize SPI for ESP32-S3
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);

  // Initialize all input pins with internal pull-ups
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_EXTRA, INPUT_PULLUP);
  pinMode(BTN_SKIP, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  lastStateCLK = digitalRead(ENC_CLK);

  // Initialize Display
  tft.begin();
  tft.setRotation(1); // Landscape (320x240)
  tft.fillScreen(ILI9341_BLACK);

  // Boot Animation & Menu Load
  fadeInLogo();
  drawCreamMenu();
}

void loop() {
  // Global Home Button handler (GPIO 8) returns to main menu from anywhere
  if (digitalRead(BTN_BACK) == LOW) {
    if (millis() - lastButtonPress > 300) {
      isRecording = false;
      currentAppState = STATE_MENU;
      drawCreamMenu();
      lastButtonPress = millis();
      return;
    }
  }

  switch (currentAppState) {
    case STATE_MENU:
      handleMenuInput();
      break;
    case STATE_MUSIC:
      handleMusicInput();
      break;
    case STATE_CAMERA:
      handleCameraInput();
      break;
    case STATE_PHOTOS:
      handlePhotosInput();
      break;
    case STATE_VIEW_PHOTO:
      handleViewPhotoInput();
      break;
    case STATE_GAMES_MENU:
      handleGamesMenuInput();
      break;
    case STATE_DINO_RUN:
      handleDinoRunInput();
      break;
    case STATE_SPACE_RUNNER:
      handleSpaceRunnerInput();
      break;
  }
}

// ==================== MAIN MENU LOGIC ====================
void handleMenuInput() {
  int currentStateCLK = digitalRead(ENC_CLK);
  
  if (currentStateCLK != lastStateCLK) {
    if (currentStateCLK == LOW) {
      previousMenu = currentMenu; 
      
      if (digitalRead(ENC_DT) == HIGH) {
        currentMenu = (currentMenu + 1) % totalMenus; 
      } else {
        currentMenu = (currentMenu - 1 + totalMenus) % totalMenus; 
      }
      
      if (currentMenu != previousMenu) {
        updateMenuCursor(previousMenu, currentMenu);
      }
    }
    lastStateCLK = currentStateCLK;
  }

  // Pin 2 opens selected main menu app
  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastButtonPress > 300) {
      executeAction();
      lastButtonPress = millis();
    }
  }
}

void executeAction() {
  if (currentMenu == 0) {
    currentAppState = STATE_MUSIC;
    drawMusicApp();
  } else if (currentMenu == 1) {
    currentAppState = STATE_CAMERA;
    drawCameraApp();
  } else if (currentMenu == 2) {
    currentAppState = STATE_PHOTOS;
    drawPhotosApp();
  } else if (currentMenu == 3) {
    currentAppState = STATE_GAMES_MENU;
    drawGamesMenu();
  }
}

// ==================== GAMES SUBMENU LOGIC ====================
void drawGamesMenu() {
  uint16_t creamBg     = tft.color565(255, 252, 245);
  uint16_t headerColor = tft.color565(40, 40, 60);

  tft.fillScreen(creamBg);
  tft.fillRect(0, 0, 320, 35, headerColor);
  tft.setTextColor(creamBg);
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.print("OUTBOUND // GAMES");

  for (int i = 0; i < totalGames; i++) {
    drawGameMenuItem(i, (i == currentGameMenu));
  }

  tft.setTextSize(1);
  tft.setTextColor(tft.color565(100, 100, 100));
  tft.setCursor(20, 215);
  tft.print("Roll: Nav | Pin 2: Select Game | Back: Menu");
}

void drawGameMenuItem(int index, bool isSelected) {
  uint16_t creamBg     = tft.color565(255, 252, 245);
  uint16_t textColor   = tft.color565(20, 20, 20);
  uint16_t selectColor = tft.color565(0, 100, 200); 

  int yPos = 75 + (index * 45);
  tft.fillRect(15, yPos - 2, 290, 35, creamBg);

  tft.setTextSize(2);
  if (isSelected) {
    tft.fillRect(15, yPos - 2, 290, 35, tft.color565(235, 225, 205));
    tft.setTextColor(selectColor);
    tft.setCursor(20, yPos + 6);
    tft.print("> ");
    tft.print(gameItems[index]);
  } else {
    tft.setTextColor(textColor);
    tft.setCursor(20, yPos + 6);
    tft.print("  ");
    tft.print(gameItems[index]);
  }
}

void updateGameMenuCursor(int oldIndex, int newIndex) {
  drawGameMenuItem(oldIndex, false);
  drawGameMenuItem(newIndex, true);
}

void handleGamesMenuInput() {
  int currentStateCLK = digitalRead(ENC_CLK);
  
  if (currentStateCLK != lastStateCLK) {
    if (currentStateCLK == LOW) {
      previousGameMenu = currentGameMenu; 
      
      if (digitalRead(ENC_DT) == HIGH) {
        currentGameMenu = (currentGameMenu + 1) % totalGames; 
      } else {
        currentGameMenu = (currentGameMenu - 1 + totalGames) % totalGames; 
      }
      
      if (currentGameMenu != previousGameMenu) {
        updateGameMenuCursor(previousGameMenu, currentGameMenu);
      }
    }
    lastStateCLK = currentStateCLK;
  }

  // Pin 2 opens selected game
  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastButtonPress > 300) {
      if (currentGameMenu == 0) {
        currentAppState = STATE_DINO_RUN;
        initDinoGame();
        drawDinoUI();
      } else {
        currentAppState = STATE_SPACE_RUNNER;
        initSpaceGame();
        drawSpaceUI();
      }
      lastButtonPress = millis();
    }
  }
}

// ==================== GAME 1: DINO RUN ====================
void initDinoGame() {
  dinoY = 165;
  isJumping = false;
  isDucking = false;
  dinoVy = 0;
  dinoScore = 0;
  obstacleX = 320;
  dinoGameOver = false;
}

void drawDinoUI() {
  uint16_t darkBg = tft.color565(20, 20, 30);
  tft.fillScreen(darkBg);

  // Header
  tft.fillRect(0, 0, 320, 30, tft.color565(40, 40, 60));
  tft.setTextColor(tft.color565(0, 255, 150));
  tft.setTextSize(2);
  tft.setCursor(15, 8);
  tft.print("DINO RUN");

  // Ground line
  tft.drawFastHLine(10, 185, 300, tft.color565(200, 200, 200));

  updateDinoGraphics();
}

void updateDinoGraphics() {
  uint16_t darkBg = tft.color565(20, 20, 30);
  // Clear game area
  tft.fillRect(10, 35, 300, 148, darkBg);

  // Score HUD
  tft.setTextSize(1);
  tft.setTextColor(tft.color565(255, 255, 255));
  tft.setCursor(220, 10);
  tft.print("SCORE: ");
  tft.print(dinoScore);

  if (!dinoGameOver) {
    // Draw Dinosaur (box or ducking rect)
    uint16_t dinoColor = tft.color565(50, 200, 255);
    if (isDucking) {
      tft.fillRect(40, dinoY + 12, 28, 16, dinoColor); // Ducking shape
    } else {
      tft.fillRect(40, dinoY, 20, 28, dinoColor);      // Standing shape
    }

    // Draw Obstacle (Cactus)
    tft.fillRect(obstacleX, 160, 14, 25, tft.color565(255, 80, 80));
  } else {
    tft.setTextColor(tft.color565(255, 50, 50));
    tft.setTextSize(2);
    tft.setCursor(95, 90);
    tft.print("GAME OVER");

    tft.setTextColor(tft.color565(200, 200, 200));
    tft.setTextSize(1);
    tft.setCursor(75, 125);
    tft.print("Press Pin 2 to Restart");
  }

  // Footer
  tft.fillRect(0, 210, 320, 30, darkBg);
  tft.setCursor(15, 215);
  tft.setTextColor(tft.color565(150, 150, 150));
  tft.print("Pin 2: Jump | Pin 3: Duck | Home: Menu");
}

void handleDinoRunInput() {
  // Pin 2 for Jump
  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastButtonPress > 250) {
      if (dinoGameOver) {
        initDinoGame();
        drawDinoUI();
      } else if (!isJumping && !isDucking) {
        isJumping = true;
        dinoVy = -11; // Jump velocity
      }
      lastButtonPress = millis();
    }
  }

  // Pin 3 for Duck
  if (digitalRead(BTN_EXTRA) == LOW) {
    if (!isJumping) {
      isDucking = true;
    }
  } else {
    isDucking = false;
  }

  // Game Engine Loop
  if (!dinoGameOver && millis() - lastDinoTick > 30) {
    lastDinoTick = millis();

    // Jump physics
    if (isJumping) {
      dinoY += dinoVy;
      dinoVy += 1; // Gravity
      if (dinoY >= 165) {
        dinoY = 165;
        isJumping = false;
      }
    }

    // Move obstacle
    obstacleX -= 6;
    if (obstacleX < 10) {
      obstacleX = 310;
      dinoScore += 10;
    }

    // Collision detection
    int dinoH = isDucking ? 16 : 28;
    int dinoCurrentY = isDucking ? dinoY + 12 : dinoY;
    if (obstacleX <= 60 && obstacleX >= 30 && dinoCurrentY + dinoH >= 160) {
      dinoGameOver = true;
    }

    updateDinoGraphics();
  }
}

// ==================== GAME 2: SPACE RUNNER ====================
void initSpaceGame() {
  playerShipX = 140;
  enemyX = random(30, 260);
  enemyY = 45;
  bulletActive = false;
  spaceScore = 0;
  spaceLives = 3;
  spaceGameOver = false;
}

void drawSpaceUI() {
  uint16_t spaceBg = tft.color565(10, 10, 20);
  tft.fillScreen(spaceBg);

  // Header
  tft.fillRect(0, 0, 320, 30, tft.color565(30, 30, 50));
  tft.setTextColor(tft.color565(255, 100, 255));
  tft.setTextSize(2);
  tft.setCursor(15, 8);
  tft.print("SPACE RUNNER");

  updateSpaceGraphics();
}

void updateSpaceGraphics() {
  uint16_t spaceBg = tft.color565(10, 10, 20);
  tft.fillRect(10, 35, 300, 170, spaceBg);

  // HUD
  tft.setTextSize(1);
  tft.setTextColor(tft.color565(255, 255, 255));
  tft.setCursor(180, 10);
  tft.print("SCR:");
  tft.print(spaceScore);
  tft.print(" LIV:");
  tft.print(spaceLives);

  if (!spaceGameOver) {
    // Draw Enemy
    tft.fillRect(enemyX, enemyY, 16, 12, tft.color565(255, 50, 50));

    // Draw Laser Bullet
    if (bulletActive) {
      tft.fillRect(bulletX, bulletY, 3, 10, tft.color565(0, 255, 255));
    }

    // Draw Player Ship (Triangle/Box at bottom)
    tft.fillRect(playerShipX, 190, 24, 10, tft.color565(50, 255, 100));
    tft.fillRect(playerShipX + 10, 184, 4, 6, tft.color565(50, 255, 100));
  } else {
    tft.setTextColor(tft.color565(255, 50, 50));
    tft.setTextSize(2);
    tft.setCursor(95, 90);
    tft.print("GAME OVER");

    tft.setTextColor(tft.color565(200, 200, 200));
    tft.setTextSize(1);
    tft.setCursor(75, 125);
    tft.print("Press Pin 2 to Restart");
  }

  // Footer
  tft.fillRect(0, 210, 320, 30, spaceBg);
  tft.setCursor(15, 215);
  tft.setTextColor(tft.color565(150, 150, 150));
  tft.print("Pin 3: Move Right (Wrap) | Pin 2: Shoot");
}

void handleSpaceRunnerInput() {
  // Pin 3: Move Right (when hits corner, wraps around to far left)
  if (digitalRead(BTN_EXTRA) == LOW) {
    if (millis() - lastButtonPress > 150) {
      playerShipX += 16;
      if (playerShipX > 290) {
        playerShipX = 15; // Wrap to far left corner!
      }
      lastButtonPress = millis();
    }
  }

  // Pin 2: Shoot Laser
  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastButtonPress > 250) {
      if (spaceGameOver) {
        initSpaceGame();
        drawSpaceUI();
      } else if (!bulletActive) {
        bulletActive = true;
        bulletX = playerShipX + 10;
        bulletY = 180;
      }
      lastButtonPress = millis();
    }
  }

  // Game Engine Loop
  if (!spaceGameOver && millis() - lastSpaceTick > 35) {
    lastSpaceTick = millis();

    // Move enemy down
    enemyY += 3;
    if (enemyY > 200) {
      spaceLives--;
      enemyY = 40;
      enemyX = random(20, 270);
      if (spaceLives <= 0) spaceGameOver = true;
    }

    // Move bullet up
    if (bulletActive) {
      bulletY -= 10;
      if (bulletY < 35) {
        bulletActive = false;
      }
      // Check bullet hit enemy
      if (bulletActive && bulletX >= enemyX - 2 && bulletX <= enemyX + 18 && bulletY <= enemyY + 12) {
        spaceScore += 20;
        bulletActive = false;
        enemyY = 40;
        enemyX = random(20, 270);
      }
    }

    updateSpaceGraphics();
  }
}

// ==================== GRAPHICS & UI ====================
void fadeInLogo() {
  for (int b = 20; b <= 255; b += 25) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(55, 100);
    tft.setTextSize(3);
    tft.setTextColor(tft.color565(0, 0, b));
    tft.print("Out");
    tft.setTextColor(tft.color565(0, b, 0));
    tft.print("Bound");
    delay(60);
  }
  delay(1200);
}

void drawCreamMenu() {
  uint16_t creamColor  = tft.color565(255, 250, 220);
  uint16_t headerColor = tft.color565(40, 40, 60);

  tft.fillScreen(creamColor);
  
  tft.fillRect(0, 0, 320, 35, headerColor);
  tft.setTextColor(creamColor);
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.print("OUTBOUND // SYSTEM");

  for (int i = 0; i < totalMenus; i++) {
    drawMenuItem(i, (i == currentMenu));
  }

  tft.setTextSize(1);
  tft.setTextColor(tft.color565(100, 100, 100));
  tft.setCursor(20, 215);
  tft.print("Roll: Navigate | Pin 2: Open App");
}

void drawMenuItem(int index, bool isSelected) {
  uint16_t creamColor  = tft.color565(255, 250, 220);
  uint16_t textColor   = tft.color565(20, 20, 20);
  uint16_t selectColor = tft.color565(0, 100, 200); 

  int yPos = 60 + (index * 38);
  tft.fillRect(15, yPos - 2, 290, 30, creamColor);

  tft.setTextSize(2);
  if (isSelected) {
    tft.setTextColor(selectColor);
    tft.setCursor(20, yPos);
    tft.print("> ");
    tft.print(menuItems[index]);
  } else {
    tft.setTextColor(textColor);
    tft.setCursor(20, yPos);
    tft.print("  ");
    tft.print(menuItems[index]);
  }
}

void updateMenuCursor(int oldIndex, int newIndex) {
  drawMenuItem(oldIndex, false);
  drawMenuItem(newIndex, true);
}

// ==================== PHOTOS APP UI (White & Cream Theme) ====================
void drawPhotosApp() {
  uint16_t creamBg     = tft.color565(255, 252, 245); 
  uint16_t headerColor = tft.color565(60, 60, 70);
  uint16_t textColor   = tft.color565(30, 30, 30);
  uint16_t selectColor = tft.color565(0, 120, 215);
  uint16_t subText     = tft.color565(120, 120, 120);
  uint16_t warnRed     = tft.color565(220, 50, 50);

  tft.fillScreen(creamBg);

  tft.fillRect(0, 0, 320, 35, headerColor);
  tft.setTextColor(creamBg);
  tft.setTextSize(2);
  tft.setCursor(15, 10);
  tft.print("OUTBOUND // PHOTOS");

  tft.setTextSize(2);
  for (int i = 0; i < totalPhotos; i++) {
    int yPos = 65 + (i * 40);
    
    if (i == currentPhotoIndex) {
      tft.fillRect(15, yPos - 4, 290, 32, tft.color565(235, 225, 205));
      tft.setTextColor(selectColor);
      tft.setCursor(20, yPos);
      tft.print("> ");
    } else {
      tft.setTextColor(textColor);
      tft.setCursor(20, yPos);
      tft.print("  ");
    }

    tft.print(photoNames[i]);

    tft.setTextSize(1);
    tft.setCursor(180, yPos + 6);
    if (photoDeleted[i]) {
      tft.setTextColor(subText);
      tft.print("[ EMPTY SLOT ]");
    } else {
      tft.setTextColor(subText);
      tft.print("SD CARD (JPG)");
    }
    tft.setTextSize(2);
  }

  tft.fillRect(0, 205, 320, 35, tft.color565(245, 238, 225));
  tft.setTextSize(1);
  
  if (deleteConfirmActive) {
    tft.setTextColor(warnRed);
    tft.setCursor(20, 216);
    tft.print("PRESS SKIP BUTTON AGAIN TO CONFIRM!");
  } else {
    tft.setTextColor(textColor);
    tft.setCursor(20, 216);
    tft.print("Roll: Nav | Pin 2: View | Skip: Del | Back: Menu");
  }
}

void drawViewPhoto() {
  uint16_t creamBg   = tft.color565(255, 252, 245);
  uint16_t frameClr  = tft.color565(60, 60, 70);
  uint16_t textColor = tft.color565(30, 30, 30);

  tft.fillScreen(creamBg);

  tft.setTextColor(textColor);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.print(photoNames[currentPhotoIndex]);

  tft.drawRect(40, 45, 240, 150, frameClr);
  tft.fillRect(42, 47, 236, 146, tft.color565(240, 235, 220));

  tft.setTextColor(tft.color565(150, 150, 150));
  tft.setTextSize(2);
  tft.setCursor(85, 110);
  tft.print("[ BLANK IMAGE ]");

  tft.setTextSize(1);
  tft.setTextColor(textColor);
  tft.setCursor(65, 212);
  tft.print("Press Pin 2 or Back to return to gallery");
}

void handlePhotosInput() {
  int currentStateCLK = digitalRead(ENC_CLK);
  if (currentStateCLK != lastStateCLK) {
    if (currentStateCLK == LOW) {
      deleteConfirmActive = false; 
      if (digitalRead(ENC_DT) == HIGH) {
        currentPhotoIndex = (currentPhotoIndex + 1) % totalPhotos;
      } else {
        currentPhotoIndex = (currentPhotoIndex - 1 + totalPhotos) % totalPhotos;
      }
      drawPhotosApp();
    }
    lastStateCLK = currentStateCLK;
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastButtonPress > 300) {
      if (!photoDeleted[currentPhotoIndex]) {
        currentAppState = STATE_VIEW_PHOTO;
        drawViewPhoto();
      }
      lastButtonPress = millis();
    }
  }

  if (digitalRead(BTN_SKIP) == LOW) {
    if (millis() - lastButtonPress > 300) {
      if (!photoDeleted[currentPhotoIndex]) {
        if (!deleteConfirmActive) {
          deleteConfirmActive = true;
          drawPhotosApp(); 
        } else {
          photoDeleted[currentPhotoIndex] = true;
          deleteConfirmActive = false;
          drawPhotosApp();
        }
      }
      lastButtonPress = millis();
    }
  }
}

void handleViewPhotoInput() {
  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastButtonPress > 300) {
      currentAppState = STATE_PHOTOS;
      drawPhotosApp();
      lastButtonPress = millis();
    }
  }
}

// ==================== CAMERA APP UI ====================
void drawCameraApp() {
  uint16_t camBg       = tft.color565(20, 20, 20);
  uint16_t camYellow   = tft.color565(255, 200, 0);

  tft.fillScreen(camBg);

  tft.setTextColor(camYellow);
  tft.setTextSize(2);
  tft.setCursor(15, 15);
  tft.print("OUTBOUND CAM");

  tft.drawCircle(285, 23, 10, camYellow);
  tft.fillCircle(285, 23, 4, camYellow);

  tft.drawFastHLine(15, 45, 290, tft.color565(60, 60, 60));

  tft.drawRect(20, 55, 280, 115, tft.color565(80, 80, 80));
  tft.drawFastHLine(145, 112, 30, tft.color565(100, 100, 100));
  tft.drawFastVLine(160, 97, 30, tft.color565(100, 100, 100));

  updateCameraUI();
}

void updateCameraUI() {
  uint16_t camBg     = tft.color565(20, 20, 20);
  uint16_t accentClr = tft.color565(0, 220, 255);
  uint16_t recRed    = tft.color565(255, 50, 50);
  uint16_t subText   = tft.color565(180, 180, 180);

  tft.fillRect(25, 140, 270, 25, camBg);

  tft.setTextColor(accentClr);
  tft.setTextSize(1);
  tft.setCursor(30, 148);
  tft.print("ZOOM: ");
  tft.print(zoomLevel, 1);
  tft.print("x");

  if (isRecording) {
    unsigned long elapsedSecs = (millis() - videoStartTime) / 1000;
    int mins = elapsedSecs / 60;
    int secs = elapsedSecs % 60;

    if ((millis() / 500) % 2 == 0) {
      tft.fillCircle(200, 152, 4, recRed);
    } else {
      tft.fillCircle(200, 152, 4, camBg);
    }

    tft.setTextColor(recRed);
    tft.setCursor(212, 148);
    tft.print("REC ");
    if (mins < 10) tft.print("0");
    tft.print(mins);
    tft.print(":");
    if (secs < 10) tft.print("0");
    tft.print(secs);
  } else {
    tft.setTextColor(subText);
    tft.setCursor(200, 148);
    tft.print("[ READY ]");
  }

  tft.fillRect(15, 200, 290, 25, camBg);
  tft.setTextColor(subText);
  tft.setTextSize(1);
  tft.setCursor(15, 212);
  tft.print("Roll: Zoom | Pin 2: Photo | Skip: Rec | Back: Menu");
}

void triggerCameraFlash() {
  tft.fillScreen(ILI9341_WHITE);
  delay(100);
  drawCameraApp();
  
  tft.fillRect(60, 100, 200, 40, tft.color565(40, 40, 40));
  tft.drawRect(60, 100, 200, 40, tft.color565(0, 220, 255));
  tft.setTextColor(tft.color565(255, 255, 255));
  tft.setTextSize(2);
  tft.setCursor(75, 112);
  tft.print("PHOTO SAVED!");
  delay(800);
  updateCameraUI();
}

void handleCameraInput() {
  int currentStateCLK = digitalRead(ENC_CLK);
  if (currentStateCLK != lastStateCLK) {
    if (currentStateCLK == LOW) {
      if (digitalRead(ENC_DT) == HIGH) {
        zoomLevel = min(5.0, zoomLevel + 0.2);
      } else {
        zoomLevel = max(1.0, zoomLevel - 0.2);
      }
      updateCameraUI();
    }
    lastStateCLK = currentStateCLK;
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastButtonPress > 300) {
      triggerCameraFlash();
      lastButtonPress = millis();
    }
  }

  if (digitalRead(BTN_SKIP) == LOW) {
    if (millis() - lastButtonPress > 300) {
      isRecording = !isRecording;
      if (isRecording) videoStartTime = millis();
      updateCameraUI();
      lastButtonPress = millis();
    }
  }

  if (isRecording) {
    updateCameraUI();
    delay(200); 
  }
}

// ==================== MUSIC APP UI ====================
void drawMusicApp() {
  uint16_t darkBg       = tft.color565(24, 24, 24);   
  uint16_t spotifyGreen = tft.color565(30, 215, 96);  
  uint16_t textColor    = tft.color565(255, 255, 255); 
  uint16_t subText      = tft.color565(160, 160, 160); 

  tft.fillScreen(darkBg);

  tft.setTextColor(spotifyGreen);
  tft.setTextSize(2);
  tft.setCursor(15, 15);
  tft.print("OUTBOUND MUSIC");

  tft.fillRect(275, 15, 4, 16, spotifyGreen);
  tft.fillCircle(273, 31, 4, spotifyGreen);
  tft.fillCircle(281, 31, 4, spotifyGreen);
  tft.fillRect(279, 12, 8, 4, spotifyGreen);

  tft.drawFastHLine(15, 45, 290, tft.color565(60, 60, 60));

  tft.setTextColor(textColor);
  tft.setTextSize(2);
  tft.setCursor(20, 75);
  tft.print(songTitles[currentSong]);

  tft.setTextColor(subText);
  tft.setTextSize(1);
  tft.setCursor(20, 100);
  tft.print("Virtual SD Track #0");
  tft.print(currentSong + 1);

  drawPlaybackStatus(20, 130);

  int barX = 20;
  int barY = 175;
  int barW = 280;
  int barH = 10;
  
  tft.drawRect(barX, barY, barW, barH, subText);
  int fillWidth = (currentProgress * (barW - 2)) / songDuration;
  tft.fillRect(barX + 1, barY + 1, fillWidth, barH - 2, spotifyGreen);

  tft.setTextSize(1);
  tft.setTextColor(subText);
  tft.setCursor(barX, barY + 14);
  tft.print(currentProgress / 60);
  tft.print(":");
  if ((currentProgress % 60) < 10) tft.print("0");
  tft.print(currentProgress % 60);

  tft.setCursor(barX + barW - 30, barY + 14);
  tft.print(songDuration / 60);
  tft.print(":");
  if ((songDuration % 60) < 10) tft.print("0");
  tft.print(songDuration % 60);

  tft.setCursor(15, 215);
  tft.print("Roll: Scrub | Pin 2: Play/Pause | Skip: Next");
}

void updateMusicUI() {
  uint16_t darkBg       = tft.color565(24, 24, 24);
  uint16_t spotifyGreen = tft.color565(30, 215, 96);
  uint16_t textColor    = tft.color565(255, 255, 255);
  uint16_t subText      = tft.color565(160, 160, 160);

  tft.fillRect(20, 70, 280, 80, darkBg);

  tft.setTextColor(textColor);
  tft.setTextSize(2);
  tft.setCursor(20, 75);
  tft.print(songTitles[currentSong]);

  tft.setTextColor(subText);
  tft.setTextSize(1);
  tft.setCursor(20, 100);
  tft.print("Virtual SD Track #0");
  tft.print(currentSong + 1);

  drawPlaybackStatus(20, 130);

  int barX = 20;
  int barY = 175;
  int barW = 280;
  int barH = 10;
  
  tft.fillRect(barX + 1, barY + 1, barW - 2, barH - 2, darkBg);
  int fillWidth = (currentProgress * (barW - 2)) / songDuration;
  tft.fillRect(barX + 1, barY + 1, fillWidth, barH - 2, spotifyGreen);

  tft.fillRect(barX, barY + 14, 80, 12, darkBg);
  tft.setTextSize(1);
  tft.setTextColor(subText);
  tft.setCursor(barX, barY + 14);
  tft.print(currentProgress / 60);
  tft.print(":");
  if ((currentProgress % 60) < 10) tft.print("0");
  tft.print(currentProgress % 60);

  tft.setCursor(barX + barW - 30, barY + 14);
  tft.print(songDuration / 60);
  tft.print(":");
  if ((songDuration % 60) < 10) tft.print("0");
  tft.print(songDuration % 60);
}

void handleMusicInput() {
  int currentStateCLK = digitalRead(ENC_CLK);
  if (currentStateCLK != lastStateCLK) {
    if (currentStateCLK == LOW) {
      if (digitalRead(ENC_DT) == HIGH) {
        currentProgress = min(songDuration, currentProgress + 5);
      } else {
        currentProgress = max(0, currentProgress - 5);
      }
      updateMusicUI();
    }
    lastStateCLK = currentStateCLK;
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastButtonPress > 300) {
      isPlaying = !isPlaying;
      updateMusicUI();
      lastButtonPress = millis();
    }
  }

  if (digitalRead(BTN_SKIP) == LOW) {
    if (millis() - lastButtonPress > 300) {
      currentSong = (currentSong + 1) % totalSongs;
      currentProgress = 0;
      updateMusicUI();
      lastButtonPress = millis();
    }
  }
}

void drawPlaybackStatus(int x, int y) {
  uint16_t spotifyGreen = tft.color565(30, 215, 96);
  uint16_t pauseRed     = tft.color565(220, 80, 80);

  if (isPlaying) {
    tft.fillTriangle(x, y, x, y + 16, x + 14, y + 8, spotifyGreen);
    tft.setTextColor(spotifyGreen);
    tft.setTextSize(2);
    tft.setCursor(x + 24, y);
    tft.print("PLAYING");
  } else {
    tft.fillRect(x, y, 4, 16, pauseRed);
    tft.fillRect(x + 7, y, 4, 16, pauseRed);
    tft.setTextColor(pauseRed);
    tft.setTextSize(2);
    tft.setCursor(x + 24, y);
    tft.print("PAUSED");
  }
}
