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
#include "task_btn.h"
#include "task_btn_attribute.h"
#include "task_btn_interface.h"
#include "task_sys.h"
#include "task_sys_interface.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Interface functions */
void open_btn_ao(h_btn_t *h_btn_)
{
	BaseType_t ret;

	configASSERT(NULL != h_btn_);
	configASSERT(NULL != h_btn_->btn_ao);
	configASSERT(NULL == h_btn_->btn_ao->h_task);

	/* La referencia al objeto permite reutilizar una única función de tarea
	 * para cualquier botón físico. */
	ret = xTaskCreate(task_btn, h_btn_->btn_ao->task_txt,
				  configMINIMAL_STACK_SIZE, h_btn_, tskIDLE_PRIORITY + 1ul,
				  &h_btn_->btn_ao->h_task);
	configASSERT(pdPASS == ret);
}

void release_btn_ao(h_btn_t *h_btn_)
{
	configASSERT(NULL != h_btn_);
	configASSERT(NULL != h_btn_->btn_ao);

	if (NULL != h_btn_->btn_ao->h_task)
	{
		vTaskDelete(h_btn_->btn_ao->h_task);
		h_btn_->btn_ao->h_task = NULL;
	}
}

BaseType_t send_btn_ao(h_btn_t *h_btn_, btn_ev_t event_, TickType_t time_)
{
    sys_msg_t message;

    configASSERT(NULL != h_btn_);

    message.btn_id = h_btn_->btn->id;
    message.event = event_;
    message.time = time_;

    LOGGER_INFO("BTN_%u sent event %u, time %lu mS", message.btn_id,
                message.event, message.time);

    return send_sys_ao(
            &h_sys,
            &message,
            TASK_BTN_DEL_ZERO);
}

void ioctl_btn_ao(h_btn_t *h_btn_)
{
	/* Punto de extensión para cambiar en el futuro debounce o modo de lectura. */
	configASSERT(NULL != h_btn_);
}

/********************** end of file ******************************************/
