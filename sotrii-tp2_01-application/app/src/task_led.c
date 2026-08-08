/*
 * Copyright (c) 2026 Sebastian Bedin <sebabedin@gmail.com> &
 * 					  Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Sebastian Bedin <sebabedin@gmail.com> &
 * 			 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "task_led_attribute.h"

/********************** macros and definitions *******************************/
#define G_TASK_LED_CNT_INI	0ul

#define DEL_LED_MIN			(pdMS_TO_TICKS(50ul))
#define DEL_LED_BLINK		(pdMS_TO_TICKS(500ul))

#define TASK_LED_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_LED_DEL_MAX	DEL_LED_MIN

#define LED_AO_QUEUE_LENGTH	(4ul)
#define LED_AO_TASK_PRIORITY	(tskIDLE_PRIORITY + 1ul)
#define LED_AO_WAIT_FOREVER	(portMAX_DELAY)

/********************** internal data declaration ****************************/
led_t led[LED_QTY] = {{LED_A, LED_A_PORT, LED_A_PIN, LED_A_OFF},
			     	  {LED_B, LED_B_PORT, LED_B_PIN, LED_B_OFF},
					  {LED_C, LED_C_PORT, LED_C_PIN, LED_C_OFF}};

led_sc_t led_sc[LED_QTY] = {{ST_LED_OFF, EV_LED_NONE, ZERO},
							{ST_LED_OFF, EV_LED_NONE, ZERO},
							{ST_LED_OFF, EV_LED_NONE, ZERO}};

/********************** internal functions declaration ***********************/
void task_led_statechart(h_led_t *h_led_);
static void led_ao_record(volatile uint32_t *last, volatile uint32_t *maximum,
					  uint32_t start);

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/
uint32_t g_task_led_cnt;

led_ao_t led_ao = {0};
volatile led_ao_wcet_t g_led_ao_wcet = {0};

h_led_t h_led[LED_QTY] = {{&led[LED_A], &led_sc[LED_A]},
				    	  {&led[LED_B], &led_sc[LED_B]},
						  {&led[LED_C], &led_sc[LED_C]}};

/********************** external functions definition ************************/
/* Task thread */
void task_led(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_led_cnt = G_TASK_LED_CNT_INI;
	led_ao_t *ao = (led_ao_t *)parameters;
	led_ao_msg_t message;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
    {
		/* Update Task Counter */
		g_task_led_cnt++;

		/* The Gatekeeper is the only task that accesses the LED GPIO. A timed
		 * receive implements the 50 ms polling required by the blink state. */
		if (pdPASS == xQueueReceive(ao->ao_queue, &message, TASK_LED_DEL_MAX))
		{
			if (pdTRUE == message.release)
			{
				HAL_GPIO_WritePin(ao->device->led->gpio_port,
								  ao->device->led->pin, LED_OFF);
				xSemaphoreGive(ao->ao_done);
				vTaskDelete(NULL);
			}

			ao->device->led_sc->ev_in = message.event;
		}
		else
		{
			ao->device->led_sc->ev_in = EV_LED_NONE;
		}

		/* Run Statechart */
		task_led_statechart(ao->device);

		/* Complete the synchronous interface call after operating the device. */
		if (EV_LED_NONE != ao->device->led_sc->ev_in)
		{
			xSemaphoreGive(ao->ao_done);
		}
	}
}

led_ao_t *open_led_ao(led_ao_id_t ao_id, h_led_t *device)
{
	uint32_t start = cycle_counter_get();
	BaseType_t ret;

	if ((ao_id >= LED_AO_ID_QTY) || (NULL == device) || (pdTRUE == led_ao.is_open))
	{
		led_ao_record(&g_led_ao_wcet.open_last, &g_led_ao_wcet.open_max, start);
		return NULL;
	}

	led_ao.ao_queue = xQueueCreate(LED_AO_QUEUE_LENGTH, sizeof(led_ao_msg_t));
	led_ao.ao_lock = xSemaphoreCreateMutex();
	led_ao.ao_done = xSemaphoreCreateBinary();
	if ((NULL == led_ao.ao_queue) || (NULL == led_ao.ao_lock) ||
		(NULL == led_ao.ao_done))
	{
		if (NULL != led_ao.ao_queue) { vQueueDelete(led_ao.ao_queue); }
		if (NULL != led_ao.ao_lock) { vSemaphoreDelete(led_ao.ao_lock); }
		if (NULL != led_ao.ao_done) { vSemaphoreDelete(led_ao.ao_done); }
		led_ao.ao_queue = NULL;
		led_ao.ao_lock = NULL;
		led_ao.ao_done = NULL;
		led_ao_record(&g_led_ao_wcet.open_last, &g_led_ao_wcet.open_max, start);
		return NULL;
	}

	led_ao.ao_id = ao_id;
	led_ao.device = device;
	led_ao.is_open = pdTRUE;
	vQueueAddToRegistry(led_ao.ao_queue, "LED AO Queue");

	ret = xTaskCreate(task_led, "LED Gatekeeper", configMINIMAL_STACK_SIZE,
				  &led_ao, LED_AO_TASK_PRIORITY, &led_ao.ao_task);
	if (pdPASS != ret)
	{
		vQueueDelete(led_ao.ao_queue);
		vSemaphoreDelete(led_ao.ao_lock);
		vSemaphoreDelete(led_ao.ao_done);
		led_ao.ao_queue = NULL;
		led_ao.ao_lock = NULL;
		led_ao.ao_done = NULL;
		led_ao.device = NULL;
		led_ao.is_open = pdFALSE;
		led_ao_record(&g_led_ao_wcet.open_last, &g_led_ao_wcet.open_max, start);
		return NULL;
	}

	led_ao_record(&g_led_ao_wcet.open_last, &g_led_ao_wcet.open_max, start);
	return &led_ao;
}

