/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
 * reserved.
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
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include <assert.h>
#include <stdlib.h>

#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_errno.h"

static void test_bad_input(apr_pool_t *p)
{
    struct {
        const char *ipstr;
        const char *mask;
        apr_status_t expected_rv;
    } testcases[] =
    {
        /* so we have a few good inputs in here; sue me */
        {"my.host.name",       NULL,               APR_EINVAL}
        ,{"127.0.0.256",       NULL,               APR_EBADIP}
        ,{"127.0.0.1",         NULL,               APR_SUCCESS}
        ,{"127.0.0.1",         "32",               APR_SUCCESS}
        ,{"127.0.0.1",         "1",                APR_SUCCESS}
        ,{"127.0.0.1",         "15",               APR_SUCCESS}
        ,{"127.0.0.1",         "-1",               APR_EBADMASK}
        ,{"127.0.0.1",         "0",                APR_EBADMASK}
        ,{"127.0.0.1",         "33",               APR_EBADMASK}
        ,{"127.0.0.1",         "255.0.0.0",        APR_SUCCESS}
        ,{"127.0.0.1",         "255.0",            APR_EBADMASK}
        ,{"127.0.0.1",         "255.255.256.0",    APR_EBADMASK}
        ,{"127.0.0.1",         "abc",              APR_EBADMASK}
        ,{"127",               NULL,               APR_SUCCESS}
        ,{"127.0.0.1.2",       NULL,               APR_EBADIP}
        ,{"127.0.0.1.2",       "8",                APR_EBADIP}
        ,{"127",               "255.0.0.0",        APR_EBADIP} /* either EBADIP or EBADMASK seems fine */
#if APR_HAVE_IPV6
        ,{"::1",               NULL,               APR_SUCCESS}
        ,{"::1",               "20",               APR_SUCCESS}
        ,{"::ffff:9.67.113.15", NULL,              APR_EBADIP} /* yes, this is goodness */
        ,{"fe80::",            "16",               APR_SUCCESS}
        ,{"fe80::",            "255.0.0.0",        APR_EBADMASK}
        ,{"fe80::1",           "0",                APR_EBADMASK}
        ,{"fe80::1",           "-1",               APR_EBADMASK}
        ,{"fe80::1",           "1",                APR_SUCCESS}
        ,{"fe80::1",           "33",               APR_SUCCESS}
        ,{"fe80::1",           "128",              APR_SUCCESS}
        ,{"fe80::1",           "129",              APR_EBADMASK}
#else
        /* do some IPv6 stuff and verify that it fails with APR_EBADIP */
        ,{"::ffff:9.67.113.15", NULL,              APR_EBADIP}
#endif
    };
    int i;
    apr_ipsubnet_t *ipsub;
    apr_status_t rv;

    for (i = 0; i < (sizeof testcases / sizeof testcases[0]); i++) {
        rv = apr_ipsubnet_create(&ipsub, testcases[i].ipstr, testcases[i].mask, p);
        assert(rv == testcases[i].expected_rv);
    }
}

static void test_singleton_subnets(apr_pool_t *p)
{
    const char *v4addrs[] = {
        "127.0.0.1", "129.42.18.99", "63.161.155.20", "207.46.230.229", "64.208.42.36",
        "198.144.203.195", "192.18.97.241", "198.137.240.91", "62.156.179.119", 
        "204.177.92.181"
    };
    apr_ipsubnet_t *ipsub;
    apr_sockaddr_t *sa;
    apr_status_t rv;
    int i, j, rc;

    for (i = 0; i < sizeof v4addrs / sizeof v4addrs[0]; i++) {
        rv = apr_ipsubnet_create(&ipsub, v4addrs[i], NULL, p);
        assert(rv == APR_SUCCESS);
        for (j = 0; j < sizeof v4addrs / sizeof v4addrs[0]; j++) {
            rv = apr_sockaddr_info_get(&sa, v4addrs[j], APR_INET, 0, 0, p);
            assert(rv == APR_SUCCESS);
            rc = apr_ipsubnet_test(ipsub, sa);
            if (!strcmp(v4addrs[i], v4addrs[j])) {
                assert(rc != 0);
            }
            else {
                assert(rc == 0);
            }
        }
    }

    /* same for v6? */
}

static void test_interesting_subnets(apr_pool_t *p)
{
    struct {
        const char *ipstr, *mask;
        int family;
        char *in_subnet, *not_in_subnet;
    } testcases[] =
    {
        {"9.67",              NULL,            APR_INET,  "9.67.113.15",         "10.1.2.3"}
        ,{"9.67.0.0",         "16",            APR_INET,  "9.67.113.15",         "10.1.2.3"}
        ,{"9.67.0.0",         "255.255.0.0",   APR_INET,  "9.67.113.15",         "10.1.2.3"}
        ,{"9.67.113.99",      "16",            APR_INET,  "9.67.113.15",         "10.1.2.3"}
        ,{"9.67.113.99",      "255.255.255.0", APR_INET,  "9.67.113.15",         "10.1.2.3"}
#if APR_HAVE_IPV6
        ,{"fe80::",           "8",             APR_INET6, "fe80::1",             "ff01::1"}
        ,{"ff01::",           "8",             APR_INET6, "ff01::1",             "fe80::1"}
        ,{"3FFE:8160::",      "28",            APR_INET6, "3ffE:816e:abcd:1234::1", "3ffe:8170::1"}
        ,{"127.0.0.1",        NULL,            APR_INET6, "::ffff:127.0.0.1",    "fe80::1"}
        ,{"127.0.0.1",        "8",             APR_INET6, "::ffff:127.0.0.1",    "fe80::1"}
#endif
    };
    apr_ipsubnet_t *ipsub;
    apr_sockaddr_t *sa;
    apr_status_t rv;
    int i, rc;

    for (i = 0; i < sizeof testcases / sizeof testcases[0]; i++) {
        rv = apr_ipsubnet_create(&ipsub, testcases[i].ipstr, testcases[i].mask, p);
        assert(rv == APR_SUCCESS);
        rv = apr_sockaddr_info_get(&sa, testcases[i].in_subnet, testcases[i].family, 0, 0, p);
        assert(rv == APR_SUCCESS);
        rc = apr_ipsubnet_test(ipsub, sa);
        assert(rc != 0);
        rv = apr_sockaddr_info_get(&sa, testcases[i].not_in_subnet, testcases[i].family, 0, 0, p);
        assert(rv == APR_SUCCESS);
        rc = apr_ipsubnet_test(ipsub, sa);
        assert(rc == 0);
    }
}

int main(void)
{
    apr_status_t rv;
    apr_pool_t *p;
    char buf[128];

    rv = apr_initialize();
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_initialize()->%d/%s\n",
                rv,
                apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    atexit(apr_terminate);

    rv = apr_pool_create(&p, NULL);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "apr_pool_create()->%d/%s\n",
                rv,
                apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    test_bad_input(p);
    test_singleton_subnets(p);
    test_interesting_subnets(p);

    printf("error strings:\n");
    printf("\tAPR_EBADIP\t`%s'\n", apr_strerror(APR_EBADIP, buf, sizeof buf));
    printf("\tAPR_EBADMASK\t`%s'\n", apr_strerror(APR_EBADMASK, buf, sizeof buf));

    return 0;
}
