#include <LedControl.h>

const char keyUp = 'U';
const char keyDown = 'D';
const char keyRight = 'R';
const char keyLeft = 'L';
const char keyNone = 'N';
const char keyInvert = 'I';
const char keyStart = 'S';

const int displaysCount = 4;
const int dataPin = 12;
const int clkPin = 10;
const int csPin = 11;

LedControl lc = LedControl(dataPin, clkPin, csPin, displaysCount);

int currentX = 0;
int currentY = 0;

#define MODE_SETUP 1
#define MODE_GAME 0

int currentMode = MODE_SETUP;

const byte rowAmount = 4;
const byte colAmount = 4;

static bool keyDownMatrix[rowAmount][colAmount];

char keyMatrix[rowAmount][colAmount] = {
  {keyStart, keyUp,    keyNone,  keyNone},
  {keyLeft, keyInvert,  keyRight, keyNone},
  {keyNone, keyDown,  keyNone,  keyNone},
  {keyNone, keyNone,  keyNone,  keyNone}
};

byte rowPins[rowAmount] = { 5, 4, 3, 2 };
byte colPins[colAmount] = { 6, 7, 8, 9 };

const int fieldRowsCount = 16;
const int fieldColsCount = 16;

bool field[fieldRowsCount][fieldColsCount];
bool bufferField[fieldRowsCount][fieldColsCount];

int ticks = 0;

void setup()
{
  Serial.begin(115200);

  for (int i = 0; i < rowAmount; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }

  for (int i = 0; i < colAmount; i++) {
    pinMode(colPins[i], INPUT);
    digitalWrite(colPins[i], HIGH);
  }

  for (int i = 0; i < displaysCount; i++) {
    lc.shutdown(i, false);
    lc.setIntensity(i, 50);
    lc.clearDisplay(i);
  }
}

void loop() {
  if (currentMode == MODE_SETUP) {
    doSetupMode();
    bool prev = field[currentY][currentX];
    if (prev) {
      field[currentY][currentX] = ticks % 2;
    }
    else {
      field[currentY][currentX] = ticks % 4 == 1;
    }
    drawScreen();
    field[currentY][currentX] = prev;
  }
  else {
    doGameMode();
    drawScreen();
  }
}

void doSetupMode() {
  char key = getKey();

  if (key != keyNone) {
    if (key == keyUp && currentY > 0) {
      currentY--;
    }

    if (key == keyDown && currentY < fieldRowsCount - 1) {
      currentY++;
    }

    if (key == keyLeft && currentX > 0) {
      currentX--;
    }

    if (key == keyRight && currentX < fieldColsCount - 1) {
      currentX++;
    }

    if (key == keyInvert) {
      field[currentY][currentX] = (field[currentY][currentX] + 1) % 2;
    }

    if (key == keyStart) {
      currentMode = MODE_GAME;
    }
  }

  ticks++;
}

void doGameMode() {
  updateField();  
}

inline bool inSegment(int x, int b, int e) {
  return b <= x && x <= e;
}

inline void translateCords(int y, int x, int *screen, int *ry, int *rx) {
  if (inSegment(y, 0, 7) && inSegment(x, 0, 7)) {
    *screen = 3;
    *ry = 7 - y % 8;
    *rx = 7 - x % 8;
  }

  if (inSegment(y, 0, 7) && inSegment(x, 8, 15)) {
    *screen = 2;
    *ry = 7 - y % 8;
    *rx = 7 - x % 8;
  }

  if (inSegment(y, 8, 15) && inSegment(x, 8, 15)) {
    *screen = 1;
    *ry = y % 8;
    *rx = x % 8;
  }

  if (inSegment(y, 8, 15) && inSegment(x, 0, 7)) {
    *screen = 0;
    *ry = y % 8;
    *rx = x % 8;
  }
}


inline bool inBound(int y, int x) {
  return y >= 0 && y < fieldRowsCount && x >= 0 && x < fieldColsCount;
}

int getNeightboursCount(int y, int x) {
  int count = 0;

  for (int dy = -1; dy < 2; dy++) {
    for (int dx = -1; dx < 2; dx++) {
      if (dx == 0 && dy == 0) {
        continue;
      }

      int yy = y + dy;
      int xx = x + dx;

      if (inBound(yy, xx)) {
        count += field[yy][xx];
      }
    }
  }

  return count;
}

inline int getNextStateOf(int y, int x) {
  int count = getNeightboursCount(y, x);

  if (field[y][x] == 1) {
    return count == 2 || count == 3;
  }
  else {
    return count == 3;
  }
}

void updateField() {
  for (int y = 0; y < fieldRowsCount; y++) {
    for (int x = 0; x < fieldColsCount; x++) {
      bufferField[y][x] = getNextStateOf(y, x);
    }
  }

  for (int y = 0; y < fieldRowsCount; y++) {
    for (int x = 0; x < fieldColsCount; x++) {
      field[y][x] = bufferField[y][x];
    }
  }
}

void drawScreen() {
  for (int y = 0; y < fieldRowsCount; y++) {
    for (int x = 0; x < fieldColsCount; x++) {
      int screen, ry, rx;
      translateCords(y, x, &screen, &ry, &rx);
      lc.setLed(screen, ry, rx, field[y][x]);
    }
  }
}

char getKey()
{
  char result = keyNone;
  for (int i = 0; i < rowAmount; i++) {
    for (int j = 0; j < colAmount; j++) {
      bool isDown = isKeyDown(i, j);
      if (!keyDownMatrix[i][j] && isDown) {
        result = keyMatrix[i][j];
      }
      keyDownMatrix[i][j] = isDown;
    }
  }
  return result;
}

bool isKeyDown(int i, int j)
{
  bool result = false;
  digitalWrite(rowPins[i], LOW);
  if (digitalRead(colPins[j]) == LOW) {
    result = true;
  }
  digitalWrite(rowPins[i], HIGH);
  return result;
}
