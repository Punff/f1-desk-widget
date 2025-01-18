#include "Formula1_Regular_112pt7b.h"
#include "Formula1_Regular_18pt7b.h"

#define MAX_IMAGE_WIDTH 320
#define TFT_HOR_RES 480
#define TFT_VER_RES 320

enum View {
  MENU_VIEW,
  DRIVER_VIEW,
  NEWS_VIEW,
  DRIVER_STANDINGS_VIEW,
  CONSTRUCTOR_STANDINGS_VIEW,
};

const char* menuViewNames[] = {
  "Drivers",
  "News",
  "Driver Standings",
  "Constructor Standings",
};

View currentView = MENU_VIEW;

extern bool turnedRight;
extern bool turnedLeft;
extern bool buttonPressedShort;
extern bool buttonPressedLong;

int16_t xpos = 0;
int16_t ypos = 0;
PNG png;

extern Driver* DRIVERS;
extern int driverNum;

extern Team* TEAMS;
extern int teamNum;

extern Article articles[MAX_ARTICLES];

bool displayDriverInfo = false;
bool allowInfo = true;
int driverIndex = 0;
int newsIndex = 0;
int menuIndex = 0;
int menuIndexNext = 1;
int menuIndexPrev = 6;

TFT_eSPI tft = TFT_eSPI();

void setupDisplay() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&Formula1_Regular_112pt7b);
}

void handleDisplay() {
  if (buttonPressedLong) {
    currentView = MENU_VIEW;
  }

  if (buttonPressedShort && currentView == DRIVER_VIEW && allowInfo) {
    displayDriverInfo = !displayDriverInfo;
  } else {
    displayDriverInfo = false;
  }


  if (turnedLeft || buttonPressedLong || turnedRight || buttonPressedShort) {
    switch (currentView) {
      case MENU_VIEW:
        Serial.println("Menu view selected.");
        handleMenu();
        break;

      case DRIVER_VIEW:
        Serial.println("Driver view selected.");
        if (displayDriverInfo) {
          handleDriverInfo();
        } else {
          handleDriverView();
          allowInfo = true;
        }
        break;

      case NEWS_VIEW:
        Serial.println("News view selected.");

        handleNews();
        break;

      case DRIVER_STANDINGS_VIEW:
        Serial.println("Driver standings view selected.");
        handleDriverStandingsView();
        break;

      case CONSTRUCTOR_STANDINGS_VIEW:
        Serial.println("Constructor standings view selected.");
        handleConstructorStandingsView();
        break;

      default:
        Serial.println("Unknown view.");
        tft.fillScreen(TFT_BLACK);
        break;
    }
    turnedLeft = false;
    turnedRight = false;
  }
}

void handleMenu() {
  buttonPressedLong = false;
  tft.fillScreen(TFT_BLACK);

  if (turnedRight) {
    menuIndex = (menuIndex + 1) % 4;
  } else if (turnedLeft) {
    menuIndex = (menuIndex - 1 + 4) % 4;
  }
  menuIndexNext = (menuIndex + 1) % 4;
  menuIndexPrev = (menuIndex - 1 + 4) % 4;

  const uint16_t F1_RED = 0xF800;
  const uint16_t F1_GREY = 0x8410;
  const uint16_t TIMING_PURPLE = 0x780F;
  const uint16_t SIGNAL_YELLOW = 0xFFE0;

  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(F1_GREY);
  tft.drawString(menuViewNames[menuIndexPrev], TFT_HOR_RES / 2, TFT_VER_RES / 2 - 30);

  tft.setTextSize(1);
  int16_t selectedWidth = tft.textWidth(menuViewNames[menuIndex]);
  int16_t x = TFT_HOR_RES / 2;
  int16_t y = TFT_VER_RES / 2;

  tft.setTextColor(F1_RED);
  tft.drawString("[", x - selectedWidth / 2 - 20, y);
  tft.drawString("]", x + selectedWidth / 2 + 20, y);
  tft.setTextColor(SIGNAL_YELLOW);
  tft.drawString(menuViewNames[menuIndex], x, y);

  tft.setTextSize(1);
  tft.setTextColor(F1_GREY);
  tft.drawString(menuViewNames[menuIndexNext], TFT_HOR_RES / 2, TFT_VER_RES / 2 + 30);

  if (buttonPressedShort) {
    currentView = static_cast<View>(menuIndex + 1);
    allowInfo = false;
  }
}

