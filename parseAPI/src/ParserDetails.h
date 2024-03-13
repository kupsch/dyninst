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
#ifndef _PARSER_DETAILS_H_
#define _PARSER_DETAILS_H_

#include <assert.h>
#include "IA_IAPI.h"

namespace Dyninst {
namespace ParseAPI {

namespace {
static inline bool is_code(Function * f, Address addr)
{
    return f->region()->isCode(addr) ||
           f->isrc()->isCode(addr);
}

}

class ParseWorkBundle;

class ParseWorkElem
{
 public:
    enum parse_work_order {
        seed_addr = 0,
        ret_fallthrough,
        call,
        call_fallthrough,
        cond_not_taken,
        cond_taken,
        br_direct,
        br_indirect,
        catch_block,
        checked_call_ft,
	resolve_jump_table,
        func_shared_code, 
        __parse_work_end__
    };

    ParseWorkElem(
            ParseWorkBundle *b, 
            parse_work_order o,
            Edge *e, 
            Address source,
            Address target, 
            bool resolvable,
            bool tailcall)
        : _bundle(b),
          _edge(e),   
          _src(source),       
          _targ(target),
          _can_resolve(resolvable),
          _tailcall(tailcall),
          _order(o),
          _call_processed(false),
          _cur(NULL),
          _shared_func(NULL) { }

    ParseWorkElem(
            ParseWorkBundle *b, 
            Edge *e, 
            Address source,
            Address target, 
            bool resolvable,
            bool tailcall)
        : _bundle(b),
          _edge(e),
          _src(source),
          _targ(target),
          _can_resolve(resolvable),
          _tailcall(tailcall),
          _order(__parse_work_end__),
          _call_processed(false),
          _cur(NULL),
          _shared_func(NULL)

    { 
      if(e) {
        switch(e->type()) {
            case CALL:
                _order = call; break;
            case COND_TAKEN:
                {
                    if (tailcall) {
                        _order = call;
                    } else {
                        _order = cond_taken; 
                    }
                    break;
                }
            case COND_NOT_TAKEN:
                _order = cond_not_taken; break;
            case INDIRECT:
                _order = br_indirect; break;
            case DIRECT:
                {
                    if (tailcall) {
                        _order = call;
                    } else {
                        _order = br_direct; 
                    }
                    break;
                }
            case FALLTHROUGH:
                _order = ret_fallthrough; break;
            case CATCH:
                _order = catch_block; break;
            case CALL_FT:
                _order = call_fallthrough; break;
            default:
                fprintf(stderr,"[%s:%d] FATAL: bad edge type %d\n",
                    FILE__,__LINE__,e->type());
                assert(0);
        } 
      } else 
        _order = seed_addr;
    }

    ParseWorkElem()
        : _bundle(NULL),
          _edge(NULL),
          _src(0),
          _targ((Address)-1),
          _can_resolve(false),
          _tailcall(false),
          _order(__parse_work_end__),
          _call_processed(false),
          _cur(NULL),
          _shared_func(NULL)
    { } 

    ParseWorkElem(ParseWorkBundle *bundle, Block *b, const InsnAdapter::IA_IAPI* ah)
         : _bundle(bundle),
          _edge(NULL),
          _targ((Address)-1),
          _can_resolve(false),
          _tailcall(false),
          _order(resolve_jump_table),
          _call_processed(false),
	  _cur(b) {	      
	      _ah = ah->clone();
              _src = _ah->getAddr();
              _shared_func = NULL;
	  }

    ParseWorkElem(ParseWorkBundle *bundle, Function *f):
          _bundle(bundle),
          _targ((Address)-1),
          _can_resolve(false),
          _tailcall(false),
          _order(func_shared_code),
          _call_processed(false),
          _cur(NULL) {
              _shared_func = f;
              _src = 0;
          }
               

    ~ParseWorkElem() {
    }

      

    ParseWorkBundle *   bundle()        const { return _bundle; }
    Edge *              edge()          const { return _edge; }
    Address             source()        const { return _src; }
    Address             target()        const { return _targ; }
    bool                resolvable()    const { return _can_resolve; }
    parse_work_order    order()         const { return _order; }
    void                setTarget(Address t)  { _targ = t; }

    bool                tailcall()      const { return _tailcall; }
    bool                callproc()      const { return _call_processed; }
    void                mark_call()     { _call_processed = true; }

    Block*          cur()           const { return _cur; }
    InsnAdapter::IA_IAPI*  ah()        const { return _ah; }
    Function*       shared_func()       const { return _shared_func; }

    struct compare {
        bool operator()(const ParseWorkElem * e1, const ParseWorkElem * e2) const
        {
            parse_work_order o1 = e1->order();
            parse_work_order o2 = e2->order();   

            if(o1 > o2)
                return true;
            else if(o1 < o2)
                return false;
            else 
	        return e1->target() > e2->target();
        }
    };

 private:
    ParseWorkBundle * _bundle{};
    Edge * _edge{};
    Address _src{};
    Address _targ{};
    bool _can_resolve{};
    bool _tailcall{};
    parse_work_order _order{};
    bool _call_processed{};
    Block* _cur{};
    InsnAdapter::IA_IAPI* _ah{};
    Function * _shared_func{};
};

class ParseWorkBundle
{
 public:
    ParseWorkBundle() {}
    ~ParseWorkBundle()
    {
        for(unsigned i=0;i<_elems.size();++i)
            delete _elems[i];
    }

    ParseWorkElem* add(ParseWorkElem * e) 
    { 
        _elems.push_back(e);
        return e;
    }
    vector<ParseWorkElem*> const& elems() { return _elems; }
 private:
    vector<ParseWorkElem*> _elems;
};

} // ParseAPI
} // Dyninst

#endif
