Generador de Señales por Bluetooth con ESP32
Este proyecto implementa un generador de señales mediante un ESP32, controlado de forma remota por comandos enviados vía Bluetooth. El sistema permite seleccionar diferentes formas de onda, su frecuencia, amplitud y duty cycle (para la onda cuadrada).

Características principales
Conectividad Bluetooth para recibir comandos desde un celular, computadora o terminal Bluetooth.

Salida de señal por GPIO2 usando PWM (modulación por ancho de pulso).

Generación de las siguientes formas de onda:

Onda cuadrada

Onda senoidal

Onda triangular

Onda simulada de ECG (electrocardiograma)

Control dinámico de parámetros:

Frecuencia (hasta 5000 Hz)

Amplitud (hasta 3.3V simulados con duty)

Duty cycle (solo para onda cuadrada)

Comandos Bluetooth disponibles
WAVE_ON – Activa la salida de onda.

WAVE_OFF – Desactiva la salida.

SQUARE_WAVE – Selecciona onda cuadrada.

SINE_WAVE – Selecciona onda senoidal.

TRI_WAVE – Selecciona onda triangular.

ECG_WAVE – Selecciona la onda ECG precargada.

FREC:<valor> – Establece la frecuencia (1 a 5000 Hz).

AMP:<valor> – Establece la amplitud simulada (0 a 3.3 V).

DUTY:<valor> – Establece el duty cycle (1 a 99 %, solo para onda cuadrada).

Tecnologías utilizadas
Lenguaje: C++ (Arduino Framework)

Microcontrolador: ESP32

Comunicación: BluetoothSerial

Salida de onda: PWM por GPIO2 usando ledcWrite

Archivos importantes
sineTable[], triTable[], ecgTable[]: Tablas de muestra para la generación digital de las ondas.

Funciones generateXWave(): Calculan y escriben los valores de PWM en tiempo real según la tabla correspondiente.

parseCommand(): Interpreta los comandos recibidos por Bluetooth.

Notas
El valor de amplitud no ajusta un DAC físico, sino que modifica el duty del PWM, simulando un cambio de nivel.

El código incluye validaciones para evitar división por cero cuando la frecuencia es 0.

La onda ECG es una secuencia fija precargada, simulando un latido cardíaco.

