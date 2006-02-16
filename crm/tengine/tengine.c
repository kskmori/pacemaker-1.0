/* $Id: tengine.c,v 1.113 2006/02/16 15:20:32 andrew Exp $ */
/* 
 * Copyright (C) 2004 Andrew Beekhof <andrew@beekhof.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <portability.h>

#include <sys/param.h>
#include <crm/crm.h>
#include <crm/cib.h>
#include <crm/msg_xml.h>
#include <crm/common/msg.h>
#include <crm/common/xml.h>
#include <tengine.h>
#include <heartbeat.h>
#include <clplumbing/Gmain_timeout.h>
#include <lrm/lrm_api.h>
#include <clplumbing/lsb_exitcodes.h>

IPC_Channel *crm_ch = NULL;

void fire_synapse(synapse_t *synapse);
gboolean confirm_synapse(synapse_t *synapse, int action_id);
void check_synapse_triggers(synapse_t *synapse, int action_id);
void cib_action_updated(
	const HA_Message *msg, int call_id, int rc,
	crm_data_t *output, void *user_data);

te_timer_t *transition_timer = NULL;
te_timer_t *abort_timer = NULL;
char *te_uuid = NULL;
extern gboolean shuttingdown;

int
unconfirmed_actions(void)
{
	int unconfirmed = 0;
	crm_debug_2("Unconfirmed actions...");
	slist_iter(
		synapse, synapse_t, transition_graph->synapses, lpc,

		/* lookup event */
		slist_iter(
			action, crm_action_t, synapse->actions, lpc2,
			if(action->executed && action->confirmed == FALSE) {
				unconfirmed++;
				crm_debug("Action %d: unconfirmed",action->id);
			}
			);
		);
	if(unconfirmed > 0) {
		crm_info("Waiting on %d unconfirmed actions", unconfirmed);
	}
	return unconfirmed;
}

	
