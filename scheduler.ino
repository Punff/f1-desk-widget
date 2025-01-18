#include <NTPClient.h>
#include <TimeLib.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char* lastUpdateFile = "/last_update.txt";
unsigned long lastUpdateCheckMillis = 0;
const unsigned long checkInterval = 60000; // 1 min

void updateData() {
  Serial.println("Updating data...");
  fetchDriverData();
  fetchConstructorData();
  fetchRSS();
}

void loadData() {
  Serial.println("Loading data...");
  loadDriverData();
  loadConstructorData();
  assignDriversToTeams();
  loadRSSArticles();
}

String readLastUpdateDate() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS not mounted.");
    return "";
  }

  if (!LittleFS.exists(lastUpdateFile)) {
    return "";
  }

  File file = LittleFS.open(lastUpdateFile, "r");
  if (!file) {
    Serial.println("Failed to open last update file.");
    return "";
  }

  String date = file.readString();
  file.close();
  date.trim();
  return date;
}

void writeLastUpdateDate(const String& date) {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS not mounted.");
    return;
  }

  File file = LittleFS.open(lastUpdateFile, "w");
  if (!file) {
    Serial.println("Failed to open last update file for writing.");
    return;
  }
  file.println(date);
  file.close();
}


void checkDailyUpdate() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdateCheckMillis < checkInterval) {
    return;
  }
  lastUpdateCheckMillis = currentMillis;

  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();

  int currentYear = year(epochTime);
  int currentMonth = month(epochTime);
  int currentDay = day(epochTime);

  char currentDate[11];
  snprintf(currentDate, sizeof(currentDate), "%04d-%02d-%02d", currentYear, currentMonth, currentDay);

  String lastUpdateDate = readLastUpdateDate();
  Serial.printf("Current Date: %s, Last Update Date: %s\n", currentDate, lastUpdateDate.c_str());

  if (String(currentDate) != lastUpdateDate) {
    Serial.println("Performing daily update...");
    updateData();
    writeLastUpdateDate(currentDate);
  } else {
    Serial.println("Daily update not needed.");
  }
}