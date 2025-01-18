#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <TinyXML.h> 

#define MAX_ARTICLES 15

struct Driver {
  char* code;
  char* number;
  int position;
  char* positionText;
  char* points;
  char* givenName;
  char* familyName;
  char* dateOfBirth;
  char* nationality;
  char* headshot;
  char* wins;
  char* teamName;
  char* teamColour;
};

struct Team {
  char* constructorID;
  char* name;
  char* nationality;
  char* wins;
  char* points;
  int position;
  char* positionText;
  char* teamColour;
  Driver* driver1;
  Driver* driver2;
};

struct Article {
  char* title;
  char* description;
  char* link;
};

int driverNum = 0;
Driver* DRIVERS = nullptr;

int teamNum = 0;
Team* TEAMS = nullptr;

const char* rss_url = "https://www.motorsport.com/rss/f1/news/";

Article articles[MAX_ARTICLES];
int articleCount = 0;

void clearArticles() {
  for (int i = 0; i < articleCount; i++) {
    free(articles[i].title);
    free(articles[i].description);
    free(articles[i].link);
  }
  articleCount = 0;
}

void fetchRSS() {
  HTTPClient http;
  http.begin(rss_url);

  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      parseRSS(payload.c_str());
      saveRSSArticles();
    }
  } else {
    Serial.printf("HTTP GET failed: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

char* extractTagContent(const char* source, const char* startTag, const char* endTag) {
  const char* start = strstr(source, startTag);
  if (!start) return NULL;

  start += strlen(startTag);

  const char* cdataStart = strstr(start, "<![CDATA[");
  const char* cdataEnd = strstr(start, "]]>");
  
  if (cdataStart && cdataStart < strstr(start, endTag)) {
    cdataStart += strlen("<![CDATA[");
    size_t cdataLength = cdataEnd - cdataStart;

    char* content = (char*)malloc(cdataLength + 1);
    if (content) {
      strncpy(content, cdataStart, cdataLength);
      content[cdataLength] = '\0';
    }
    return content;
  }

  const char* end = strstr(start, endTag);
  if (!end) return NULL;

  size_t length = end - start;
  char* content = (char*)malloc(length + 1);
  if (content) {
    strncpy(content, start, length);
    content[length] = '\0';
  }
  return content;
}

void parseRSS(const char* xmlData) {
  clearArticles();

  const char* itemStart = xmlData;
  while ((itemStart = strstr(itemStart, "<item>")) && articleCount < MAX_ARTICLES) {
    const char* itemEnd = strstr(itemStart, "</item>");
    if (!itemEnd) break;

    char* title = extractTagContent(itemStart, "<title><![CDATA[", "]]></title>");
    char* description = extractTagContent(itemStart, "<description><![CDATA[", "]]></description>");
    char* link = extractTagContent(itemStart, "<link>", "</link>");

    if (description) {
      char* anchorStart = strstr(description, "<a");
      if (anchorStart) {
        *anchorStart = '\0';
      }
    }

    articles[articleCount].title = title ? title : strdup("No Title");
    articles[articleCount].description = description ? description : strdup("No Description");
    articles[articleCount].link = link ? link : strdup("No Link");

    articleCount++;
    itemStart = itemEnd;
  }

  Serial.printf("Parsed %d articles\n", articleCount);
}

void saveRSSArticles() {
  if (!LittleFS.begin()) {
    Serial.println("[LittleFS] Initialization failed.");
    return;
  }

  File file = LittleFS.open("/news.json", "w");
  if (!file) {
    Serial.println("[LittleFS] Failed to open file for writing.");
    LittleFS.end();
    return;
  }

  DynamicJsonDocument doc(4096);
  JsonArray jsonArticles = doc.to<JsonArray>();

  for (int i = 0; i < articleCount; i++) {
    JsonObject article = jsonArticles.createNestedObject();
    article["title"] = articles[i].title ? articles[i].title : "No Title";
    article["description"] = articles[i].description ? articles[i].description : "No Description";
    article["link"] = articles[i].link ? articles[i].link : "No Link";
  }

  if (serializeJsonPretty(doc, file) == 0) {
    Serial.println("[LittleFS] Failed to write JSON to file.");
  } else {
    Serial.println("[LittleFS] Successfully saved RSS articles.");
  }

  file.close();
  LittleFS.end();
}

void loadRSSArticles() {
  if (!LittleFS.begin()) {
    Serial.println("[LittleFS] Initialization failed.");
    return;
  }

  File file = LittleFS.open("/news.json", "r");
  if (!file) {
    Serial.println("[LittleFS] Failed to open file for reading.");
    LittleFS.end();
    return;
  }

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    Serial.print("[LittleFS] JSON Deserialization failed: ");
    Serial.println(error.c_str());
    file.close();
    LittleFS.end();
    return;
  }

  clearArticles();

  JsonArray jsonArticles = doc.as<JsonArray>();
  articleCount = jsonArticles.size();

  for (int i = 0; i < articleCount && i < MAX_ARTICLES; i++) {
    JsonObject article = jsonArticles[i];
    articles[i].title = strdup(article["title"] | "No Title");
    articles[i].description = strdup(article["description"] | "No Description");
    articles[i].link = strdup(article["link"] | "No Link");
  }

  file.close();
  LittleFS.end();

  Serial.printf("[LittleFS] Successfully loaded %d RSS articles.\n", articleCount);
}

void setupFS() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS initialisation failed!");
    while (1) yield();
  }
}

