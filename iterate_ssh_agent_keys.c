/* 
 * Copyright, 2008  Jamie Beverly
 * pam_ssh_agent_auth PAM module
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */


#include "includes.h"
#include "config.h"

#include "openbsd-compat/sys-queue.h"
#include "xmalloc.h"
#include "log.h"
#include "buffer.h"
#include "key.h"
#include "authfd.h"
#include <stdio.h>
#include <openssl/evp.h>

#include "userauth_pubkey_from_id.h"
#include "identity.h"

u_char * session_id2 = NULL;
uint8_t session_id_len = 0;

u_char *
session_id2_gen()
{
    char *cookie = NULL;
    uint8_t i = 0;
    uint32_t rnd = 0;

    rnd = arc4random();
    session_id_len = (uint8_t) rnd;

    cookie = calloc(1,session_id_len);

    for (i = 0; i < session_id_len; i++) {
        if (i % 4 == 0) {
            rnd = arc4random();
        }
        cookie[i] = (char) rnd;
        rnd >>= 8;
    }

    return cookie;
}

int
find_authorized_keys(uid_t uid)
{
    Identity *id;
    Key *key;
    AuthenticationConnection *ac;
    char *comment;
    uint8_t retval = 0;

    OpenSSL_add_all_digests();
    session_id2 = session_id2_gen();

    if ((ac = ssh_get_authentication_connection(uid))) {
        verbose("Contacted ssh-agent of user %s (%u)", getpwuid(uid)->pw_name, uid);
        for (key = ssh_get_first_identity(ac, &comment, 2); key != NULL; key = ssh_get_next_identity(ac, &comment, 2)) 
        {
            if(key != NULL) {
                id = xcalloc(1, sizeof(*id));
                id->key = key;
                id->filename = comment;
                id->ac = ac;
                if(userauth_pubkey_from_id(id)) {
                    retval = 1;
                }
                xfree(id->filename);
                key_free(id->key);
                xfree(id);
                if(retval == 1)
                    break;
            }
        }
        ssh_close_authentication_connection(ac);
    }
    else {
        verbose("No ssh-agent could be contacted");
    }
    xfree(session_id2);
    EVP_cleanup();
    return retval;
}
