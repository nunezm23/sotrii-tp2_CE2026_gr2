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
#include "task_sys.h"
#include "task_sys_attribute.h"
#include "task_sys_interface.h"

/********************** macros and definitions *******************************/
#define SYS_QUEUE_LENGTH		(1ul)
#define SYS_QUEUE_ITEM_SIZE	(sizeof(sys_event_t))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/
volatile sys_ao_wcet_t g_sys_ao_wcet = {0ul, 0ul, 0ul, 0ul};

static void update_wcet(volatile uint32_t *maximum_, uint32_t start_)
{
	uint32_t elapsed = cycle_counter_get() - start_;

	if (elapsed > *maximum_)
	{
		*maximum_ = elapsed;
	}
}

/********************** external functions definition ************************/
/* Interface functions */
void open_sys_ao(h_sys_t *h_sys_)
{
	uint32_t start = cycle_counter_get();
	BaseType_t ret;

	configASSERT(NULL != h_sys_);
	configASSERT(NULL != h_sys_->sys_ao);
	configASSERT(NULL == h_sys_->sys_ao->h_queue);
	configASSERT(NULL == h_sys_->sys_ao->h_task);

	/* A queue with one element plus xQueueOverwrite implements Latest Input
	 * Only: a slow consumer always receives the most recent complete input. */
	h_sys_->sys_ao->h_queue = xQueueCreate(SYS_QUEUE_LENGTH,
										 SYS_QUEUE_ITEM_SIZE);
	configASSERT(NULL != h_sys_->sys_ao->h_queue);
	vQueueAddToRegistry(h_sys_->sys_ao->h_queue, h_sys_->sys_ao->queue_txt);

	ret = xTaskCreate(task_sys, h_sys_->sys_ao->task_txt,
				  configMINIMAL_STACK_SIZE, h_sys_, tskIDLE_PRIORITY + 1ul,
				  &h_sys_->sys_ao->h_task);
	configASSERT(pdPASS == ret);

	update_wcet(&g_sys_ao_wcet.open_cycles_max, start);
}

void release_sys_ao(h_sys_t *h_sys_)
{
	uint32_t start = cycle_counter_get();

	configASSERT(NULL != h_sys_);
	configASSERT(NULL != h_sys_->sys_ao);

	if (NULL != h_sys_->sys_ao->h_task)
	{
		vTaskDelete(h_sys_->sys_ao->h_task);
		h_sys_->sys_ao->h_task = NULL;
	}

	if (NULL != h_sys_->sys_ao->h_queue)
	{
		vQueueUnregisterQueue(h_sys_->sys_ao->h_queue);
		vQueueDelete(h_sys_->sys_ao->h_queue);
		h_sys_->sys_ao->h_queue = NULL;
	}

	update_wcet(&g_sys_ao_wcet.release_cycles_max, start);
}

BaseType_t send_sys_ao(h_sys_t *h_sys_, const sys_event_t *event_)
{
	uint32_t start = cycle_counter_get();
	BaseType_t ret = pdFAIL;

	if ((NULL != h_sys_) && (NULL != h_sys_->sys_ao) &&
		(NULL != h_sys_->sys_ao->h_queue) && (NULL != event_))
	{
		ret = xQueueOverwrite(h_sys_->sys_ao->h_queue, event_);
	}

	update_wcet(&g_sys_ao_wcet.send_cycles_max, start);
	return ret;
}

void ioctl_sys_ao(h_sys_t *h_sys_)
{
	uint32_t start = cycle_counter_get();

	configASSERT(NULL != h_sys_);
	configASSERT(NULL != h_sys_->sys_ao);

	/* Reserved for future AO-specific control commands. */
	update_wcet(&g_sys_ao_wcet.ioctl_cycles_max, start);
}

/********************** end of file ******************************************/
