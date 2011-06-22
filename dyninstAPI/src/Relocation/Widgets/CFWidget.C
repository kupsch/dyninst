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

#include "CFWidget.h"
#include "Widget.h"
#include "../CFG/RelocTarget.h"

#include "instructionAPI/h/Instruction.h"

#include "../patchapi_debug.h"

#include "../CodeTracker.h"
#include "../CodeBuffer.h"


#if defined(MEMORY_EMULATION_LAYER)
#include "dyninstAPI/src/BPatch_memoryAccessAdapter.h"
#include "dyninstAPI/h/BPatch_memoryAccess_NP.h"
#include "dyninstAPI/src/MemoryEmulator/memEmulatorWidget.h"
#endif

using namespace Dyninst;
using namespace Relocation;
using namespace InstructionAPI;

///////////////////////

// Pick values that don't correspond to actual targets. I'm skipping
// 0 because it's used all over the place as a null.
const Address CFWidget::Fallthrough(1);
const Address CFWidget::Taken(2);

// Case 1: an empty trace ender for traces that do not
// end in a CF-category instruction
CFWidget::Ptr CFWidget::create(Address a) {
   CFWidget::Ptr ptr = Ptr(new CFWidget(a));
   return ptr;
}

// Case 2: wrap a CF-category instruction
CFWidget::Ptr CFWidget::create(Widget::Ptr atom) {
   CFWidget::Ptr ptr = Ptr(new CFWidget(atom->insn(), atom->addr()));
   return ptr;
}

CFWidget::CFWidget(InstructionAPI::Instruction::Ptr insn, Address addr)  :
   isCall_(false), 
   isConditional_(false), 
   isIndirect_(false),
   gap_(0),
   insn_(insn),
   addr_(addr) {
   
   if (insn->getCategory() == c_CallInsn) {
      // Calls have a fallthrough but are not conditional.
      // TODO: conditional calls work how?

      isCall_ = true;
   } else if (insn->allowsFallThrough()) {
      isConditional_ = true;
   }

   // TODO: IAPI is recording all PPC64 instructions as PPC32. However, the
   // registers they use are still PPC64. This is a pain to fix, and therefore
   // I'm working around it here and in Movement-adhoc.C by checking _both_
   // 32- and 64-bit. 

   Architecture fixme = insn_->getArch();
   if (fixme == Arch_ppc32) fixme = Arch_ppc64;

   Expression::Ptr thePC(new RegisterAST(MachRegister::getPC(insn_->getArch())));
   Expression::Ptr thePCFixme(new RegisterAST(MachRegister::getPC(fixme)));

   Expression::Ptr exp = insn_->getControlFlowTarget();

   exp->bind(thePC.get(), Result(u64, addr_));
   exp->bind(thePCFixme.get(), Result(u64, addr_));
   Result res = exp->eval();
   if (!res.defined) {
      isIndirect_ = true;
   }
}


