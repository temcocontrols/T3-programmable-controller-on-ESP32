/*
 * Timer fucntions, with callback, mainly for Windows and *nix.
 *
 * This file is part of uSNMP ("micro-SNMP").
 * uSNMP is released under a BSD-style license. The full text follows.
 *
 * Copyright (c) 2022 Francis Tay. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is hereby granted without fee provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Francis Tay nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY FRANCIS TAY AND CONTRIBUTERS 'AS
 * IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOTLIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL FRANCIS TAY OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARAY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/


#include "timer.h"
#include "esp_timer.h"
#include "esp_err.h"

static void (*timer_func_handler)(void) = NULL;
static esp_timer_handle_t periodic_timer = NULL;

static void esp_timer_cb(void* arg)
{
	if (timer_func_handler) timer_func_handler();
}

int timer_start(int mSec, void (*timer_handler)(void))
{
	esp_err_t err;
	timer_func_handler = timer_handler;

	if (periodic_timer) {
		esp_timer_stop(periodic_timer);
		esp_timer_delete(periodic_timer);
		periodic_timer = NULL;
	}

	const esp_timer_create_args_t create_args = {
		.callback = &esp_timer_cb,
		.arg = NULL,
		.name = "usnmp_timer"
	};

	err = esp_timer_create(&create_args, &periodic_timer);
	if (err != ESP_OK) return -1;
	err = esp_timer_start_periodic(periodic_timer, (uint64_t)mSec * 1000ULL);
	if (err != ESP_OK) {
		esp_timer_delete(periodic_timer);
		periodic_timer = NULL;
		return -1;
	}
	return 0;
}

void timer_stop(void)
{
	if (periodic_timer) {
		esp_timer_stop(periodic_timer);
		esp_timer_delete(periodic_timer);
		periodic_timer = NULL;
	}
}
