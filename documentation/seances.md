# Suivi des Seances du projet

### Séance du 2/09
-> Recherche et reflexion sur le projet

### Séance du 9/09
-> Suite de la reflexion sur le projet
-> recherche des composants et BOM

### Séance du 16/09
-> test d'une esp32-S3 M16R8
    -> problème vis à vis d'un des ports usb-c
-> test lien esp32 - ecran (avec un ST7789 provenant d'un ancien projet)
-> recherches d'API pour le projets (cf APIs)

### Séance du 30/09
-> test de l'écran definitif (ILI9341 avec touch)
-> suite des recherches d'API

### Séance du 14/10
-> recherche de modèles pour le text to speech/brain/speech to text
-> test du text to speech et speech to text

### Séance du 25/11
-> test du combo ecran + speech to text avec ESP_SR
-> modification des réglages de l'arduino IDE à :
        -> Flash size : 16MB (128Mb)
        -> Partition Scheme : ESP SR 16M (3MB APP/7MB SPIFFS/2.9MB MODEL)
        ->  PSRAM : OPI PSRAM 
-> le code de test prend 57% de progamme storage et 18% de dynamic memory