BaseType_t send_led_ao(led_ao_t *ao, led_ev_t event)
{
	uint32_t start = cycle_counter_get();
	led_ao_msg_t message = {event, pdFALSE};
	BaseType_t ret = pdFAIL;

	if ((NULL != ao) && (pdTRUE == ao->is_open) && (event < EV_LED_NONE) &&
		(pdPASS == xSemaphoreTake(ao->ao_lock, LED_AO_WAIT_FOREVER)))
	{
		if ((pdPASS == xQueueSend(ao->ao_queue, &message, LED_AO_WAIT_FOREVER)) &&
			(pdPASS == xSemaphoreTake(ao->ao_done, LED_AO_WAIT_FOREVER)))
		{
			ret = pdPASS;
		}
		xSemaphoreGive(ao->ao_lock);
	}

	led_ao_record(&g_led_ao_wcet.send_last, &g_led_ao_wcet.send_max, start);
	return ret;
}

BaseType_t ioctl_led_ao(led_ao_t *ao, led_ao_ioctl_t request)
{
	uint32_t start = cycle_counter_get();
	BaseType_t ret = pdFAIL;

	switch (request)
	{
		case LED_AO_IOCTL_OFF:  ret = send_led_ao(ao, EV_LED_OFF); break;
		case LED_AO_IOCTL_ON:   ret = send_led_ao(ao, EV_LED_ON); break;
		case LED_AO_IOCTL_BLINK: ret = send_led_ao(ao, EV_LED_BLINK); break;
		default: break;
	}

	led_ao_record(&g_led_ao_wcet.ioctl_last, &g_led_ao_wcet.ioctl_max, start);
	return ret;
}

BaseType_t release_led_ao(led_ao_t *ao)
{
	uint32_t start = cycle_counter_get();
	led_ao_msg_t message = {EV_LED_OFF, pdTRUE};
	BaseType_t ret = pdFAIL;

	if ((NULL != ao) && (pdTRUE == ao->is_open) &&
		(pdPASS == xSemaphoreTake(ao->ao_lock, LED_AO_WAIT_FOREVER)))
	{
		if ((pdPASS == xQueueSend(ao->ao_queue, &message, LED_AO_WAIT_FOREVER)) &&
			(pdPASS == xSemaphoreTake(ao->ao_done, LED_AO_WAIT_FOREVER)))
		{
			ao->is_open = pdFALSE;
			ao->ao_task = NULL;
			ret = pdPASS;
		}
		xSemaphoreGive(ao->ao_lock);
		vQueueDelete(ao->ao_queue);
		vSemaphoreDelete(ao->ao_done);
		vSemaphoreDelete(ao->ao_lock);
		ao->ao_queue = NULL;
		ao->ao_done = NULL;
		ao->ao_lock = NULL;
		ao->device = NULL;
	}

	led_ao_record(&g_led_ao_wcet.release_last, &g_led_ao_wcet.release_max, start);
	return ret;
}

static void led_ao_record(volatile uint32_t *last, volatile uint32_t *maximum,
					  uint32_t start)
{
	uint32_t elapsed = cycle_counter_get() - start;
	*last = elapsed;
	if (elapsed > *maximum)
	{
		*maximum = elapsed;
	}
}

void task_led_statechart(h_led_t *h_led_)
{
	switch (h_led_->led_sc->state)
	{
		case ST_LED_OFF:
		case ST_LED_ON:

			switch (h_led_->led_sc->ev_in)
			{
				case EV_LED_OFF:

					h_led_->led_sc->state = ST_LED_OFF;
					h_led_->led->pin_state = LED_OFF;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led_->led->gpio_port, h_led_->led->pin, h_led_->led->pin_state);

					break;

				case EV_LED_ON:

					h_led_->led_sc->state = ST_LED_ON;
					h_led_->led->pin_state = LED_ON;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led_->led->gpio_port, h_led_->led->pin, h_led_->led->pin_state);

					break;

				case EV_LED_BLINK:

					h_led_->led_sc->state = ST_LED_BLINK;
					h_led_->led->pin_state = HAL_GPIO_ReadPin(h_led_->led->gpio_port, h_led_->led->pin);
					h_led_->led_sc->tick = DEL_LED_BLINK;

					HAL_GPIO_TogglePin(h_led_->led->gpio_port, h_led_->led->pin);

					break;

				case EV_LED_NONE:

					break;
			}

			break;

		case ST_LED_BLINK:

			switch (h_led_->led_sc->ev_in)
			{
				case EV_LED_OFF:

					h_led_->led_sc->state = ST_LED_OFF;
					h_led_->led->pin_state = LED_OFF;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led_->led->gpio_port, h_led_->led->pin, h_led_->led->pin_state);

					break;

				case EV_LED_ON:

					h_led_->led_sc->state = ST_LED_ON;
					h_led_->led->pin_state = LED_ON;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led_->led->gpio_port, h_led_->led->pin, h_led_->led->pin_state);

					break;

				case EV_LED_BLINK:
				case EV_LED_NONE:

					h_led_->led_sc->state = ST_LED_BLINK;
					h_led_->led_sc->tick -= DEL_LED_MIN;

					if (ZERO == h_led_->led_sc->tick)
					{
						h_led_->led->pin_state = HAL_GPIO_ReadPin(h_led_->led->gpio_port, h_led_->led->pin);
						h_led_->led_sc->tick = DEL_LED_BLINK;

						HAL_GPIO_TogglePin(h_led_->led->gpio_port, h_led_->led->pin);
					}

					break;
			}

			break;
	}
}

/********************** end of file ******************************************/