bool CFWidget::generate(const codeGen &templ,
                      const RelocBlock *trace,
                      CodeBuffer &buffer)
{
   // We need to create jumps to wherever our successors are
   // We can assume the addresses returned by our Targets
   // are valid, since we'll fixpoint until those stabilize. 
   //
   // There are the following cases:
   //
   // No explicit control flow/unconditional direct branch:
   //   1) One target
   //   2) Generate a branch unless it's unnecessary
   // Conditional branch:
   //   1) Two targets
   //   2) Use stored instruction to generate correct condition
   //   3) Generate a fallthrough "branch" if necessary
   // Call:
   //   1) Two targets (call and natural successor)
   //   2) As above, except make sure call bit is flipped on
   // Indirect branch:
   //   1) Just go for it... we have no control, really
   relocation_cerr << "CFWidget generation for " << trace->id() << endl;
   if (destMap_.empty() && !isIndirect_) {
      // No successors at all? Well, it happens if
      // we hit a halt...
      relocation_cerr << "CFWidget /w/ no successors, ret true" << endl;
      return true;
   }

   typedef enum {
      Illegal,
      Single,
      Taken_FT,
      Indirect } Options;
  
   Options opt = Illegal;

   if (isIndirect_) {
      opt = Indirect;
      relocation_cerr << "  generating CFWidget as indirect branch" << endl;
   }
   else if (isConditional_ || isCall_) {
      opt = Taken_FT;
      relocation_cerr << "  generating CFWidget as call or conditional branch" << endl;
   }
   else {
      opt = Single;
      relocation_cerr << "  generating CFWidget as direct branch" << endl;
   }

   switch (opt) {
      case Single: {

         assert(!isIndirect_);
         assert(!isConditional_);
         assert(!isCall_);

         // Check for a taken destination first.
         bool fallthrough = false;
         DestinationMap::iterator iter = destMap_.find(Taken);
         if (iter == destMap_.end()) {
            iter = destMap_.find(Fallthrough);
            fallthrough = true;
         }
         if (iter == destMap_.end()) {
            cerr << "Error in CFWidget from trace " << trace->id()
                 << ", could not find target for single control transfer" << endl;
            cerr << "\t DestMap dump:" << endl;
            for (DestinationMap::iterator d = destMap_.begin(); 
                 d != destMap_.end(); ++d) {
               cerr << "\t\t " << d->first << " : " << d->second->format() << endl;
            }
         }
            
         assert(iter != destMap_.end());

         TargetInt *target = iter->second;
         assert(target);

         if (target->necessary()) {
            if (!generateBranch(buffer,
                                target,
                                insn_,
                                trace,
                                fallthrough)) {
               return false;
            }
         }
         else {
            relocation_cerr << "    target reported unnecessary" << endl;
         }
         break;
      }
      case Taken_FT: {
         // This can be either a call (with an implicit fallthrough as shown by
         // the FUNLINK) or a conditional branch.
         if (isCall_) {
            // Well, that kinda explains things
            assert(!isConditional_);
            relocation_cerr << "  ... generating call" << endl;
            if (!generateCall(buffer,
                              destMap_[Taken],
                              trace,
                              insn_))
               return false;
         }
         else {
            assert(!isCall_);
            relocation_cerr << "  ... generating conditional branch" << endl;
            if (!generateConditionalBranch(buffer,
                                           destMap_[Taken],
                                           trace,
                                           insn_))
               return false;
         }

         // Not necessary by design - fallthroughs are always to the next generated
         // We can have calls that don't return and thus don't have funlink edges
    
         if (destMap_.find(Fallthrough) != destMap_.end()) {
            TargetInt *ft = destMap_[Fallthrough];
            if (ft->necessary()) {
               if (!generateBranch(buffer, 
                                   ft,
                                   insn_,
                                   trace,
                                   true)) {
                  return false;
               }
            }
         }
         break;
      }
      case Indirect: {
         Register reg = Null_Register; /* = originalRegister... */
         // Originally for use in helping with jump tables, I'm taking
         // this for the memory emulation effort. Huzzah!
         if (!generateAddressTranslator(buffer, templ, reg))
            return false;
         if (isCall_) {
            if (!generateIndirectCall(buffer, 
                                      reg, 
                                      insn_, 
                                      trace,
                                      addr_)) 
               return false;
            // We may be putting another block in between this
            // one and its fallthrough due to edge instrumentation
            // So if there's the possibility for a return put in
            // a fallthrough branch
            if (destMap_.find(Fallthrough) != destMap_.end()) {
               if (!generateBranch(buffer,
                                   destMap_[Fallthrough],
                                   Instruction::Ptr(),
                                   trace,
                                   true)) 
                  return false;
            }
         }
         else {
            if (!generateIndirect(buffer, reg, trace, insn_))
               return false;
         }
         break;
      }
      default:
         assert(0);
   }
   if (gap_) {
      // We don't know what the callee does to the return addr,
      // so we'll catch it at runtime. 
      buffer.addPatch(new PaddingPatch(gap_, true, false, trace->block()), 
                      padTracker(addr_, gap_,
                                 trace));
   }
  
   return true;
}

CFWidget::~CFWidget() {
   // Don't delete the Targets; they're taken care of when we nuke the overall CFG. 
}

TrackerElement *CFWidget::tracker(const RelocBlock *trace) const {
   assert(addr_ != 1);
   EmulatorTracker *e = new EmulatorTracker(addr_, trace->block(), trace->func());
   return e;
}

TrackerElement *CFWidget::destTracker(TargetInt *dest) const {
   block_instance *destBlock = NULL;
   func_instance *destFunc = NULL;
   switch (dest->type()) {
      case TargetInt::RelocBlockTarget: {
         Target<RelocBlock *> *targ = static_cast<Target<RelocBlock *> *>(dest);
         assert(targ);
         assert(targ->t());
         destBlock = targ->t()->block();
         destFunc = targ->t()->func();
         assert(destBlock);
         break;
      }
      case TargetInt::BlockTarget:
         destBlock = (static_cast<Target<block_instance *> *>(dest))->t();
         assert(destBlock);
         break;
      default:
         assert(0);
         break;
   }
   EmulatorTracker *e = new EmulatorTracker(dest->origAddr(), destBlock, destFunc);
   return e;
}