void handleNews() {
  buttonPressedShort = false;
  if (turnedRight) {
    newsIndex = (newsIndex + 1) % MAX_ARTICLES;
    turnedRight = false;
  } else if (turnedLeft) {
    newsIndex = (newsIndex - 1 + MAX_ARTICLES) % MAX_ARTICLES;
    turnedLeft = false;
  }

  tft.fillScreen(TFT_BLACK);
  Article selected = articles[newsIndex];

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  tft.fillRect(0, 0, TFT_HOR_RES, 95, 0x3186);
  tft.drawFastHLine(0, 95, TFT_HOR_RES, 0xF800);
  tft.drawFastHLine(0, 96, TFT_HOR_RES, 0xF800);

  const int screenWidth = tft.width() - 20;
  const int lineHeight = tft.fontHeight() - 6;
  int cursorY = 30;
  int cursorX = 10;

  char* text = strdup(selected.title);
  char* word = strtok(text, " ");
  String currentLine = "";

  while (word) {
    String tempLine = currentLine + (currentLine.isEmpty() ? "" : " ") + word;
    int lineWidth = tft.textWidth(tempLine.c_str());

    if (lineWidth > screenWidth) {
      tft.setCursor(cursorX, cursorY);
      tft.print(currentLine);
      cursorY += lineHeight;

      if (cursorY + lineHeight > tft.height()) {
        break;
      }

      currentLine = word;
    } else {
      currentLine = tempLine;
    }

    word = strtok(nullptr, " ");
  }

  if (!currentLine.isEmpty() && cursorY <= tft.height() - lineHeight) {
    tft.setCursor(cursorX, cursorY);
    tft.print(currentLine);
  }

  free(text);

  tft.setFreeFont(&Formula1_Regular_18pt7b);

  text = strdup(selected.description);
  word = strtok(text, " ");
  currentLine = "";
  cursorY = 120;

  while (word) {
    String tempLine = currentLine + (currentLine.isEmpty() ? "" : " ") + word;
    int lineWidth = tft.textWidth(tempLine.c_str());

    if (lineWidth > screenWidth) {
      tft.setCursor(cursorX, cursorY);
      tft.print(currentLine);
      cursorY += lineHeight;

      if (cursorY > tft.height() - lineHeight) {
        break;
      }

      currentLine = word;
    } else {
      currentLine = tempLine;
    }

    word = strtok(nullptr, " ");
  }

  if (!currentLine.isEmpty() && cursorY <= tft.height() - lineHeight) {
    tft.setCursor(cursorX, cursorY);
    tft.print(currentLine);
    cursorY += lineHeight;
  }

  tft.fillRect(0, TFT_VER_RES - 8, TFT_HOR_RES, 15, 0x3186);
  tft.drawFastHLine(0, TFT_VER_RES - 9, TFT_HOR_RES, 0xF800);
  tft.drawFastHLine(0, TFT_VER_RES - 10, TFT_HOR_RES, 0xF800);

  free(text);
  tft.setFreeFont(&Formula1_Regular_112pt7b);
}

