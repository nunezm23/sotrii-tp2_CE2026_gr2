# Análisis y explicación del código fuente

## 1. Descripción general

El código implementa una aplicación embebida sobre **FreeRTOS** para controlar un LED mediante un botón.

La aplicación está dividida en tres tareas principales:

- `task_btn`: lee el estado del botón.
- `task_sys`: decide qué acción debe realizar el sistema.
- `task_led`: controla físicamente el LED.

Las tareas no se llaman directamente entre sí. Para comunicarse utilizan **colas de FreeRTOS**:

```text
Botón → task_btn → cola → task_sys → cola → task_led → LED
```

Cada vez que se presiona el botón, el LED cambia de comportamiento siguiendo esta secuencia:

```text
Apagado → Encendido → Parpadeando → Apagado
```

Después de apagarlo, una nueva pulsación vuelve a iniciar la secuencia.

---

## 2. Funcionamiento de `app.c`

El archivo `app.c` contiene la función `app_init()`, encargada de preparar la aplicación antes de que las tareas comiencen a trabajar.

### Inicialización de variables

Primero se inicializan varios contadores globales:

```c
g_app_tick_cnt = 0;
g_task_idle_cnt = 0;
g_app_stack_overflow_cnt = 0;
```

Estos contadores parecen estar destinados a registrar información de funcionamiento, como el número de ticks, la actividad de la tarea Idle o posibles desbordamientos de pila. En los archivos adjuntos solamente se observa su inicialización.

### Creación de las colas

Se crean dos colas.

La primera cola comunica la tarea del botón con la tarea del sistema:

```c
h_sys_task_q = xQueueCreate(5, sizeof(sys_ev_t));
```

Puede almacenar hasta cinco eventos.

La segunda cola comunica la tarea del sistema con la tarea del LED:

```c
h_led_task_q = xQueueCreate(1, sizeof(led_ev_t));
```

Puede almacenar una sola orden para el LED.

Después de crear cada cola se utiliza `configASSERT()` para comprobar que la creación fue exitosa. Si no existe memoria suficiente y una cola no puede crearse, la aplicación se detendrá en la aserción.

### Creación de las tareas

La función crea las siguientes tareas:

- `task_a`
- `task_b`
- `task_led`
- `task_sys`
- `task_btn`

Las tareas A y B tienen una prioridad mayor que las tareas del botón, sistema y LED. Sin embargo, sus archivos no fueron adjuntados, por lo que no se puede explicar su función.

A las tareas principales se les entrega un parámetro:

```c
task_led recibe &h_led
task_sys recibe &h_sys
task_btn recibe &h_btn
```

Ese parámetro permite que cada tarea acceda a sus estructuras de configuración y a su máquina de estados.

Finalmente, se inicializan las interrupciones de la aplicación y el contador de ciclos:

```c
app_it_init();
cycle_counter_init();
```

---

## 3. Funcionamiento de `task_btn_attribute.h`

Este archivo define los tipos de datos utilizados para representar los botones.

### Identificadores de botones

```c
BTN_A
BTN_B
BTN_QTY
```

Se consideran dos botones. `BTN_QTY` representa la cantidad total y se usa para dimensionar arreglos.

### Eventos del botón

```c
EV_BTN_UP
EV_BTN_DOWN
```

- `EV_BTN_UP`: el botón está liberado.
- `EV_BTN_DOWN`: el botón está presionado.

### Estados del botón

```c
ST_BTN_UP
ST_BTN_DOWN
```

Estos estados permiten recordar el estado anterior del botón. Gracias a esto, la tarea puede saber si ocurrió una transición real.

### Estructuras

`btn_t` almacena la información física del botón:

- identificador;
- puerto GPIO;
- pin;
- estado actual del pin.

`btn_sc_t` almacena la información de su máquina de estados:

- estado actual;
- evento de entrada;
- tiempo acumulado;
- evento de salida;
- duración del estado anterior.

`h_btn_t` relaciona el botón físico con su máquina de estados.

---

## 4. Funcionamiento de `task_btn.c`

Este archivo implementa la lectura del botón.

Se crean arreglos para dos botones y para sus respectivas máquinas de estados:

