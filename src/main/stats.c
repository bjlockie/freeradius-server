/*
 * stats.c	Internal statistics handling.
 *
 * Version:	$Id$
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Copyright 2008  The FreeRADIUS server project
 * Copyright 2008  Alan DeKok <aland@deployingradius.com>
 */

#include <freeradius-devel/ident.h>
RCSID("$Id$")

#include <freeradius-devel/radiusd.h>
#include <freeradius-devel/rad_assert.h>

#ifdef WITH_STATS

fr_stats_t radius_auth_stats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#ifdef WITH_ACCOUNTING
fr_stats_t radius_acct_stats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif

#ifdef WITH_PROXY
fr_stats_t proxy_auth_stats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#ifdef WITH_ACCOUNTING
fr_stats_t proxy_acct_stats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif
#endif

void request_stats_final(REQUEST *request)
{
	if (request->master_state == REQUEST_COUNTED) return;

	if ((request->listener->type != RAD_LISTEN_AUTH) &&
	    (request->listener->type != RAD_LISTEN_ACCT)) return;

	/*
	 *	Update the statistics.
	 *
	 *	Note that we do NOT do this in a child thread.
	 *	Instead, we update the stats when a request is
	 *	deleted, because only the main server thread calls
	 *	this function, which makes it thread-safe.
	 */
	switch (request->reply->code) {
	case PW_AUTHENTICATION_ACK:
		radius_auth_stats.total_responses++;
		radius_auth_stats.total_access_accepts++;
		if (request->client && request->client->auth) {
			request->client->auth->accepts++;
		}
		break;

	case PW_AUTHENTICATION_REJECT:
		radius_auth_stats.total_responses++;
		radius_auth_stats.total_access_rejects++;
		if (request->client && request->client->auth) {
			request->client->auth->rejects++;
		}
		break;

	case PW_ACCESS_CHALLENGE:
		radius_auth_stats.total_responses++;
		radius_auth_stats.total_access_challenges++;
		if (request->client && request->client->auth) {
			request->client->auth->challenges++;
		}
		break;

#ifdef WITH_ACCOUNTING
	case PW_ACCOUNTING_RESPONSE:
		radius_acct_stats.total_responses++;
		if (request->client && request->client->acct) {
			request->client->acct->responses++;
		}
		break;
#endif

		/*
		 *	No response, it must have been a bad
		 *	authenticator.
		 */
	case 0:
		if (request->packet->code == PW_AUTHENTICATION_REQUEST) {
			radius_auth_stats.total_bad_authenticators++;
			if (request->client && request->client->auth) {
				request->client->auth->bad_authenticators++;
			}
		}
		break;

	default:
		break;
	}

#ifdef WITH_PROXY
	if (!request->proxy) goto done;	/* simplifies formatting */
		
	switch (request->proxy_reply->code) {
	case PW_AUTHENTICATION_REQUEST:
		proxy_auth_stats.total_requests += request->num_proxied_requests;
		break;

#ifdef WITH_ACCOUNTING
	case PW_ACCOUNTING_REQUEST:
		proxy_acct_stats.total_requests++;
		break;
#endif

	default:
		break;
	}

	if (!request->proxy_reply) goto done;	/* simplifies formatting */

	switch (request->proxy_reply->code) {
	case PW_AUTHENTICATION_ACK:
		proxy_auth_stats.total_responses += request->num_proxied_responses;
		proxy_auth_stats.total_access_accepts += request->num_proxied_responses;
		break;

	case PW_AUTHENTICATION_REJECT:
		proxy_auth_stats.total_responses += request->num_proxied_responses;
		proxy_auth_stats.total_access_rejects += request->num_proxied_responses;
		break;

	case PW_ACCESS_CHALLENGE:
		proxy_auth_stats.total_responses += request->num_proxied_responses;
		proxy_auth_stats.total_access_challenges += request->num_proxied_responses;
		break;

#ifdef WITH_ACCOUNTING
	case PW_ACCOUNTING_RESPONSE:
		radius_acct_stats.total_responses++;
		break;
#endif

	default:
		proxy_auth_stats.total_unknown_types++;
		break;
	}

 done:
#endif /* WITH_PROXY */

	request->master_state = REQUEST_COUNTED;
}

typedef struct fr_stats2vp {
	int	attribute;
	size_t	offset;
} fr_stats2vp;

/*
 *	Authentication
 */