void handleDriverView() {
    buttonPressedShort = false;
    if (turnedRight) {
        driverIndex = (driverIndex + 1) % driverNum;
        turnedRight = false;
        allowInfo = false;
    } else if (turnedLeft) {
        driverIndex = (driverIndex - 1 + driverNum) % driverNum;
        turnedLeft = false;
        allowInfo = false;
    }

    tft.fillScreen(TFT_BLACK);
    Driver selected = DRIVERS[driverIndex];
    uint16_t colour = convertHexToRGB565(selected.teamColour);
    
    displayDriver(selected.code);
    
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString(selected.givenName, TFT_VER_RES - 60, 60);
    tft.drawString(selected.familyName, TFT_VER_RES - 20, 90);
    
    tft.setTextSize(1);
    tft.setTextColor(colour);
    tft.drawString(selected.teamName, TFT_VER_RES - 60, 150);
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(selected.code, TFT_VER_RES + 75, 235);
    
    tft.setTextSize(2);
    tft.setTextColor(colour);
    int offset = (strlen(selected.number) == 1) ? 85 : 70;
    tft.drawString(selected.number, TFT_VER_RES + offset, 255);
    
    int diagonalLength = 15;
    tft.drawLine(0, 0, diagonalLength, diagonalLength, colour);
    cyclingBorder(colour);
    
    for(int i = 0; i < 3; i++) {
        tft.drawRect(5 + (i*3), 5 + (i*3), 20, 20, colour & (0xFFFF >> i));
    }
}

void handleDriverInfo() {
  buttonPressedShort = false;
  if (turnedRight) {
    driverIndex = (driverIndex + 1) % driverNum;
    turnedRight = false;
    allowInfo = false;
    displayDriverInfo = false;
  } else if (turnedLeft) {
    driverIndex = (driverIndex - 1 + driverNum) % driverNum;
    turnedLeft = false;
    allowInfo = false;
    displayDriverInfo = false;
  }

  tft.fillScreen(TFT_BLACK);
  Driver selected = DRIVERS[driverIndex];
  uint16_t colour = convertHexToRGB565(selected.teamColour);

  tft.fillRect(0, 0, TFT_HOR_RES, 35, 0x3186);
  tft.drawFastHLine(0, 35, TFT_HOR_RES, 0xF800);
  tft.drawFastHLine(0, 36, TFT_HOR_RES, 0xF800);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setFreeFont(&Formula1_Regular_112pt7b);
  tft.drawString("DRIVER STATISTICS", TFT_HOR_RES / 2, 5);

  tft.setFreeFont(&Formula1_Regular_18pt7b);
  tft.setTextDatum(TL_DATUM);
  int startY = 50;
  int labelX = 10;
  int valueX = TFT_HOR_RES / 2;

  const char* labels[] = { "WINS", "POINTS", "NATIONALITY", "DATE OF BIRTH" };
  const char* values[] = { selected.wins, selected.points, selected.nationality, selected.dateOfBirth };

  for (int i = 0; i < 4; i++) {
    if (i % 2 == 0) {
      tft.fillRect(0, startY + (i * 40), TFT_HOR_RES, 35, 0x1082);
    }

    tft.setTextColor(0x8410);
    tft.drawString(labels[i], labelX, startY + (i * 40) + 10);

    tft.setTextColor(TFT_WHITE);
    if (i < 2) {
      tft.setTextColor(TFT_YELLOW);
    }
    tft.drawString(values[i], valueX, startY + (i * 40) + 10);

    tft.fillRect(labelX - 5, startY + (i * 40) + 5, 2, 25, colour);
  }

  tft.setTextColor(0x8410);
  tft.drawString("<", 10, TFT_VER_RES - 20);
  tft.drawString(">", TFT_HOR_RES - 20, TFT_VER_RES - 20);
  tft.setFreeFont(&Formula1_Regular_112pt7b);
}

#define DRIVERS_PER_PAGE 7

int driverStandingsPage = 0;

