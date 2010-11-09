/*
 * Copyright (c) 1996-2009 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
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

#include "x86_process.h"

x86_process::x86_process(Dyninst::PID p, std::string e, std::vector<std::string> a, std::map<int, int> f) :
  int_process(p, e, a, f)
{
}

x86_process::x86_process(Dyninst::PID pid_, int_process *p) :
  int_process(pid_, p)
{
}

x86_process::~x86_process()
{
}

unsigned x86_process::plat_breakpointSize()
{
  assert(getTargetArch() == Arch_x86_64 || getTargetArch() == Arch_x86);
  return 1;
}

void x86_process::plat_breakpointBytes(char *buffer)
{
  assert(getTargetArch() == Arch_x86_64 || getTargetArch() == Arch_x86);
  buffer[0] = 0xcc;
}
