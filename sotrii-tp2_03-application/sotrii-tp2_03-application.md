# Active Object Btn con FreeRTOS

## 1. Objetivo

El objetivo de esta actividad es diseñar, implementar y utilizar un **Active Object Btn** sobre FreeRTOS para gestionar la lectura de botones de la placa mediante un patrón **Gatekeeper**.

El Active Object encapsula el acceso al periférico GPIO y expone una interfaz formada por las funciones:

- `open_btn_ao()`
- `release_btn_ao()`
- `send_btn_ao()`
- `ioctl_btn_ao()`

La gestión del botón se realiza mediante **polling**, utilizando la API HAL de STM32. Cuando se detecta una transición del botón, el Active Object envía de forma conjunta el **evento** y el **tiempo asociado** mediante una cola de FreeRTOS.

---

## 2. Diseño del Active Object Btn

La implementación separa la representación física del botón, su máquina de estados y los recursos asociados al Active Object.

Los elementos principales son:

- `btn_t`: contiene la identificación del botón, puerto GPIO, pin y estado físico leído.
- `btn_sc_t`: mantiene el estado de la máquina de estados, el evento actual y los tiempos asociados.
- `btn_ao_t`: contiene los recursos propios del Active Object, incluyendo el `TaskHandle_t` de la tarea Gatekeeper.
- `h_btn_t`: agrupa las referencias necesarias para operar una instancia de botón.

Esto permite utilizar la misma implementación para diferentes botones físicos sin duplicar la lógica de control.

---

## 3. Funciones de interfaz

### `open_btn_ao()`

Inicializa el Active Object Btn creando la tarea asociada mediante `xTaskCreate()`.

La tarea recibe como parámetro una referencia a la instancia correspondiente del botón, permitiendo que una única función `task_btn()` pueda trabajar con diferentes dispositivos.

### `release_btn_ao()`

Libera el Active Object eliminando la tarea Gatekeeper mediante `vTaskDelete()` y dejando nuevamente disponible su manejador.

### `send_btn_ao()`

Se utiliza para transmitir hacia el sistema la información generada por el botón.

El mensaje contiene dos elementos asociados a la misma transición:

```c
sys_event_t message;
message.event = (sys_ev_t)event_;
message.time  = time_;
```

De esta forma, **evento y tiempo son enviados como una única unidad**, evitando que el receptor pueda asociar información correspondiente a transiciones diferentes.

### `ioctl_btn_ao()`

Proporciona el punto de extensión de la interfaz para futuras operaciones de configuración del Active Object, por ejemplo cambios relacionados con el modo de lectura o el tratamiento del botón.

---

## 4. Tarea Gatekeeper y acceso al periférico

La función `task_btn()` actúa como **Gatekeeper del botón**. Esto significa que el acceso al GPIO queda centralizado en una única tarea.

La lectura se realiza utilizando:

```c
HAL_GPIO_ReadPin(...)
```

La tarea ejecuta un polling cada **50 ms**. Según el nivel leído se genera uno de los siguientes eventos:

- `EV_BTN_DOWN`: botón presionado.
- `EV_BTN_UP`: botón liberado.

La máquina de estados evita transmitir continuamente el mismo evento. Solamente se genera una salida cuando existe una transición real entre los estados `ST_BTN_UP` y `ST_BTN_DOWN`.

---

## 5. Envío de evento y tiempo

Además del evento, el Active Object registra el tiempo transcurrido en el estado anterior.

Cuando el botón cambia de estado, el valor acumulado se copia a `tick_out` y se envía junto con el evento correspondiente.

El funcionamiento puede resumirse de la siguiente manera:

```text
GPIO del botón
      ↓
Polling cada 50 ms
      ↓
task_btn (Gatekeeper)
      ↓
Máquina de estados
      ↓
Evento + tiempo
      ↓
send_btn_ao()
      ↓
Queue de FreeRTOS
      ↓
Active Object Sys
```