void handleDriverStandingsView() {
  buttonPressedShort = false;
  int totalPages = (driverNum + DRIVERS_PER_PAGE - 1) / DRIVERS_PER_PAGE;

  if (turnedRight) {
    driverStandingsPage = min(driverStandingsPage + 1, totalPages - 1);
    turnedRight = false;
  } else if (turnedLeft) {
    driverStandingsPage = max(0, driverStandingsPage - 1);
    turnedLeft = false;
  }

  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, TFT_HOR_RES, 35, 0x3186);
  tft.drawFastHLine(0, 35, TFT_HOR_RES, 0xF800);
  tft.drawFastHLine(0, 36, TFT_HOR_RES, 0xF800);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("DRIVER STANDINGS", TFT_HOR_RES / 2, 5);

  tft.setFreeFont(&Formula1_Regular_18pt7b);
  char pageInfo[20];
  sprintf(pageInfo, "PAGE %d/%d", driverStandingsPage + 1, totalPages);
  tft.setTextDatum(TR_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(0x8410);
  tft.drawString(pageInfo, TFT_HOR_RES - 5, TFT_VER_RES - 15);


  tft.setTextDatum(TL_DATUM);
  int y = 45;
  int startIndex = driverStandingsPage * DRIVERS_PER_PAGE;
  int endIndex = min(startIndex + DRIVERS_PER_PAGE, driverNum);

  tft.setTextColor(0x8410);
  tft.drawString("POS", 10, y);
  tft.drawString("DRIVER", 60, y);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("PTS", TFT_HOR_RES - 10, y);
  tft.setTextDatum(TL_DATUM);
  y += 20;

  tft.setFreeFont(&Formula1_Regular_112pt7b);
  for (int i = startIndex; i < endIndex; i++) {
    Driver driver = DRIVERS[i];

    if (i % 2 == 0) {
      tft.fillRect(0, y - 5, TFT_HOR_RES, 30, 0x1082);
    }

    tft.setTextColor(TFT_WHITE);
    char pos[4];
    sprintf(pos, "%02d", i + 1);
    tft.drawString(pos, 10, y);

    tft.fillRect(3, y + 2, 3, 20, convertHexToRGB565(driver.teamColour));

    tft.setTextColor(convertHexToRGB565(driver.teamColour));
    tft.drawString(driver.code, 60, y);

    tft.setTextColor(TFT_WHITE);
    String firstName = String(driver.givenName).substring(0, 1) + ".";
    tft.drawString(firstName + " " + String(driver.familyName), 140, y);

    tft.setTextColor(TFT_YELLOW);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(driver.points, TFT_HOR_RES - 10, y);
    tft.setTextDatum(TL_DATUM);

    y += 35;
  }
}

#define TEAMS_PER_PAGE 7
int constructorStandingsPage = 0;

void handleConstructorStandingsView() {
  buttonPressedShort = false;
  int totalPages = (teamNum + TEAMS_PER_PAGE - 1) / TEAMS_PER_PAGE;
  if (turnedRight) {
    constructorStandingsPage = min(constructorStandingsPage + 1, totalPages - 1);
    turnedRight = false;
  } else if (turnedLeft) {
    constructorStandingsPage = max(0, constructorStandingsPage - 1);
    turnedLeft = false;
  }

  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, TFT_HOR_RES, 35, 0x3186);
  tft.drawFastHLine(0, 35, TFT_HOR_RES, 0xF800);
  tft.drawFastHLine(0, 36, TFT_HOR_RES, 0xF800);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("CONSTRUCTOR STANDINGS", TFT_HOR_RES / 2, 5);
  tft.setFreeFont(&Formula1_Regular_18pt7b);

  char pageInfo[20];
  sprintf(pageInfo, "PAGE %d/%d", constructorStandingsPage + 1, totalPages);
  tft.setTextDatum(TR_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(0x8410);
  tft.drawString(pageInfo, TFT_HOR_RES - 5, TFT_VER_RES - 15);

  tft.setTextDatum(TL_DATUM);
  int y = 45;
  int startIndex = constructorStandingsPage * TEAMS_PER_PAGE;
  int endIndex = min(startIndex + TEAMS_PER_PAGE, teamNum);

  tft.setTextColor(0x8410);
  tft.drawString("POS", 10, y);
  tft.drawString("TEAM", 60, y);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("PTS", TFT_HOR_RES - 10, y);
  tft.setTextDatum(TL_DATUM);
  y += 20;

  tft.setFreeFont(&Formula1_Regular_112pt7b);
  for (int i = startIndex; i < endIndex; i++) {
    Team team = TEAMS[i];
    if (i % 2 == 0) {
      tft.fillRect(0, y - 5, TFT_HOR_RES, 30, 0x1082);
    }

    tft.setTextColor(TFT_WHITE);
    char pos[4];
    sprintf(pos, "%02d", i + 1);
    tft.drawString(pos, 10, y);

    tft.fillRect(3, y + 2, 3, 20, convertHexToRGB565(team.driver1->teamColour));
    tft.setTextColor(convertHexToRGB565(team.driver1->teamColour));
    tft.drawString(team.name, 60, y);

    tft.setTextColor(TFT_YELLOW);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(team.points, TFT_HOR_RES - 10, y);
    tft.setTextDatum(TL_DATUM);
    y += 35;
  }
}