String downloadHeadshot(const String& url, const String& driverId) {
  if (url.isEmpty()) {
    Serial.printf("[Image] No headshot URL for driver %s\n", driverId.c_str());
    return "";
  }

  HTTPClient http;
  http.begin(url);
  http.setTimeout(5000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[Image] Failed to download headshot for %s: HTTP %d\n",
                  driverId.c_str(), httpCode);
    http.end();
    return "";
  }

  String filename = "/" + driverId + ".png";
  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.printf("[Image] Failed to create file for %s\n", driverId.c_str());
    http.end();
    return "";
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = 0;
  int len = http.getSize();
  size_t total = len;

  uint8_t buffer[1024];
  unsigned long startTime = millis();

  while (http.connected() && (len > 0 || len == -1)) {
    size_t available = stream->available();
    if (available) {
      int c = stream->readBytes(buffer, ((available > sizeof(buffer)) ? sizeof(buffer) : available));
      written += file.write(buffer, c);

      if (len > 0) {
        len -= c;
      }
    }
    yield();
  }

  file.close();
  http.end();

  // Verify file size
  if (written == 0) {
    LittleFS.remove(filename);
    Serial.printf("[Image] No data downloaded for %s\n", driverId.c_str());
    return "";
  }

  Serial.printf("[Image] Downloaded headshot for %s (%d bytes)\n", driverId.c_str(), written);
  return filename;
}

String fetchDataPayload(const char* url) {
  HTTPClient http;
  http.begin(url);
  http.setTimeout(10000);

  int httpCode = http.GET();
  String payload = "";

  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
  } else {
    Serial.printf("[HTTP] GET failed for %s, error code: %d, error: %s\n",
                  url,
                  httpCode,
                  http.errorToString(httpCode).c_str());
  }

  http.end();
  return payload;
}

void fetchConstructorData() {
    const char* standingsUrl = "https://api.jolpi.ca/ergast/f1/2024/constructorstandings/";

    Serial.println("[API] Fetching constructor data...");

    if (!LittleFS.begin()) {
        Serial.println("[LittleFS] Initialization failed.");
        return;
    }

    String standingsPayload = fetchDataPayload(standingsUrl);

    if (standingsPayload.isEmpty()) {
        Serial.println("[API] Failed to retrieve constructor data.");
        LittleFS.end();
        return;
    }

    DynamicJsonDocument standingsDoc(20480);
    DeserializationError standingsError = deserializeJson(standingsDoc, standingsPayload);

    if (standingsError) {
        Serial.printf("[API] JSON Deserialization failed: %s\n", standingsError.c_str());
        LittleFS.end();
        return;
    }

    JsonArray standings = standingsDoc["MRData"]["StandingsTable"]["StandingsLists"][0]["ConstructorStandings"].as<JsonArray>();

    teamNum = standings.size();

    if (TEAMS != nullptr) {
        for (int i = 0; i < teamNum; i++) {
            safeFree(TEAMS[i].constructorID);
            safeFree(TEAMS[i].name);
            safeFree(TEAMS[i].nationality);
            safeFree(TEAMS[i].wins);
            safeFree(TEAMS[i].points);
            safeFree(TEAMS[i].positionText);
        }
        delete[] TEAMS;
        TEAMS = nullptr;
    }

    TEAMS = new Team[teamNum];

    Serial.printf("[Debug] Loading %d teams from API\n", teamNum);

    for (int i = 0; i < teamNum; i++) {
        JsonObject teamStanding = standings[i];
        JsonObject constructor = teamStanding["Constructor"];

        TEAMS[i].constructorID = strdup(constructor["constructorId"] | "");
        TEAMS[i].name = strdup(constructor["name"] | "");
        TEAMS[i].nationality = strdup(constructor["nationality"] | "");
        TEAMS[i].wins = strdup(teamStanding["wins"] | "0");
        TEAMS[i].points = strdup(teamStanding["points"] | "0");
        TEAMS[i].position = teamStanding["position"] | 0;
        TEAMS[i].positionText = strdup(teamStanding["positionText"] | "");

        Serial.printf("[Debug] Loaded team %d: %s (%s)\n", i, TEAMS[i].name, TEAMS[i].constructorID);
    }

    File file = LittleFS.open("/constructors.json", "w");
    if (!file) {
        Serial.println("[LittleFS] Failed to open /constructors.json for writing.");
        LittleFS.end();
        return;
    }

    DynamicJsonDocument outDoc(16384);
    JsonArray constructorsArray = outDoc.to<JsonArray>();

    for (int i = 0; i < teamNum; i++) {
        JsonObject constructor = constructorsArray.createNestedObject();
        constructor["constructorID"] = TEAMS[i].constructorID;
        constructor["name"] = TEAMS[i].name;
        constructor["nationality"] = TEAMS[i].nationality;
        constructor["wins"] = TEAMS[i].wins;
        constructor["points"] = TEAMS[i].points;
        constructor["position"] = TEAMS[i].position;
        constructor["positionText"] = TEAMS[i].positionText;
    }

    if (serializeJsonPretty(outDoc, file) == 0) {
        Serial.println("[LittleFS] Failed to write constructors to file.");
    } else {
        Serial.println("[LittleFS] Successfully wrote constructors to /constructors.json");
    }

    file.close();
    LittleFS.end();
}