```c
btn_t btn[BTN_QTY];
btn_sc_t btn_sc[BTN_QTY];
h_btn_t h_btn[BTN_QTY];
```

Inicialmente, ambos botones se consideran liberados.

### Bucle principal

La función `task_btn()` se ejecuta continuamente.

Cada 50 ms realiza lo siguiente:

1. Incrementa el contador de ejecuciones.
2. Lee el GPIO mediante `HAL_GPIO_ReadPin()`.
3. Determina si el botón está presionado o liberado.
4. Ejecuta la máquina de estados.
5. Espera 50 ms mediante `vTaskDelay()`.

La lectura se convierte en uno de estos eventos:

```c
EV_BTN_DOWN
EV_BTN_UP
```

### Máquina de estados

Si el botón estaba liberado y ahora aparece presionado:

```text
ST_BTN_UP → ST_BTN_DOWN
```

La tarea genera `EV_BTN_DOWN` y lo envía a la cola `h_sys_task_q`.

Si el botón estaba presionado y ahora aparece liberado:

```text
ST_BTN_DOWN → ST_BTN_UP
```

La tarea genera `EV_BTN_UP` y también lo envía a la cola.

Mientras el botón permanece en el mismo estado, no se envían nuevos eventos. Esto evita enviar repetidamente el evento de pulsación mientras el usuario mantiene el botón presionado.

El muestreo cada 50 ms también ayuda a ignorar rebotes eléctricos muy rápidos, aunque no implementa un algoritmo de antirrebote completo.

---

## 5. Funcionamiento de `task_sys_attribute.h`

Este archivo define los eventos y estados generales del sistema.

### Eventos del sistema

```c
EV_SYS_OFF
EV_SYS_ON
EV_SYS_BLINK
EV_SYS_NONE
```

Los eventos del botón se relacionan numéricamente con los del sistema:

```c
EV_SYS_OFF = EV_BTN_UP
EV_SYS_ON  = EV_BTN_DOWN
```

Por esta razón, cuando `task_btn` envía `EV_BTN_DOWN`, `task_sys` lo interpreta como `EV_SYS_ON`.

### Estados del sistema

```c
ST_SYS_IDLE
ST_SYS_ACTIVE_0
ST_SYS_ACTIVE_1
```

Estos tres estados representan los tres modos del LED:

| Estado del sistema | Acción asociada |
|---|---|
| `ST_SYS_IDLE` | LED apagado |
| `ST_SYS_ACTIVE_0` | LED encendido |
| `ST_SYS_ACTIVE_1` | LED parpadeando |

La estructura `sys_sc_t` almacena el estado, el evento de entrada, el evento de salida y los contadores de tiempo.

---

## 6. Funcionamiento de `task_sys.c`

`task_sys` contiene la lógica principal de la aplicación.

La máquina de estados comienza en:

```c
ST_SYS_IDLE
```

### Recepción de eventos

Cada 50 ms, la tarea intenta leer la cola `h_sys_task_q`.

Si existe un evento, lo procesa. Si la cola está vacía, asigna:

```c
EV_SYS_NONE
```

La recepción no bloquea la tarea porque se utiliza un tiempo de espera de cero.

### Cambio de modo

La tarea solamente cambia de estado cuando recibe `EV_SYS_ON`, que corresponde a una nueva pulsación.

#### Primera pulsación

```text
ST_SYS_IDLE → ST_SYS_ACTIVE_0
```

La tarea envía `EV_SYS_ON` a la cola del LED. El resultado es que el LED se enciende.

#### Segunda pulsación

```text
ST_SYS_ACTIVE_0 → ST_SYS_ACTIVE_1
```

La tarea envía `EV_SYS_BLINK`. El LED comienza a parpadear.

#### Tercera pulsación

```text
ST_SYS_ACTIVE_1 → ST_SYS_IDLE
```

La tarea envía `EV_SYS_OFF`. El LED se apaga.

La liberación del botón llega como `EV_SYS_OFF`, pero no provoca un cambio directo en la máquina del sistema. Su función principal es permitir que `task_btn` detecte posteriormente una nueva pulsación.

---

## 7. Funcionamiento de `task_led_attribute.h`

Este archivo define los datos utilizados para controlar los LED.

### Identificadores

```c
LED_A
LED_B
LED_C
LED_QTY
```

