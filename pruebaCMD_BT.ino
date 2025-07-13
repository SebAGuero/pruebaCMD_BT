#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8
#define MAX_FREQ 5000
#define PWM_PIN 2   // GPIO2

#define SINE_STEPS 256
uint8_t sineTable[SINE_STEPS];
int sineIndex = 0;
unsigned long lastMicros = 0;

#define TRI_STEPS 256
uint8_t triTable[TRI_STEPS];
int triIndex = 0;
unsigned long lastTriMicros = 0;

// Cambia este valor al total de elementos en ecgTable
#define ECG_STEPS 288
uint8_t ecgTable[ECG_STEPS] = {
  128,130,133,136,139,142,145,147,148,149,149,149,148,147,145,143,
  140,136,132,128,124,120,117,114,112,110,109,108,107,107,108,109,
  111,113,116,119,122,125,128,131,134,136,138,140,141,142,142,142,
  141,139,137,134,131,127,123,119,115,111,107,103,100, 97, 95, 93,
   92, 91, 91, 92, 93, 95, 97,100,103,106,110,114,118,122,126,
  130,134,138,141,144,147,149,151,152,152,151,149,146,143,139,134,
  129,124,118,113,108,103, 99, 95, 92, 90, 89, 89, 90, 92, 95,
   99,103,108,113,118,123,128,133,138,142,146,150,153,155,156,156,
  155,153,150,146,141,136,130,124,118,112,107,102, 98, 95, 92, 90,
   89, 89, 90, 91, 93, 95, 98,102,106,110,115,120,125,130,135,
  139,144,148,152,155,157,158,158,157,155,152,148,143,138,132,126,
  120,114,109,104,100, 97, 94, 92, 91, 90, 91, 92, 94, 97, 100,
  104,109,114,119,124,129,134,139,144,148,152,155,157,158,158,157,
  155,152,148,143,138,132,127,121,115,110,105,101, 97, 94, 92, 90,
   89, 89, 90, 92, 95, 98,102,106,110,115,120,125,130,135,139,
  144,148,152,155,157,158,158,157,155,152,148,143,138,132,127,121,
  115,110,105,101, 97, 94, 92, 90, 89, 89, 90, 92, 95, 98
};

int ecgIndex = 0;
unsigned long lastEcgMicros = 0;

bool waveOn = false;
String waveType = "SQUARE";
float amplitude = 3.3; // Valor default para ver bien la señal
int freq = 1000;        // Frecuencia default 1000Hz
int duty = 50;
bool pwmConfigured = false;

// Funciones para llenar las tablas de onda
void fillSineTable() {
  for (int i = 0; i < SINE_STEPS; i++) {
    float angle = 2.0 * PI * ((float)i / (float)SINE_STEPS);
    sineTable[i] = (uint8_t)((sin(angle) * 0.5 + 0.5) * (pow(2, PWM_RESOLUTION) - 1));
  }
}

void initTriTable() {
  for (int i = 0; i < TRI_STEPS; i++) {
    if (i < TRI_STEPS / 2) {
      triTable[i] = map(i, 0, TRI_STEPS / 2, 0, 255);
    } else {
      triTable[i] = map(i, TRI_STEPS / 2, TRI_STEPS - 1, 255, 0);
    }
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_WAVEGEN");
  Serial.println(">> ESP32 WAVEGEN listo para comandos vía Bluetooth.");
  Serial.println(">> Usando GPIO2 como salida.");
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);

  fillSineTable();
  initTriTable();
}

void loop() {
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();
    parseCommand(command);
  }

  if (waveOn) {
    // Si la frecuencia se puso en 0 por algún comando antes de la validación, o por un valor inicial mal seteado, aquí también se podría añadir una verificación
    // para evitar la división por cero antes de llamar a ledcSetup o a las funciones de onda.
  
    if (!pwmConfigured || (waveType != "SQUARE" && freq == 0)) {
        if (freq == 0) { // Asegura que frecq no sea 0 si es una onda que reqiere calculo de intervalo
            freq = 1; // Setea a un valor mínimo válido para evitar división por cero
            Serial.println("Advertencia: Frecuencia ajustada a 1Hz para evitar división por cero.");
            SerialBT.println("Advertencia: Frecuencia ajustada a 1Hz para evitar división por cero.");
        }
      ledcSetup(PWM_CHANNEL, freq, PWM_RESOLUTION);
      pwmConfigured = true;
    }

    if (waveType == "SQUARE") generateSquareWave();
    else if (waveType == "SINE") generateSineWave();
    else if (waveType == "TRI") generateTriWave();
    else if (waveType == "ECG") generateECGWave();
  } else {
    ledcWrite(PWM_CHANNEL, 0);
  }
}

void generateSquareWave() {
  int max_duty = pow(2, PWM_RESOLUTION) - 1;
  int amp_duty = int((amplitude / 3.3) * max_duty);
  int duty_val = int((duty / 100.0) * amp_duty);
  ledcWrite(PWM_CHANNEL, duty_val);
}