void fetchDriverData() {
  const char* standingsURL = "https://api.jolpi.ca/ergast/f1/2024/driverstandings/";
  const char* driverURL = "https://api.openf1.org/v1/drivers?session_key=latest";

  Serial.println("[API] Fetching driver data...");

  if (!LittleFS.begin()) {
    Serial.println("[LittleFS] Initialization failed.");
    return;
  }

  String standingsPayload = fetchDataPayload(standingsURL);
  String driverPayload = fetchDataPayload(driverURL);

  if (standingsPayload.isEmpty() || driverPayload.isEmpty()) {
    Serial.println("[API] Failed to retrieve data from one or more APIs.");
    LittleFS.end();
    return;
  }

  DynamicJsonDocument standingsDoc(20480);
  DynamicJsonDocument driverDoc(20480);

  DeserializationError standingsError = deserializeJson(standingsDoc, standingsPayload);
  DeserializationError driverError = deserializeJson(driverDoc, driverPayload);

  if (standingsError || driverError) {
    Serial.println("[API] JSON Deserialization failed:");
    Serial.println(standingsError ? standingsError.c_str() : "Standings OK");
    Serial.println(driverError ? driverError.c_str() : "Driver OK");
    LittleFS.end();
    return;
  }

  JsonArray standings = standingsDoc["MRData"]["StandingsTable"]["StandingsLists"][0]["DriverStandings"].as<JsonArray>();
  JsonArray openF1Drivers = driverDoc.as<JsonArray>();

  DynamicJsonDocument outDoc(40960);
  JsonArray driversArray = outDoc.to<JsonArray>();

  for (JsonObject driverStanding : standings) {
    JsonObject driver = driverStanding["Driver"];
    JsonObject constructor = driverStanding["Constructors"][0];

    JsonObject newDriver = driversArray.createNestedObject();

    const char* driverCode = driver["code"] | "";

    newDriver["code"] = driverCode;
    newDriver["number"] = driver["permanentNumber"] | "";
    newDriver["givenName"] = driver["givenName"] | "";  // FIX: Convert 'umlaut' letters into good counterpart for Hulk and Perez
    newDriver["familyName"] = driver["familyName"] | "";
    newDriver["dateOfBirth"] = driver["dateOfBirth"] | "";
    newDriver["nationality"] = driver["nationality"] | "";
    newDriver["position"] = driverStanding["position"] | "";
    newDriver["positionText"] = driverStanding["position"] | "";
    newDriver["points"] = driverStanding["points"] | "";
    newDriver["wins"] = driverStanding["wins"] | "";
    newDriver["teamName"] = constructor["name"] | "";

    for (JsonObject openF1Driver : openF1Drivers) {
      if (openF1Driver["name_acronym"].as<String>() == driverCode) {
        String headshotUrl = openF1Driver["headshot_url"] | "";
        if (headshotUrl.indexOf("1col") != -1) {
          headshotUrl.replace("1col", "3col");
        }

        String localHeadshotPath = downloadHeadshot(headshotUrl, driverCode);
        newDriver["headshot"] = localHeadshotPath;
        newDriver["teamColour"] = openF1Driver["team_colour"] | "";

        break;
      }
    }

    Serial.printf("[Debug] Processed driver: %s %s\n", newDriver["givenName"].as<const char*>(),newDriver["familyName"].as<const char*>());
  }

  File file = LittleFS.open("/drivers.json", "w");
  if (!file) {
    Serial.println("[LittleFS] Failed to open file for writing.");
    LittleFS.end();
    return;
  }

  if (serializeJsonPretty(driversArray, file) == 0) {
    Serial.println("[LittleFS] Failed to write JSON to file");
  } else {
    Serial.println("[LittleFS] Successfully wrote JSON to file");
  }

  file.close();
  LittleFS.end();

  driverNum = driversArray.size();
  Serial.printf("[LittleFS] Total drivers stored: %d\n", driverNum);
}

