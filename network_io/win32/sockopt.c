/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "networkio.h"
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <string.h>
#include <winsock2.h>

ap_status_t soblock(SOCKET sd)
{
    int one = 1;

    if (ioctlsocket(sd, FIONBIO, &one) == SOCKET_ERROR) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t sononblock(SOCKET sd)
{
    int zero = 0;

    if (ioctlsocket(sd, FIONBIO, &zero) == SOCKET_ERROR) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_setsocketopt(struct socket_t *sock, ap_int32_t opt, ap_int32_t on)
{
    int one;
    struct linger li;
    ap_status_t stat;

    if (on)
        one = 1;
    else
        one = 0;

    if (opt & APR_SO_KEEPALIVE) {
        if (setsockopt(sock->sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int)) == -1) {
            return APR_EEXIST;
        }
    }
    if (opt & APR_SO_DEBUG) {
        if (setsockopt(sock->sock, SOL_SOCKET, SO_DEBUG, (void *)&one, sizeof(int)) == -1) {
            return APR_EEXIST;
        }
    }
    if (opt & APR_SO_REUSEADDR) {
        if (setsockopt(sock->sock, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int)) == -1) {
            return APR_EEXIST;
        }
    }
    if (opt & APR_SO_NONBLOCK) {
        if (on) {
            if ((stat = soblock(sock->sock)) != APR_SUCCESS) 
                return stat;
        }
        else {
            if ((stat = sononblock(sock->sock)) != APR_SUCCESS)
                return stat;
        }
    }
    if (opt & APR_SO_LINGER) {
        li.l_onoff = on;
        li.l_linger = MAX_SECS_TO_LINGER;
        if (setsockopt(sock->sock, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger)) == -1) {
            return APR_EEXIST;
        }
    }
    return APR_SUCCESS;
}         

ap_status_t ap_gethostname(char *buf, int len, ap_context_t *cont)
{
    if (gethostname(buf, len) == -1)
        return APR_EEXIST;
    else
        return APR_SUCCESS;
}

ap_status_t ap_get_remote_hostname(char **name, struct socket_t *sock)
{
    struct hostent *hptr;

    hptr = gethostbyaddr((char *)&(sock->addr->sin_addr),
                         sizeof(struct in_addr), AF_INET);

    if (hptr != NULL) {
        *name = ap_pstrdup(sock->cntxt, hptr->h_name);
        if (*name) {
            return APR_SUCCESS;
        }
        return APR_ENOMEM;
    }

    /* XXX - I don't know what the correct way to deal with host resolution
     * errors on Windows is. WSAGetLastError() seems to use a different set of
     * error numbers than APR does. The previous code didn't deal with them at
     * all.  - manoj
     */

    return h_errno;
}