Se definen tres LED.

### Eventos

```c
EV_LED_OFF
EV_LED_ON
EV_LED_BLINK
EV_LED_NONE
```

Estos eventos se relacionan directamente con los eventos enviados por `task_sys`.

### Estados

```c
ST_LED_OFF
ST_LED_ON
ST_LED_BLINK
```

La estructura `led_t` almacena el puerto, el pin y el estado del LED.

La estructura `led_sc_t` mantiene el estado de la máquina y un contador utilizado para controlar el parpadeo.

---

## 8. Funcionamiento de `task_led.c`

Este archivo controla el GPIO del LED.

Cada 50 ms, `task_led()` intenta recibir una orden desde `h_led_task_q`.

Si no recibe una orden, utiliza:

```c
EV_LED_NONE
```

Después ejecuta su máquina de estados.

### Apagar el LED

Al recibir `EV_LED_OFF`:

1. Cambia al estado `ST_LED_OFF`.
2. Asigna el nivel definido como `LED_OFF`.
3. Reinicia el contador.
4. Escribe el nivel en el GPIO con `HAL_GPIO_WritePin()`.

### Encender el LED

Al recibir `EV_LED_ON`:

1. Cambia al estado `ST_LED_ON`.
2. Asigna el nivel definido como `LED_ON`.
3. Reinicia el contador.
4. Escribe el nivel en el GPIO.

### Hacer parpadear el LED

Al recibir `EV_LED_BLINK`:

1. Cambia al estado `ST_LED_BLINK`.
2. Carga un contador de 500 ms.
3. Cambia inmediatamente el nivel del pin mediante `HAL_GPIO_TogglePin()`.

Mientras permanece en ese estado, la tarea resta 50 ms al contador en cada ejecución.

Cuando el contador llega a cero, vuelve a cargar 500 ms y alterna nuevamente el pin.

Por lo tanto:

- el LED cambia de nivel aproximadamente cada 500 ms;
- un ciclo completo de encendido y apagado dura aproximadamente un segundo.

---

## 9. Ejemplo completo de funcionamiento

Supongamos que inicialmente el LED está apagado.

### Primera pulsación

1. `task_btn` detecta el botón presionado.
2. Envía `EV_BTN_DOWN`.
3. `task_sys` lo interpreta como `EV_SYS_ON`.
4. Cambia a `ST_SYS_ACTIVE_0`.
5. Envía una orden de encendido.
6. `task_led` enciende el LED.

### Segunda pulsación

1. `task_btn` vuelve a detectar una transición de pulsación.
2. `task_sys` cambia a `ST_SYS_ACTIVE_1`.
3. Envía `EV_SYS_BLINK`.
4. `task_led` comienza a alternar el GPIO cada 500 ms.

### Tercera pulsación

1. `task_sys` regresa a `ST_SYS_IDLE`.
2. Envía `EV_SYS_OFF`.
3. `task_led` apaga el LED.

---

## 10. Observaciones importantes

Aunque el código define dos botones y tres LED, en `app.c` solamente se crea una tarea para el botón y otra para el LED.

Además, las tareas reciben `&h_btn` y `&h_led`, que apuntan al inicio de los arreglos. Como las tareas no recorren todos sus elementos, el comportamiento mostrado se aplica principalmente a:

```text
BTN_A
LED_A
```

También se observa que los envíos a las colas utilizan tiempo de espera cero. Si una cola estuviera llena, el evento podría no enviarse. El código no comprueba el resultado de `xQueueSend()`.

Estas condiciones no impiden comprender el ejemplo, pero serían importantes si la aplicación se ampliara para utilizar varios botones o varios LED.

---

## 11. Conclusión

El código presenta una arquitectura separada por responsabilidades:

- La tarea del botón detecta entradas.
- La tarea del sistema toma decisiones.
- La tarea del LED controla la salida física.

Las colas permiten que cada tarea funcione de manera independiente y que la información se transmita mediante eventos.

La secuencia funcional principal es:

```text
Pulsación 1: encender
Pulsación 2: parpadear
Pulsación 3: apagar
```

Este diseño es un ejemplo sencillo de un sistema dirigido por eventos implementado con tareas, colas y máquinas de estados en FreeRTOS.