void loadDriverData() {
  if (!LittleFS.begin()) {
    Serial.println("[LittleFS] Initialization failed.");
    return;
  }

  File file = LittleFS.open("/drivers.json", "r");
  if (!file) {
    Serial.println("[LittleFS] Failed to open file for reading.");
    LittleFS.end();
    return;
  }

  StaticJsonDocument<16384> doc;
  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    Serial.print("[LittleFS] JSON Deserialization failed: ");
    Serial.println(error.c_str());
    file.close();
    LittleFS.end();
    return;
  }

  JsonArray jsonDrivers = doc.as<JsonArray>();
  driverNum = jsonDrivers.size();

  if (DRIVERS != nullptr) {
    for (int i = 0; i < driverNum; i++) {
      safeFree(DRIVERS[i].code);
      safeFree(DRIVERS[i].givenName);
      safeFree(DRIVERS[i].number);
      safeFree(DRIVERS[i].familyName);
      safeFree(DRIVERS[i].dateOfBirth);
      safeFree(DRIVERS[i].nationality);
      safeFree(DRIVERS[i].positionText);
      safeFree(DRIVERS[i].points);
      safeFree(DRIVERS[i].wins);
      safeFree(DRIVERS[i].headshot);
      safeFree(DRIVERS[i].teamName);
      safeFree(DRIVERS[i].teamColour);
    }
    delete[] DRIVERS;
    DRIVERS = nullptr;
  }

  DRIVERS = new Driver[driverNum];

  Serial.printf("[Debug] Loading %d drivers from file\n", driverNum);

  for (int i = 0; i < driverNum; i++) {
    JsonObject driver = jsonDrivers[i];

    DRIVERS[i].code = strdup(driver["code"] | "///");
    DRIVERS[i].number = strdup(driver["number"] | "00");
    DRIVERS[i].givenName = strdup(driver["givenName"] | "");
    DRIVERS[i].familyName = strdup(driver["familyName"] | "");
    DRIVERS[i].dateOfBirth = strdup(driver["dateOfBirth"] | "");
    DRIVERS[i].nationality = strdup(driver["nationality"] | "");
    DRIVERS[i].positionText = strdup(driver["positionText"] | "");
    DRIVERS[i].position = driver["position"];
    DRIVERS[i].points = strdup(driver["points"] | "");
    DRIVERS[i].headshot = strdup(driver["headshot"] | "");
    DRIVERS[i].wins = strdup(driver["wins"] | "");
    DRIVERS[i].teamName = strdup(driver["teamName"] | "");
    DRIVERS[i].teamColour = strdup((driver["teamColour"] | "FFFFFF"));

    Serial.printf("[Debug] Loaded driver %d: %s %s (%s)\n",
                  i,
                  DRIVERS[i].givenName,
                  DRIVERS[i].familyName,
                  DRIVERS[i].code);
  }

  file.close();
  LittleFS.end();

  Serial.printf("[LittleFS] Successfully loaded %d drivers into RAM\n", driverNum);
}

