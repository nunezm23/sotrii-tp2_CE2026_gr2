# TP2 - Actividad 04

## Compilacion y depuracion

La aplicacion se compila desde la configuracion `Debug` de STM32CubeIDE para
la placa NUCLEO-F446RE. La verificacion funcional en placa fue confirmada
durante la ejecucion de pruebas de BTN_A y BTN_B.

La traza se obtiene por semihosting mediante `LOGGER_INFO`. Los mensajes
relevantes permiten seguir el recorrido completo:

`BTN_x sent event` -> `SYS received` -> clasificacion de la pulsacion ->
`LED_x received event`.

## Comportamiento observado

| Estimulo | Resultado esperado y observado |
| --- | --- |
| BTN_A, pulsacion menor que 1 s | LED_A parpadea y LED_C permanece encendido 5 s. |
| BTN_A, pulsacion de 1 s o mayor | Se detiene cualquier parpadeo y LED_C permanece encendido el tiempo medido. |
| BTN_B, pulsacion menor que 1 s | LED_B parpadea y LED_C permanece encendido 10 s. |
| BTN_B, pulsacion de 1 s o mayor | Se detiene cualquier parpadeo y LED_C permanece encendido el tiempo medido. |
| Nueva pulsacion con LED_C encendido | SYS reinicia el temporizador de LED_C sin bloquear la tarea. |

El umbral se define en `task_sys.c` como `DEL_BTN_LONG` y vale 1000 ms.

## Medicion WCET de interfaces LED

Las cuatro interfaces del driver LED estan instrumentadas con el contador de
ciclos DWT del Cortex-M4. El contador se inicia antes de abrir los Active
Objects, por lo que tambien se mide `open_led_ao()`.

| Interfaz | Ultima muestra (ciclos) | WCET acumulado (ciclos) | WCET (us) | Variable de depuracion |
| --- | ---: | ---: | ---: | --- |
| `open_led_ao()` | 9896 | 9896 | 117.81 | `g_led_ao_wcet` |
| `send_led_ao()` | 12207 | 14923 | 177.65 | `g_led_ao_wcet` |
| `ioctl_led_ao()` | 0 | 0 | No ejecutada | `g_led_ao_wcet` |
| `release_led_ao()` | No medida | No medida | No medida | `g_led_ao_wcet` |

Para completar las interfaces no ejercitadas, detener el depurador luego de
ejecutar varias veces cada escenario y copiar los campos `*_max` de
`g_led_ao_wcet`. El tiempo se obtiene como:

`tiempo [us] = ciclos / 84`