void cyclingBorder(uint16_t colour) {
  const int maxPos = 140;
  const int steps = 15;
  const int stepSize = maxPos / steps;

  for (int i = 0; i <= steps; i++) {
    int pos = i * stepSize;

    tft.drawFastVLine(TFT_HOR_RES - 1, TFT_VER_RES - pos + 5, stepSize, colour);
    tft.drawFastVLine(TFT_HOR_RES - 3, TFT_VER_RES - pos + 10, stepSize, colour);
    tft.drawFastVLine(TFT_HOR_RES - 5, TFT_VER_RES - pos + 15, stepSize, colour);

    tft.drawFastHLine(TFT_HOR_RES - pos, TFT_VER_RES - 1, stepSize, colour);
    tft.drawFastHLine(TFT_HOR_RES - pos - 5, TFT_VER_RES - 3, stepSize, colour);
    tft.drawFastHLine(TFT_HOR_RES - pos - 10, TFT_VER_RES - 5, stepSize, colour);

    delay(10);
  }
}

void displayDriver(const char* driverCode) {
  String filename = "/" + String(driverCode) + ".png";

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }

  if (!LittleFS.exists(filename)) {
    Serial.printf("File %s does not exist\n", filename.c_str());
    LittleFS.end();
    tft.fillRect(xpos, ypos, MAX_IMAGE_WIDTH, MAX_IMAGE_WIDTH, TFT_BLACK);
    return;
  }

  int16_t rc = png.open(filename.c_str(), pngOpen, pngClose, pngRead, pngSeek, pngDraw);

  if (rc == PNG_SUCCESS) {
    tft.startWrite();

    Serial.printf("Image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    uint32_t dt = millis();

    if (png.getWidth() > MAX_IMAGE_WIDTH) {
      Serial.println("Image too wide for allocated line buffer size!");
    } else {
      rc = png.decode(NULL, 0);
    }

    tft.endWrite();
    png.close();
    Serial.printf("Rendering took: %d ms\n", millis() - dt);
  } else {
    Serial.printf("PNG decode failed with code: %d\n", rc);
    png.close();
    tft.fillRect(xpos, ypos, MAX_IMAGE_WIDTH, MAX_IMAGE_WIDTH, TFT_BLACK);
  }

  LittleFS.end();
}

void pngDraw(PNGDRAW* pDraw) {
  static uint16_t lineBuffer[MAX_IMAGE_WIDTH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);

  if (pDraw->y % 10 == 0) {
    yield();
  }
}

uint16_t convertHexToRGB565(const char* hexColor) {
  uint32_t rgb888 = strtol(hexColor, NULL, 16);
  uint8_t r = (rgb888 >> 16) & 0xFF;
  uint8_t g = (rgb888 >> 8) & 0xFF;
  uint8_t b = rgb888 & 0xFF;

  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}