void loadConstructorData() {
  if (!LittleFS.begin()) {
    Serial.println("[LittleFS] Initialization failed.");
    return;
  }

  File file = LittleFS.open("/constructors.json", "r");
  if (!file) {
    Serial.println("[LittleFS] Failed to open file for reading.");
    LittleFS.end();
    return;
  }

  StaticJsonDocument<16384> doc;
  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    Serial.print("[LittleFS] JSON Deserialization failed: ");
    Serial.println(error.c_str());
    file.close();
    LittleFS.end();
    return;
  }

  JsonArray jsonTeams = doc.as<JsonArray>();
  teamNum = jsonTeams.size();

  if (TEAMS != nullptr) {
    for (int i = 0; i < teamNum; i++) {
      safeFree(TEAMS[i].constructorID);
      safeFree(TEAMS[i].name);
      safeFree(TEAMS[i].nationality);
      safeFree(TEAMS[i].positionText);
      safeFree(TEAMS[i].teamColour);
    }
    delete[] TEAMS;
    TEAMS = nullptr;
  }

  TEAMS = new Team[teamNum];

  Serial.printf("[Debug] Loading %d teams from file\n", teamNum);

  for (int i = 0; i < teamNum; i++) {
    JsonObject team = jsonTeams[i];

    TEAMS[i].constructorID = strdup(team["constructorID"] | "");
    TEAMS[i].name = strdup(team["name"] | "");
    TEAMS[i].nationality = strdup(team["nationality"] | "");
    TEAMS[i].points = strdup(team["points"] | "");
    TEAMS[i].position = team["position"] | 0;
    TEAMS[i].positionText = strdup(team["positionText"] | "");
    TEAMS[i].teamColour = strdup((team["teamColour"] | "FFFFFF"));

    Serial.printf("[Debug] Loaded team %d: %s (%s) - Points: %d\n",
                  i,
                  TEAMS[i].name,
                  TEAMS[i].constructorID,
                  TEAMS[i].points);
  }

  file.close();
  LittleFS.end();

  Serial.printf("[LittleFS] Successfully loaded %d teams into RAM\n", teamNum);
}

void formatLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("[LittleFS] Failed to initialize. Attempting to format...");
    if (LittleFS.format()) {
      Serial.println("[LittleFS] Successfully formatted.");
    } else {
      Serial.println("[LittleFS] Failed to format. Check your hardware.");
    }
  } else {
    Serial.println("[LittleFS] Initialized successfully.");
  }
  LittleFS.end();
}

void clearTeamData() {
  if (TEAMS != nullptr) {
    for (int i = 0; i < teamNum; i++) {
      safeFree(TEAMS[i].constructorID);
      safeFree(TEAMS[i].name);
      safeFree(TEAMS[i].nationality);
      safeFree(TEAMS[i].points);
      safeFree(TEAMS[i].positionText);
      safeFree(TEAMS[i].teamColour);
      TEAMS[i].driver1 = nullptr;
      TEAMS[i].driver2 = nullptr;
    }
    delete[] TEAMS;
    TEAMS = nullptr;
    Serial.println("[Info] Team data cleared from RAM.");
  }
  teamNum = 0;
}

void clearDriverData() {
  if (DRIVERS != nullptr) {
    for (int i = 0; i < driverNum; i++) {
      safeFree(DRIVERS[i].code);
      safeFree(DRIVERS[i].givenName);
      safeFree(DRIVERS[i].familyName);
      safeFree(DRIVERS[i].dateOfBirth);
      safeFree(DRIVERS[i].nationality);
      safeFree(DRIVERS[i].headshot);
      safeFree(DRIVERS[i].teamName);
      safeFree(DRIVERS[i].teamColour);
    }
    delete[] DRIVERS;
    DRIVERS = nullptr;
    Serial.println("[Info] Driver data cleared from RAM.");
  }
  driverNum = 0;
}

void assignDriversToTeams() {
  if (DRIVERS == nullptr || TEAMS == nullptr) {
    Serial.println("[Error] Driver or Team data is missing.");
    return;
  }

  for (int i = 0; i < teamNum; i++) {
    int driverCount = 0;
    for (int j = 0; j < driverNum; j++) {
      if (strcmp(TEAMS[i].name, DRIVERS[j].teamName) == 0) {
        if (driverCount == 0) {
          TEAMS[i].driver1 = &DRIVERS[j];
          driverCount++;
        } else if (driverCount == 1) {
          TEAMS[i].driver2 = &DRIVERS[j];
          break;
        }
      }
    }

    if (TEAMS[i].driver1 != nullptr) {
      Serial.printf("[Link] Team %s: Driver1 = %s %s\n", TEAMS[i].name, TEAMS[i].driver1->givenName, TEAMS[i].driver1->familyName);
    }
    if (TEAMS[i].driver2 != nullptr) {
      Serial.printf("[Link] Team %s: Driver2 = %s %s\n", TEAMS[i].name, TEAMS[i].driver2->givenName, TEAMS[i].driver2->familyName);
    }
  }
}

void safeFree(char* ptr) {
  if (ptr) {
    free(ptr);
  }
}