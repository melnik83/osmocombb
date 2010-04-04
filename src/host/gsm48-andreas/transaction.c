/* GSM 04.07 Transaction handling */

/* (C) 2009 by Harald Welte <laforge@gnumonks.org>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <osmocom/transaction.h>
#include <osmocom/mnccms.h>
#include <osmocom/debug.h>
#include <osmocore/talloc.h>
#include <osmocom/gsm_04_08.h>

void *tall_trans_ctx;

void _gsm48_cc_trans_free(struct gsm_trans *trans);

struct gsm_trans *trans_find_by_id(struct osmocom_ms *ms,
				   u_int8_t proto, u_int8_t trans_id)
{
	struct gsm_trans *trans;

	llist_for_each_entry(trans, &ms->trans_list, entry) {
		if (trans->subscr == subscr &&
		    trans->protocol == proto &&
		    trans->transaction_id == trans_id)
			return trans;
	}
	return NULL;
}

struct gsm_trans *trans_find_by_callref(struct osmocom_ms *ms,
					u_int32_t callref)
{
	struct gsm_trans *trans;

	llist_for_each_entry(trans, &ms->trans_list, entry) {
		if (trans->callref == callref)
			return trans;
	}
	return NULL;
}

struct gsm_trans *trans_alloc(struct osmocom_ms *ms,
			      u_int8_t protocol, u_int8_t trans_id,
			      u_int32_t callref)
{
	struct gsm_trans *trans;

	DEBUGP(DCC, "ms %s\n", ms->name);

	trans = talloc_zero(tall_trans_ctx, struct gsm_trans);
	if (!trans)
		return NULL;

	trans->ms = ms;

	trans->protocol = protocol;
	trans->transaction_id = trans_id;
	trans->callref = callref;

	llist_add_tail(&trans->entry, &ms->trans_list);

	return trans;
}

void trans_free(struct gsm_trans *trans)
{
	switch (trans->protocol) {
	case GSM48_PDISC_CC:
		_gsm48_cc_trans_free(trans);
		break;
#if 0
	case GSM48_PDISC_SS:
		_gsm411_ss_trans_free(trans);
		break;
	}
	case GSM48_PDISC_SMS:
		_gsm411_sms_trans_free(trans);
		break;
	}
#endif

	llist_del(&trans->entry);

	talloc_free(trans);
}

/* allocate an unused transaction ID 
 * in the given protocol using the ti_flag specified */
int trans_assign_trans_id(struct osmocom_ms *ms,
			  u_int8_t protocol, u_int8_t ti_flag)
{
	struct gsm_trans *trans;
	unsigned int used_tid_bitmask = 0;
	int i, j, h;

	if (ti_flag)
		ti_flag = 0x8;

	/* generate bitmask of already-used TIDs for this (subscr,proto) */
	llist_for_each_entry(trans, &ms->trans_list, entry) {
		if (trans->subscr != subscr ||
		    trans->protocol != protocol ||
		    trans->transaction_id == 0xff)
			continue;
		used_tid_bitmask |= (1 << trans->transaction_id);
	}

	/* find a new one, trying to go in a 'circular' pattern */
	for (h = 6; h > 0; h--)
		if (used_tid_bitmask & (1 << (h | ti_flag)))
			break;
	for (i = 0; i < 7; i++) {
		j = ((h + i) % 7) | ti_flag;
		if ((used_tid_bitmask & (1 << j)) == 0)
			return j;
	}

	return -1;
}
