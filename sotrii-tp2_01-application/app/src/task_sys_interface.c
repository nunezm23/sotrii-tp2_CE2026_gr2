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

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Interface functions */
void open_sys_ao(h_sys_t *h_sys_)
{
    BaseType_t ret;

    configASSERT(h_sys_ != NULL);
    configASSERT(h_sys_->sys_sc != NULL);

    /* Active Object ID */
    h_sys_->ao_id = 0u;

    /* Initialize Statechart */
    h_sys_->sys_sc->state    = ST_SYS_IDLE;
    h_sys_->sys_sc->ev_in    = EV_SYS_NONE;
    h_sys_->sys_sc->tick     = 0u;
    h_sys_->sys_sc->ev_out   = EV_SYS_NONE;
    h_sys_->sys_sc->tick_out = 0u;

    /* Create Active Object Queue */
    h_sys_->ao_queue = xQueueCreate(
            SYS_QUEUE_LEN,
            sizeof(sys_ev_t));

    configASSERT(h_sys_->ao_queue != NULL);

    /* Create Gatekeeper Task */
    ret = xTaskCreate(
            task_sys,
            "Task Sys",
            (2 * configMINIMAL_STACK_SIZE),
            (void *)h_sys_,
            (tskIDLE_PRIORITY + 1),
            &h_sys_->ao_task);

    configASSERT(ret == pdPASS);

    LOGGER_INFO("\tAO Sys initialized");
}

void release_sys_ao(h_sys_t *h_sys_)
{
    configASSERT(h_sys_ != NULL);

    if (h_sys_->ao_queue != NULL)
    {
        vQueueDelete(h_sys_->ao_queue);
        h_sys_->ao_queue = NULL;
    }
}

BaseType_t send_sys_ao(
        h_sys_t *h_sys_,
        sys_ev_t event_,
        TickType_t timeout_)
{
    configASSERT(h_sys_ != NULL);

    return xQueueSend(
            h_sys_->ao_queue,
            &event_,
            timeout_);
}

void ioctl_sys_ao(h_sys_t *h_sys_)
{
    UNUSED(h_sys_);

    /* TODO:
     * Configure Active Object parameters.
     * No configurable parameters yet.
     */
}

/********************** end of file ******************************************/
