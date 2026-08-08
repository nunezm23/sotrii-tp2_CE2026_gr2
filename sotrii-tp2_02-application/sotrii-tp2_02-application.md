# TP2 - Actividad 02
## Implementación del Active Object **Sys**

**Alumno:** Sebastián Pinto

---

# Objetivo

El objetivo de esta actividad fue transformar el módulo **System (Sys)** en un **Active Object (AO)**, encapsulando completamente sus recursos internos y eliminando el acceso directo a la cola de eventos desde otros módulos del sistema.

La idea principal consiste en que cada Active Object sea responsable de administrar su propia tarea, su cola de mensajes y su estado interno, permitiendo que la comunicación entre módulos se realice únicamente mediante funciones de interfaz.

---

# Implementación

Para realizar esta actividad se modificó la arquitectura existente del proyecto siguiendo el patrón **Active Object**.

## Estructura del Active Object

Se amplió la estructura `h_sys_t` incorporando los elementos necesarios para administrar el Active Object.

La estructura quedó compuesta por:

- Identificador del Active Object (`ao_id`)
- Cola privada (`ao_queue`)
- Handle de la tarea (`ao_task`)
- Máquina de estados (`sys_sc`)

De esta manera todos los recursos necesarios para el funcionamiento del AO quedan encapsulados dentro de la misma estructura.

---

## Cola privada

Anteriormente el sistema utilizaba una cola global compartida (`h_sys_task_q`).

Durante esta actividad dicha cola fue eliminada.

Ahora cada instancia del Active Object crea y administra su propia cola mediante el miembro:

```c
ao_queue
```

Con esto se evita que otros módulos accedan directamente al mecanismo interno de comunicación.

---

## Interfaz pública

Se implementó el archivo de interfaz correspondiente al Active Object Sys.

Las funciones disponibles son:

- `open_sys_ao()`
- `send_sys_ao()`
- `release_sys_ao()`
- `ioctl_sys_ao()`

Estas funciones constituyen la única forma de interactuar con el Active Object desde el resto del sistema.

---

## Inicialización

La función `open_sys_ao()` realiza toda la inicialización del Active Object.

Entre las tareas realizadas se encuentran:

- Creación de la cola privada.
- Registro de la cola para depuración.
- Creación de la tarea Gatekeeper.
- Inicialización de los recursos internos.

Con esta implementación el archivo `app.c` únicamente debe llamar a:

```c
open_sys_ao(&h_sys);
```

sin conocer cómo se implementa internamente el Active Object.

---

## Gatekeeper

La tarea `task_sys()` pasó a comportarse como la Gatekeeper del Active Object.

Su funcionamiento consiste en:

1. Esperar mensajes en su cola privada.
2. Actualizar la máquina de estados.
3. Ejecutar las acciones correspondientes.
4. Esperar el siguiente período de ejecución.

La tarea ya no accede a recursos globales para obtener eventos.

---

## Comunicación entre módulos

El módulo Button dejó de acceder directamente a la cola del sistema.

Anteriormente se utilizaba:

```c
xQueueSend(...)
```

sobre una cola global.

Después de la modificación la comunicación se realiza mediante:

```c
send_sys_ao(...)
```

Con esto el módulo Button desconoce completamente cómo está implementado internamente el Active Object Sys.

---

# Cambios realizados

Durante la actividad se realizaron las siguientes modificaciones:

- Conversión del módulo Sys a Active Object.
- Eliminación de la cola global.
- Implementación de una cola privada para Sys.
- Implementación de la interfaz pública del Active Object.
- Encapsulamiento de la tarea Gatekeeper.
- Adaptación del módulo Button para utilizar la interfaz pública.
- Modificación de `app.c` para inicializar el Active Object mediante `open_sys_ao()`.

---

# Resultados

Una vez implementados los cambios:

- El proyecto compiló correctamente.
- Se eliminó la dependencia de la cola global.
- La comunicación entre Button y System pasó a realizarse mediante la interfaz del Active Object.
- La arquitectura quedó desacoplada, facilitando futuras modificaciones sin afectar otros módulos.

---

# Conclusiones

La implementación del Active Object permitió encapsular completamente el módulo Sys, ocultando su implementación interna y reduciendo el acoplamiento entre componentes.

La utilización de una interfaz pública evita que otros módulos manipulen directamente las colas o la tarea del Active Object, favoreciendo una arquitectura más modular y escalable.

Este cambio constituye la base para las siguientes actividades, donde otros módulos del sistema también serán migrados al patrón Active Object.