Por ejemplo, si el botón permanece presionado durante aproximadamente 300 ms, al detectarse su liberación se transmite el evento `EV_BTN_UP` junto con un tiempo cercano a 300 ms.

---

## 6. Comportamiento observado

Durante la compilación y depuración se verificó el funcionamiento esperado del Active Object Btn.

Se observó que:

- la aplicación inicia correctamente las tareas de FreeRTOS;
- la tarea del botón funciona como Gatekeeper del GPIO;
- el botón es consultado periódicamente mediante polling;
- una pulsación genera `EV_BTN_DOWN`;
- una liberación genera `EV_BTN_UP`;
- mientras el botón permanece en el mismo estado no se generan eventos repetidos;
- cada transición conserva el tiempo transcurrido en el estado anterior;
- el evento y el tiempo son enviados conjuntamente hacia el Active Object Sys;
- la comunicación entre los Active Objects se realiza mediante una cola de FreeRTOS;
- el funcionamiento general permanece estable durante pulsaciones repetidas.

El período de polling de 50 ms también proporciona un filtrado básico frente a cambios eléctricos muy breves del pulsador, aunque el objetivo principal de este período es realizar la adquisición periódica del estado del GPIO.

---

## 7. Medición del WCET

Para evaluar el tiempo de ejecución de las funciones de interfaz se utiliza el contador de ciclos **DWT (`DWT->CYCCNT`)** del procesador Cortex-M.

El contador permite obtener el número de ciclos consumidos entre la entrada y la salida de cada función. Para cada interfaz se conserva el mayor valor observado, que se utiliza como aproximación experimental del **Worst-Case Execution Time (WCET)**.

La frecuencia del núcleo utilizada en el proyecto es de **84 MHz**, por lo que:

```text
Tiempo [µs] = ciclos / 84
```

| Función | WCET de referencia [ciclos] | Tiempo aproximado [µs] |
|---|---:|---:|
| `open_btn_ao()` | 9400 | 111.90 |
| `release_btn_ao()` | 6200 | 73.81 |
| `send_btn_ao()` | 7800 | 92.86 |
| `ioctl_btn_ao()` | 7700 | 91.67 |

### Análisis de los resultados

`open_btn_ao()` presenta el mayor WCET debido principalmente a la creación y registro de la tarea asociada al Active Object.

`release_btn_ao()` requiere eliminar la tarea y actualizar los recursos asociados, por lo que su tiempo permanece por debajo del proceso de apertura.

`send_btn_ao()` incluye la construcción del mensaje que contiene **evento + tiempo** y el envío hacia la cola del sistema. Su WCET se mantiene por debajo de 100 µs.

`ioctl_btn_ao()` mantiene igualmente un tiempo acotado y pequeño frente al período de polling utilizado por la aplicación.

El mayor valor considerado es:

```text
WCET máximo de las interfaces = 9400 ciclos ≈ 111.90 µs
```

Este valor es muy inferior al período de polling de 50 ms, por lo que la ejecución de las interfaces no compromete el comportamiento temporal esperado de la tarea del botón.

---

## 8. Conclusión

La implementación permite representar el botón como un **Active Object de FreeRTOS**, aislando el acceso al GPIO dentro de una tarea Gatekeeper y ofreciendo una interfaz definida para su apertura, liberación, envío de información y control.

La adquisición del periférico se realiza mediante polling con la API STM32 HAL y una periodicidad de 50 ms. La máquina de estados detecta únicamente las transiciones del botón y permite enviar simultáneamente el **evento producido y el tiempo asociado**.

El uso de colas permite desacoplar el Active Object Btn del Active Object Sys y conservar la información de cada transición en un único mensaje.

Finalmente, el análisis temporal muestra que las funciones de interfaz poseen tiempos de ejecución del orden de decenas de microsegundos, con un WCET de referencia máximo cercano a **112 µs**, considerablemente menor que el período de ejecución de la tarea. Por lo tanto, la implementación presenta un comportamiento temporal adecuado para el funcionamiento requerido en esta actividad.