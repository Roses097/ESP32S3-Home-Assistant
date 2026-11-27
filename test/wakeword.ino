/*
wake word : Hi ESP
 Carte : ESP32S3 Dev Module
 Flash Size : 16MB
 PSRAM   : OPI PSRAM
 Partition Scheme : "ESP SR 16M (3MB APP/7MB SPIFFS/2.9MB MODEL)"
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ESP_I2S.h"
#include "ESP_SR.h"

// ---------- Broches ----------
#define I2S_SCK  41
#define I2S_WS   42
#define I2S_SD   40
#define TOUCH_CS 21     // tactile non utilisé : CS forcé à HIGH (bus SPI partagé)

#define I2S_SAMPLE_RATE 16000
#define SR_INPUT_FORMAT "M" // un seul micro
#define SR_INPUT_CHANNELS SR_CHANNELS_MONO

TFT_eSPI tft = TFT_eSPI();
I2SClass i2s;

// ---------- Commandes de test ----------
// Dans ce core, sr_cmd_t = { command_id, texte } : le texte est écrit en clair (anglais),
// plusieurs phrases peuvent partager le même command_id.
enum {
  CMD_HELLO,
  CMD_GOODBYE,
  CMD_YES,
  CMD_NO,
  CMD_START,
  CMD_STOP,
  CMD_MAX
};

static const sr_cmd_t sr_commands[] = {
  {CMD_HELLO,   "Hello"},
  {CMD_HELLO,   "Hi there"},
  {CMD_GOODBYE, "Goodbye"},
  {CMD_GOODBYE, "See you later"},
  {CMD_YES,     "Yes"},
  {CMD_NO,      "No"},
  {CMD_START,   "Start"},
  {CMD_STOP,    "Stop"},
};
static const size_t NB_CMDS = sizeof(sr_commands) / sizeof(sr_cmd_t);

// Texte affiché à l'écran pour chaque command_id (ASCII : la police TFT n'a pas d'accents)
static const char* const LABELS_FR[CMD_MAX] = {
  "Bonjour !",
  "Au revoir !",
  "Oui",
  "Non",
  "Demarrer",
  "Arreter",
};

// ---------- Messages callback -> boucle principale ----------
// (on ne dessine pas depuis le callback : il tourne dans la tâche audio de ESP-SR)
enum UiEvent { UI_WAKE, UI_TIMEOUT, UI_COMMAND };
struct UiMsg { UiEvent type; int commandId; int phrase; };
QueueHandle_t uiQueue;

void onSrEvent(sr_event_t event, int command_id, int phrase_id) {
  UiMsg m = {UI_WAKE, -1, -1};
  switch (event) {
    case SR_EVENT_WAKEWORD:
      // En mono il n'y a pas d'événement WAKEWORD_CHANNEL : on passe en mode commande ici
      ESP_SR.setMode(SR_MODE_COMMAND);
      m.type = UI_WAKE;
      xQueueSend(uiQueue, &m, 0);
      break;

    case SR_EVENT_WAKEWORD_CHANNEL:
      ESP_SR.setMode(SR_MODE_COMMAND);
      break;

    case SR_EVENT_TIMEOUT:
      ESP_SR.setMode(SR_MODE_WAKEWORD);         // retour à l'attente du mot de réveil
      m.type = UI_TIMEOUT;
      xQueueSend(uiQueue, &m, 0);
      break;

    case SR_EVENT_COMMAND:
      m.type = UI_COMMAND;
      m.commandId = command_id;
      m.phrase = phrase_id;
      xQueueSend(uiQueue, &m, 0);
      ESP_SR.setMode(SR_MODE_COMMAND);          // permet d'enchaîner plusieurs commandes
      break;

    default:
      break;
  }
}

// ---------- Affichage ----------
#define ZONE_STATUS_H  50
#define ZONE_CMD_Y     60
#define ZONE_CMD_H     90
#define ZONE_LOG_Y     165
#define LOG_LINES      5

String logLines[LOG_LINES];

void drawStatus(const char* msg, uint16_t color) {
  tft.fillRect(0, 0, tft.width(), ZONE_STATUS_H, TFT_BLACK);
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(8, 12);
  tft.print(msg);
  tft.drawFastHLine(0, ZONE_STATUS_H, tft.width(), TFT_DARKGREY);
}

void drawCommand(const String& txt) {
  tft.fillRect(0, ZONE_CMD_Y, tft.width(), ZONE_CMD_H, TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setTextWrap(true);
  tft.setCursor(8, ZONE_CMD_Y + 8);
  tft.print(txt);
}

void drawLog() {
  tft.fillRect(0, ZONE_LOG_Y, tft.width(), tft.height() - ZONE_LOG_Y, TFT_BLACK);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextSize(2);
  for (int i = 0; i < LOG_LINES; i++) {
    tft.setCursor(8, ZONE_LOG_Y + 6 + i * 28);
    tft.print(logLines[i]);
  }
}

void pushLog(const String& s) {
  for (int i = LOG_LINES - 1; i > 0; i--) logLines[i] = logLines[i - 1];
  logLines[0] = s;
  drawLog();
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);

  tft.init();
  tft.setRotation(1);                 // paysage 480x320
  tft.fillScreen(TFT_BLACK);
  drawStatus("Demarrage...", TFT_YELLOW);

  uiQueue = xQueueCreate(8, sizeof(UiMsg));

  // INMP441 : micro mono sur le canal gauche (L/R relié à GND), 16 bits / 16 kHz.
  // setPins(bclk, ws, dout, din) : pas de sortie audio (-1)
  i2s.setTimeout(1000);
  i2s.setPins(I2S_SCK, I2S_WS, -1, I2S_SD);
  if (!i2s.begin(I2S_MODE_STD, I2S_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
    drawStatus("Erreur I2S !", TFT_RED);
    Serial.println("Echec init I2S");
    while (true) delay(1000);
  }
  // Si le micro est trop faible / ne répond pas, alternative (données 32 bits ramenées à 16) :
  //   i2s.begin(I2S_MODE_STD, I2S_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);
  //   i2s.configureRX(I2S_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_RX_TRANSFORM_32_TO_16);

  ESP_SR.onEvent(onSrEvent);
  if (!ESP_SR.begin(i2s, sr_commands, NB_CMDS, SR_INPUT_CHANNELS, SR_MODE_WAKEWORD, SR_INPUT_FORMAT)) {
    drawStatus("Erreur ESP-SR !", TFT_RED);
    Serial.println("Echec ESP_SR.begin : verifie le Partition Scheme 'ESP SR 16M' et PSRAM = OPI");
    while (true) delay(1000);
  }

  drawStatus("Dis: Hi ESP", TFT_CYAN);
  drawCommand("");
  drawLog();
}

// ---------- Loop ----------
void loop() {
  UiMsg m;
  if (xQueueReceive(uiQueue, &m, pdMS_TO_TICKS(50)) != pdTRUE) return;

  switch (m.type) {
    case UI_WAKE:
      Serial.println("Wake word detecte");
      drawStatus("Je t'ecoute...", TFT_RED);
      break;

    case UI_TIMEOUT:
      Serial.println("Timeout");
      drawStatus("Dis: Hi ESP", TFT_CYAN);
      break;

    case UI_COMMAND: {
      const char* heard = (m.phrase >= 0 && (size_t)m.phrase < NB_CMDS) ? sr_commands[m.phrase].str : "?";
      const char* label = (m.commandId >= 0 && m.commandId < CMD_MAX) ? LABELS_FR[m.commandId] : "???";
      Serial.printf("Commande %d : %s\n", m.commandId, heard);
      drawCommand(String(label));
      pushLog(String(heard));
      drawStatus("Commande OK", TFT_GREEN);
      break;
    }
  }
}