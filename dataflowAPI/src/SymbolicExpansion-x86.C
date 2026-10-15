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

// This and SymbolicExpansion-x86_64.C each instantiate one of the ROSE x86
// semantics classes, whose translate() is a switch of about 2000 lines
// that takes the compiler over 10 seconds.  They are kept apart so
// the two compile in parallel.

#include "SymbolicExpansion.h"
#include "SymEvalPolicy.h"

#include "../rose/SgAsmx86Instruction.h"
#include "../rose/x86InstructionSemantics.h"

#include "debug_dataflow.h"

using namespace Dyninst;
using namespace DataflowAPI;

bool SymbolicExpansion::expandX86(SgAsmInstruction *rose_insn,
                                  SymEvalPolicy &policy) {
    SgAsmx86Instruction *insn = static_cast<SgAsmx86Instruction *>(rose_insn);
    X86InstructionSemantics<SymEvalPolicy, Handle> t(policy);
    try {
        t.processInstruction(insn);
    } catch (rose::BinaryAnalysis::InstructionSemantics2::BaseSemantics::Exception &e) {
      expand_printf("Failed to process instruction '%s': %s\n", rose_insn->get_mnemonic().c_str(), e.what());
      return false;
    }
    return true;
}
