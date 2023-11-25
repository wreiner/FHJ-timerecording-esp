# timerecording hardware POC

Read RFID tags with an ESP8266 and write data to Firebase.

## Configuration

Credentials are hardcoded as this is only a test in `src/main.cpp`.

- Set `WIFI_SSID` and `WIFI_KEY` for wifi configuration.

- Set `API_KEY`, `DATABASE_URL`, `USER_EMAIL`, `USER_PASSWORD` for Firebase access.

## Compile

To use the Firebase library install the library through Platformio but overwrite the file `.pio/libdeps/nodemcuv2/Firebase ESP8266 Client/src/FirebaseFS.h` with `overwrites/FirebaseFS.h`.
It basically comments out everything regarding file systems.

Also create a symlink between `.pio/libdeps/nodemcuv2/Firebase ESP8266 Client/src/Firebase.h` and `.pio/libdeps/nodemcuv2/Firebase ESP8266 Client/src/FirebaseESP8266.h`.