static fr_stats2vp authvp[] = {
	{ 128, offsetof(fr_stats_t, total_requests) },
	{ 129, offsetof(fr_stats_t, total_access_accepts) },
	{ 130, offsetof(fr_stats_t, total_access_rejects) },
	{ 131, offsetof(fr_stats_t, total_access_challenges) },
	{ 132, offsetof(fr_stats_t, total_responses) },
	{ 133, offsetof(fr_stats_t, total_dup_requests) },
	{ 134, offsetof(fr_stats_t, total_malformed_requests) },
	{ 135, offsetof(fr_stats_t, total_bad_authenticators) },
	{ 136, offsetof(fr_stats_t, total_packets_dropped) },
	{ 137, offsetof(fr_stats_t, total_unknown_types) },
	{ 0, 0 }
};


#ifdef WITH_PROXY
/*
 *	Proxied authentication requests.
 */
static fr_stats2vp proxy_authvp[] = {
	{ 138, offsetof(fr_stats_t, total_requests) },
	{ 139, offsetof(fr_stats_t, total_access_accepts) },
	{ 140, offsetof(fr_stats_t, total_access_rejects) },
	{ 141, offsetof(fr_stats_t, total_access_challenges) },
	{ 142, offsetof(fr_stats_t, total_responses) },
	{ 143, offsetof(fr_stats_t, total_dup_requests) },
	{ 144, offsetof(fr_stats_t, total_malformed_requests) },
	{ 145, offsetof(fr_stats_t, total_bad_authenticators) },
	{ 146, offsetof(fr_stats_t, total_packets_dropped) },
	{ 147, offsetof(fr_stats_t, total_unknown_types) },
	{ 0, 0 }
};
#endif


#ifdef WITH_ACCOUNTING
/*
 *	Accounting
 */
static fr_stats2vp acctvp[] = {
	{ 148, offsetof(fr_stats_t, total_requests) },
	{ 149, offsetof(fr_stats_t, total_responses) },
	{ 150, offsetof(fr_stats_t, total_dup_requests) },
	{ 151, offsetof(fr_stats_t, total_malformed_requests) },
	{ 152, offsetof(fr_stats_t, total_bad_authenticators) },
	{ 153, offsetof(fr_stats_t, total_packets_dropped) },
	{ 154, offsetof(fr_stats_t, total_unknown_types) },
	{ 0, 0 }
};

#ifdef WITH_PROXY
static fr_stats2vp proxy_acctvp[] = {
	{ 155, offsetof(fr_stats_t, total_requests) },
	{ 156, offsetof(fr_stats_t, total_responses) },
	{ 157, offsetof(fr_stats_t, total_dup_requests) },
	{ 158, offsetof(fr_stats_t, total_malformed_requests) },
	{ 159, offsetof(fr_stats_t, total_bad_authenticators) },
	{ 160, offsetof(fr_stats_t, total_packets_dropped) },
	{ 161, offsetof(fr_stats_t, total_unknown_types) },
	{ 0, 0 }
};
#endif
#endif


#define FR2ATTR(x) ((11344 << 16) | (x))

static void request_stats_addvp(REQUEST *request,
				fr_stats2vp *table, fr_stats_t *stats)
{
	int i;
	VALUE_PAIR *vp;

	for (i = 0; table[i].attribute != 0; i++) {
		vp = radius_paircreate(request, &request->reply->vps,
				       FR2ATTR(table[i].attribute),
				       PW_TYPE_INTEGER);
		if (!vp) continue;

		vp->vp_integer = *(int *)(((char *) stats) + table[i].offset);
	}
}


void request_stats_reply(REQUEST *request)
{
	VALUE_PAIR *vp;

	if (request->packet->code != PW_STATUS_SERVER) return;

	if ((request->packet->src_ipaddr.af != AF_INET) ||
	    (request->packet->src_ipaddr.ipaddr.ip4addr.s_addr != htonl(INADDR_LOOPBACK))) return;

	vp = pairfind(request->packet->vps, FR2ATTR(127));
	if (!vp || (vp->vp_integer == 0)) return;


	if (vp->vp_integer & 0x01) {
		request_stats_addvp(request, authvp, &radius_auth_stats);
	}
		
#ifdef WITH_ACCOUNTING
	if (vp->vp_integer & 0x02) {
		request_stats_addvp(request, acctvp, &radius_acct_stats);
	}
#endif

#ifdef WITH_PROXY
	if (vp->vp_integer & 0x04) {
		request_stats_addvp(request, proxy_authvp, &proxy_auth_stats);
	}

#ifdef WITH_ACCOUNTING
	if (vp->vp_integer & 0x08) {
		request_stats_addvp(request, proxy_acctvp, &proxy_acct_stats);
	}
#endif
#endif
}

#endif /* WITH_STATS */