void generateSineWave() {
  // Asegurarse que freq * SINE_STEPS no sea 0
  if (freq == 0) return; // Ya lo validamos en parseCommand y loop, pero un doble check no está de más
  unsigned long now = micros();
  unsigned long interval = 1000000UL / ((unsigned long)freq * SINE_STEPS); // Convertir a unsigned long para la operación
  if (now - lastMicros >= interval) {
    lastMicros = now;
    int maxDuty = pow(2, PWM_RESOLUTION) - 1;
    int baseValue = sineTable[sineIndex];
    int val = (int)((baseValue / (float)maxDuty) * ((amplitude / 3.3) * maxDuty));
    ledcWrite(PWM_CHANNEL, val);
    sineIndex = (sineIndex + 1) % SINE_STEPS;
  }
}

void generateTriWave() {
  // Asegurarse que freq * TRI_STEPS no sea 0
  if (freq == 0) return; // Ya lo validamos en parseCommand y loop, pero un doble check no está de más
  unsigned long now = micros();
  unsigned long interval = 1000000UL / ((unsigned long)freq * TRI_STEPS); // Convertir a unsigned long para la operación
  if (now - lastTriMicros >= interval) {
    lastTriMicros = now;
    int maxDuty = pow(2, PWM_RESOLUTION) - 1;
    int baseValue = triTable[triIndex];
    int val = (int)((baseValue / 255.0) * ((amplitude / 3.3) * maxDuty));
    ledcWrite(PWM_CHANNEL, val);
    triIndex = (triIndex + 1) % TRI_STEPS;
  }
}

void generateECGWave() {
  // Asegurarse que freq * ECG_STEPS no sea 0
  if (freq == 0) return; //doble validasion
  unsigned long now = micros();
  unsigned long interval = 1000000UL / ((unsigned long)freq * ECG_STEPS); // Convertir a unsigned long para la operación
  if (now - lastEcgMicros >= interval) {
    lastEcgMicros = now;
    int maxDuty = pow(2, PWM_RESOLUTION) - 1;
    int baseValue = ecgTable[ecgIndex];
    int val = (int)((baseValue / 255.0) * ((amplitude / 3.3) * maxDuty));
    ledcWrite(PWM_CHANNEL, val);
    ecgIndex = (ecgIndex + 1) % ECG_STEPS;
  }
}

void parseCommand(String cmd) {
  cmd.toUpperCase();
  Serial.println("Comando recibido: " + cmd);

  if (cmd == "WAVE_ON") {
    waveOn = true;
    pwmConfigured = false;
    SerialBT.println(">> Onda ACTIVADA");
    Serial.println(">> Onda ACTIVADA");
  }
  else if (cmd == "WAVE_OFF") {
    waveOn = false;
    ledcWrite(PWM_CHANNEL, 0);
    SerialBT.println(">> Onda DESACTIVADA");
    Serial.println(">> Onda DESACTIVADA");
  }
  else if (cmd.startsWith("AMP:")) {
    float val = cmd.substring(4).toFloat();
    if (val >= 0 && val <= 3.3) {
      amplitude = val;
      SerialBT.printf(">> Amplitud seteada a %.2f V\n", amplitude);
      Serial.printf(">> Amplitud seteada a %.2f V\n", amplitude);
    } else {
      SerialBT.println("⚠️ Valor AMP inválido. Debe ser entre 0 y 3.3");
    }
  }
  else if (cmd.startsWith("FREC:")) {
    int val = cmd.substring(5).toInt();
    // --- MODIFICACIÓN: evita la división por cero ---
    if (val >= 1 && val <= MAX_FREQ) {
      freq = val;
      pwmConfigured = false;
      SerialBT.printf(">> Frecuencia seteada a %d Hz\n", freq);
      Serial.printf(">> Frecuencia seteada a %d Hz\n", freq);
    } else {
      SerialBT.printf("⚠️ Valor FREC inválido. Debe ser entre 1 y %d Hz\n", MAX_FREQ);
    }
  }
  else if (cmd.startsWith("DUTY:")) {
    int val = cmd.substring(5).toInt();
    if (val >= 1 && val <= 99) {
      duty = val;
      SerialBT.printf(">> Duty cycle seteado a %d %%\n", duty);
      Serial.printf(">> Duty cycle seteado a %d %%\n", duty);
    } else {
      SerialBT.println("⚠️ Valor DUTY inválido. Debe ser entre 1 y 99.");
    }
  }
  else if (cmd == "SQUARE_WAVE") {
    waveType = "SQUARE";
    SerialBT.println(">> Onda cuadrada seleccionada");
    Serial.println(">> Onda cuadrada seleccionada");
  }
  else if (cmd == "SINE_WAVE") {
    waveType = "SINE";
    SerialBT.println(">> Onda senoidal seleccionada");
    Serial.println(">> Onda senoidal seleccionada");
  }
  else if (cmd == "TRI_WAVE") {
    waveType = "TRI";
    SerialBT.println(">> Onda triangular seleccionada");
    Serial.println(">> Onda triangular seleccionada");
  }
  else if (cmd == "ECG_WAVE") {
    waveType = "ECG";
    SerialBT.println(">> Onda ECG seleccionada");
    Serial.println(">> Onda ECG seleccionada");
  }
  else {
    SerialBT.println("⚠️ Comando desconocido");
    Serial.println("⚠️ Comando desconocido");
  }
}

