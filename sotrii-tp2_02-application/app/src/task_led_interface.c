/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
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
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
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
#include "app_it.h"
#include "task_led.h"
#include "task_led_attribute.h"

/********************** macros and definitions *******************************/
#define QUEUE_LENGTH__		(1)
#define QUEUE_ITEM_SIZE__	(sizeof(led_ev_t))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Interface functions */
void open_led_ao(h_led_t *h_led_)
{
	/* Before a queue or semaphore (binary or counting) or mutex is used it must
     * be explicitly created.
	 *
	 * Check the queue or semaphore (binary or counting) or mutex was created
     * successfully.
     *
     * Add queue or semaphore (binary or counting) or mutex to registry. */
	h_led_->led_ao->h_queue = xQueueCreate(QUEUE_LENGTH__, QUEUE_ITEM_SIZE__);
	configASSERT(NULL != h_led_->led_ao->h_queue);
	vQueueAddToRegistry(h_led_->led_ao->h_queue, h_led_->led_ao->queue_txt);

	/* Add threads, ... */
    BaseType_t ret;

    /* Task LED thread at priority 1 */
	ret = xTaskCreate(task_led,							/* Pointer to the function thats implement the task. */
					  h_led_->led_ao->task_txt,			/* Text name for the task. This is to facilitate debugging only. */
					  (configMINIMAL_STACK_SIZE),		/* Stack depth in words. */
					  (void *)h_led_,					/* We are using the task parameter. */
					  (tskIDLE_PRIORITY + 1ul),			/* This task will run at priority 1. */
					  &h_led_->led_ao->h_task);			/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Total amount of heap space that remains unallocated. Is also available
     * with xFreeBytesRemaining variable for heap management schemes 2 to 5.
     * Memory array used by heap_4 is specified as:
     * uint8_t ucHeap[configTOTAL_HEAP_SIZE]; */
    ret = xPortGetFreeHeapSize();
}

void release_led_ao(h_led_t *h_led_)
{
    vQueueUnregisterQueue(h_led_->led_ao->h_queue);
	vQueueDelete(h_led_->led_ao->h_queue);

	vTaskDelete(h_led_->led_ao->h_task);
}

BaseType_t send_led_ao(h_led_t *h_led_, void *event_)
{
	return xQueueSend((QueueHandle_t)h_led_->led_ao->h_queue, event_, (TickType_t)ZERO);
}

void ioctl_led_ao(h_led_t *h_led_)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(h_led_);
}

/********************** end of file ******************************************/
