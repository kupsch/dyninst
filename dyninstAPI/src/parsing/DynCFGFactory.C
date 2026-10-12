#include "DynCFGFactory.h"
#include "Function.h"
#include "image.h"
#include "parse_block.h"
#include "parse_func.h"
#include "relocationEntry.h"

#include <boost/thread/lock_guard.hpp>
#include <limits>

namespace {

  class PLTFunction : public Dyninst::SymtabAPI::Function {
    Dyninst::SymtabAPI::relocationEntry r;

  public:
    explicit PLTFunction(Dyninst::SymtabAPI::relocationEntry re)
        : Dyninst::SymtabAPI::Function(re.getDynSym()), r{re} {
    }

    std::string getName() const override {
      return r.name();
    }

    Offset getOffset() const override {
      return r.target_addr();
    }

    unsigned int getSize() const override {
      return 0;
    }

    SymtabAPI::Module *getModule() const override {
      return nullptr;
    }
  };


  // ARM and AArch64 mark each run of code or data with a mapping symbol -- $a,
  // $d, $t or $x, optionally followed by '.' and more -- at the address the run
  // begins.  They are untyped and they sit exactly where a function can start,
  // so they would otherwise be taken for its name.  They name a stretch of the
  // section, not anything in it.
  bool isMappingSymbol(const std::string &name) {
    if (name.size() < 2 || name[0] != '$') {
      return false;
    }
    if (name[1] != 'a' && name[1] != 'd' && name[1] != 't' && name[1] != 'x') {
      return false;
    }
    return name.size() == 2 || name[2] == '.';
  }


  // Whether this symbol is one whose name a function may take.
  //
  // It is almost always an untyped one.  Symtab aggregates every ST_FUNCTION
  // and ST_INDIRECT symbol into a Function keyed by offset, and the caller has
  // already looked this address up in that same map, so a typed symbol here
  // would have been found there.  What reaches us is ST_NOTYPE or ST_CODE:
  // hand-written assembly routinely labels an entry point without typing it,
  // and such a name is still the one the author gave it.  ST_FUNCTION is
  // accepted anyway for the one case that escapes the map -- in a relocatable
  // file an offset does not identify a symbol, so those functions are
  // deliberately kept out of it.
  bool namesAFunction(Dyninst::SymtabAPI::Symbol *sym) {
    using Dyninst::SymtabAPI::Symbol;
    Symbol::SymbolType t = sym->getType();
    if (t != Symbol::ST_FUNCTION && t != Symbol::ST_NOTYPE && t != Symbol::ST_CODE) {
      return false;
    }
    return !sym->getMangledName().empty() && !isMappingSymbol(sym->getMangledName());
  }


  // Several symbols can share an address, and the order findSymbolByOffset
  // returns them in is not defined, so without a total order the name a
  // function reports would vary.  Prefer a typed symbol, then a global one,
  // then the name that sorts first.
  bool outranks(Dyninst::SymtabAPI::Symbol *lhs, Dyninst::SymtabAPI::Symbol *rhs) {
    using Dyninst::SymtabAPI::Symbol;
    auto rank = [](Symbol *s) {
      int r = 0;
      if (s->getType() == Symbol::ST_FUNCTION) {
        r += 2;
      }
      if (s->getLinkage() == Symbol::SL_GLOBAL) {
        r += 1;
      }
      return r;
    };
    if (rank(lhs) != rank(rhs)) {
      return rank(lhs) > rank(rhs);
    }
    return lhs->getMangledName() < rhs->getMangledName();
  }

}

namespace Dyninst { namespace DyninstAPI {

  ParseAPI::Block *DynCFGFactory::mkblock(ParseAPI::Function *f, ParseAPI::CodeRegion *r,
                                          Address addr) {
    auto *block = new parse_block(static_cast<parse_func *>(f), r, addr);

    if (_img->trackNewBlocks_) {
      _img->newBlocks_.push_back(block);
    }
    return block;
  }

  ParseAPI::Edge *DynCFGFactory::mkedge(ParseAPI::Block *src, ParseAPI::Block *trg,
                                        ParseAPI::EdgeTypeEnum type) {
    return new ParseAPI::Edge(src, trg, type);
  }

  ParseAPI::Function *DynCFGFactory::mkfunc(Address addr, ParseAPI::FuncSource src,
                                            std::string name, ParseAPI::CodeObject *obj,
                                            ParseAPI::CodeRegion *reg,
                                            InstructionSource *isrc) {
    boost::lock_guard<decltype(_mtx)> _lock{_mtx};

    SymtabAPI::Symtab *st = _img->getObject();
    SymtabAPI::Function *stf{};
    pdmodule *pdmod = _img->getOrCreateModule(st->getDefaultModule());

    auto found = obj->cs()->linkage().find(addr);
    // PLT stub
    if (found != obj->cs()->linkage().end()) {
      name = found->second;
      std::vector<SymtabAPI::relocationEntry> relocs;
      st->getFuncBindingTable(relocs);
      for (auto i = relocs.begin(); i != relocs.end(); i++) {
        if (i->target_addr() == found->first) {
          stf = new PLTFunction(*i);
          break;
        }
      }
      if (stf && stf->getFirstSymbol()) {
        auto *ret = parse_func::plt_func(stf, pdmod, _img, obj, reg, isrc, src);
        // PLT stubs are typically are undefined symbols in the binary,
        // so there is no corresponding SymtabAPI::Function at Symtab level.
        // PLTFunction is a subclass of SymtabAPI::Function to represent PLT stubs.
        // However, since there is no easy way to add a PLTFunction back to the
        // Symtab object, we need to add PLTFunction to a data structure for
        // future lookup.
        _img->insertPLTParseFuncMap(stf->getName(), ret);
        return ret;
      }
    }
    if (!st->findFuncByEntryOffset(stf, addr)) {
      // The parser hands us an invented targ<addr> for a function it found by
      // following control flow rather than from a hint.  When the symbol table
      // names that address, that name is the real one and belongs on the
      // function: an object excluded from analysis is parsed with no hints at
      // all, so everything an on-demand parse reaches arrives here unnamed
      // even though its symbol was there the whole time.
      //
      // Only for an excluded object.  An ordinary parse arrives here with its
      // hints already applied, and whatever it has not named by now it has
      // decided not to name; renaming there would change what every existing
      // caller of an ordinary parse sees.
      SymtabAPI::Symbol *best{};
      if (_img->analysisExcluded()) {
        for (auto *sym : st->findSymbolByOffset(addr)) {
          if (!namesAFunction(sym)) {
            continue;
          }
          if (!best || outranks(sym, best)) {
            best = sym;
          }
        }
      }
      if (best) {
        name = best->getMangledName();
      }
      stf = st->createFunction(name, addr, 0, pdmod->mod());
    } else {
      pdmod = _img->getOrCreateModule(stf->getModule());
    }
    assert(stf);

    return new parse_func(stf, pdmod, _img, obj, reg, isrc, src);
  }

  ParseAPI::Block *DynCFGFactory::mksink(ParseAPI::CodeObject *obj,
                                         ParseAPI::CodeRegion *r) {
    return new parse_block(obj, r, std::numeric_limits<Address>::max());
  }

}}
