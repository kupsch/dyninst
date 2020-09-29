/*
 * See the dyninst/COPYRIGHT file for copyright information.
 *
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 *
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#include <string.h>
#include <stdlib.h>
#include "symbolDemangle.h"

static thread_local char* lastSymName = nullptr;
static thread_local bool lastIncludeParams = false;
static thread_local char* lastDemangled = nullptr;



// Returns a demangled symbol using symbol_demangle, but includes a per thread
// cache of the previous demangling.
//
char *symbol_demangle_with_cache(const char *symName, bool includeParams)
{
    if (lastSymName && lastDemangled && includeParams == lastIncludeParams && !strcmp(symName, lastSymName))  {
	// found hit, return a copy of cached demangling
	return strdup(lastDemangled);
    }

    char *demangled = symbol_demangle(symName, includeParams);

    // update cache
    if (lastSymName)  {
	free(lastSymName);
    }
    if (lastDemangled)  {
	free(lastDemangled);
    }
    lastSymName = strdup(symName);
    lastIncludeParams = includeParams;
    lastDemangled = strdup(demangled);

    return demangled;
}