TrackerElement *CFWidget::addrTracker(Address addr, const RelocBlock *trace) const {
   EmulatorTracker *e = new EmulatorTracker(addr, trace->block(), trace->func());
   return e;
}

TrackerElement *CFWidget::padTracker(Address addr, unsigned size, const RelocBlock *trace) const {
   PaddingTracker *p = new PaddingTracker(addr, size, trace->block(), trace->func());
   return p;
}

void CFWidget::addDestination(Address index, TargetInt *dest) {
   assert(dest);
   relocation_cerr << "CFWidget @ " << std::hex << addr() << ", adding destination " << dest->format()
                   << " / " << index << std::dec << endl;

   destMap_[index] = dest;
}

TargetInt *CFWidget::getDestination(Address dest) const {
   CFWidget::DestinationMap::const_iterator d_iter = destMap_.find(dest);
   if (d_iter != destMap_.end()) {
      return d_iter->second;
   }
   return NULL;
}


bool CFWidget::generateBranch(CodeBuffer &buffer,
			    TargetInt *to,
			    Instruction::Ptr insn,
                            const RelocBlock *trace,
			    bool fallthrough) {
   assert(to);
   if (!to->necessary()) return true;

   // We can put in an unconditional branch as an ender for 
   // a block that doesn't have a real branch. So if we don't have
   // an instruction generate a "generic" branch

   // We can see a problem where we want to branch to (effectively) 
   // the next instruction. So if we ever see that (a branch of offset
   // == size) back up the codeGen and shrink us down.

   CFPatch *newPatch = new CFPatch(CFPatch::Jump, insn, to, trace->func(), addr_);

   if (fallthrough || trace->block() == NULL) {
      buffer.addPatch(newPatch, destTracker(to));
   }
   else {
      buffer.addPatch(newPatch, tracker(trace));
   }
  
   return true;
}

bool CFWidget::generateCall(CodeBuffer &buffer,
			  TargetInt *to,
                          const RelocBlock *trace,
			  Instruction::Ptr insn) {
   if (!to) {
      // This can mean an inter-module branch...
      // DebugBreak();
      return true;
   }

   CFPatch *newPatch = new CFPatch(CFPatch::Call, insn, to, trace->func(), addr_);

   buffer.addPatch(newPatch, tracker(trace));

   return true;
}

bool CFWidget::generateConditionalBranch(CodeBuffer &buffer,
				       TargetInt *to,
                                       const RelocBlock *trace,
				       Instruction::Ptr insn) {
   assert(to);

   CFPatch *newPatch = new CFPatch(CFPatch::JCC, insn, to, trace->func(), addr_);

   buffer.addPatch(newPatch, tracker(trace));

   return true;
}

bool CFWidget::generateAddressTranslator(CodeBuffer &buffer,
                                       const codeGen &templ,
                                       Register &reg) 
{
   return true;
#if 0
   if (!templ.addrSpace()->isMemoryEmulated() ||
       BPatch_defensiveMode != block()->func()->obj()->hybridMode())
      return true;

   if (insn_->getOperation().getID() == e_ret_near ||
       insn_->getOperation().getID() == e_ret_far) {
      // Oops!
      return true;
   }
   if (!insn_->readsMemory()) {
      return true;
   }
   
   BPatch_memoryAccessAdapter converter;
   BPatch_memoryAccess *acc = converter.convert(insn_, addr_, false);
   if (!acc) {
      reg = Null_Register;
      return true;
   }
   
   codeGen patch(128);
   patch.applyTemplate(templ);
   
   // TODO: we probably want this in a form that doesn't stomp the stack...
   // But we can probably get away with this for now. Check that.

   // step 1: create space on the stack. 
   ::emitPush(RealRegister(REGNUM_EAX), patch);
   
   // step 2: save registers that will be affected by the call
   ::emitPush(RealRegister(REGNUM_ECX), patch);
   ::emitPush(RealRegister(REGNUM_EDX), patch);
   ::emitPush(RealRegister(REGNUM_EAX), patch);
   
   // Step 3: LEA this sucker into ECX.
   const BPatch_addrSpec_NP *start = acc->getStartAddr(0);
   if (start->getReg(0) == REGNUM_ESP ||
       start->getReg(1) == REGNUM_ESP) {
      cerr << "ERROR: CF insn that uses the stack pointer! " << insn_->format() << endl;
   }

   int stackShift = -16;
   // If we are a call _instruction_ but isCall is false, then we've got an extra word
   // on the stack from an emulated return address
   if (!isCall_ && insn_->getCategory() == c_CallInsn) stackShift -= 4;

   emitASload(start, REGNUM_ECX, stackShift, patch, true);
   
   // Step 4: save flags post-LEA
   emitSimpleInsn(0x9f, patch);
   emitSaveO(patch);
   ::emitPush(RealRegister(REGNUM_EAX), patch);
   
   // This might look a lot like a memEmulatorWidget. That's, well, because it
   // is. 
   buffer.addPIC(patch, tracker());
   
   // Where are we going?
   block_instance *func = templ.addrSpace()->findOnlyOneFunction("RTtranslateMemory");
   // FIXME for static rewriting; this is a dynamic-only hack for proof of concept.
   assert(func);
   
   // Now we start stealing from memEmulatorWidget. We need to call our translation function,
   // which means a non-PIC patch to the CodeBuffer. I don't feel like rewriting everything,
   // so there we go.
   buffer.addPatch(new MemEmulatorPatch(REGNUM_ECX, REGNUM_ECX, addr_, func->getAddress()),
                   tracker());
   patch.setIndex(0);
   
   // Restore flags
   ::emitPop(RealRegister(REGNUM_EAX), patch);
   emitRestoreO(patch);
   emitSimpleInsn(0x9E, patch);
   ::emitPop(RealRegister(REGNUM_EAX), patch);
   ::emitPop(RealRegister(REGNUM_EDX), patch);
   
   // ECX now holds the pointer to the destination...
   // Dereference
   ::emitMovRMToReg(RealRegister(REGNUM_ECX),
                    RealRegister(REGNUM_ECX),
                    0,
                    patch);
   
   // ECX now holds the _actual_ destination, so move it on to the stack. 
   // We've got ECX saved
   ::emitMovRegToRM(RealRegister(REGNUM_ESP),
                    1*4, 
                    RealRegister(REGNUM_ECX),
                    patch);
   ::emitPop(RealRegister(REGNUM_ECX), patch);
   // And tell our people to use the top of the stack
   // for their work.
   // TODO: trust liveness and leave this in a register. 

   buffer.addPIC(patch, tracker());
   reg = REGNUM_ESP;
   return true;
#endif
}

std::string CFWidget::format() const {
   stringstream ret;
   ret << "CFWidget(" << std::hex;
   ret << addr_ << ",";
   if (isIndirect_) ret << "<ind>";
   if (isConditional_) ret << "<cond>";
   if (isCall_) ret << "<call>";
		     
   for (DestinationMap::const_iterator iter = destMap_.begin();
        iter != destMap_.end();
        ++iter) {
      switch (iter->first) {
         case Fallthrough:
            ret << "FT";
            break;
         case Taken:
            ret << "T";
            break;
         default:
            ret << iter->first;
            break;
      }
      ret << "->" << (iter->second ? iter->second->format() : "<NULL>") << ",";
   }
   ret << std::dec << ")";
   return ret.str();
}

#if 0
unsigned CFWidget::size() const
{ 
   if (insn_ != NULL) 
      return insn_->size(); 
   return 0;
}
#endif

/////////////////////////
// Patching!
/////////////////////////

CFPatch::CFPatch(Type a,
                 InstructionAPI::Instruction::Ptr b,
                 TargetInt *c,
		 const func_instance *d,
                 Address e) :
  type(a), orig_insn(b), target(c), func(d), origAddr_(e) {
  if (b)
    ugly_insn = new instruction(b->ptr());
  else
    ugly_insn = NULL;
}

CFPatch::~CFPatch() { 
  if (ugly_insn) delete ugly_insn;
}

unsigned CFPatch::estimate(codeGen &) {
   if (orig_insn) {
      return orig_insn->size();
   }
   return 0;
}

bool PaddingPatch::apply(codeGen &gen, CodeBuffer *) {
   //cerr << "PaddingPatch::apply, current addr " << hex << gen.currAddr() << ", size " << size_ << ", registerDefensive " << (registerDefensive_ ? "<true>" : "<false>") << dec << endl;
   if (1 || noop_) {
      gen.fill(size_, codeGen::cgNOP);
   }
   else if ( 0 == (size_ % 2) ) {
      gen.fill(size_, codeGen::cgIllegal);
   } else {
      gen.fill(size_, codeGen::cgTrap);
   }
   //cerr << "\t After filling, current addr " << hex << gen.currAddr() << dec << endl;
   //gen.fill(10, codeGen::cgNOP);
   return true;
}

unsigned PaddingPatch::estimate(codeGen &) {
   return size_;;
}
