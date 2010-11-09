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

#if !defined SYMTAB_READER_H_
#define SYMTAB_READER_H_

#include "SymReader.h"
#include <string>
#include <vector>
//Some components (StackwalkerAPI, ProcControlAPI) use a SymReader (defined in dyn_util/h)
// to read symbols rather than a straight dependency on SymtabAPI.  A component can
// either define its own SymReader (as ProcControlAPI does) or it can use SymtabAPI as
// its symbol reader.  These SymtabReader and SymtabReaderFactory implement the SymReader
// interface with a SymtabAPI implementation.

namespace Dyninst {
namespace SymtabAPI {

class Symtab;
class Region;
class FastParser;

class SymtabReaderFactory : public SymbolReaderFactory
{
  private:
   std::map<std::string, SymReader *> open_syms;
  public:
   SymtabReaderFactory();
   virtual ~SymtabReaderFactory();
   virtual SymReader *openSymbolReader(std::string pathname);
   virtual SymReader *openSymbolReader(const char *buffer, unsigned long size);
   virtual bool closeSymbolReader(SymReader *sr);
};

class SymtabReader : public SymReader {
   friend class SymtabReaderFactory;
  protected:
   Symtab *symtab;
   int ref_count;
   std::vector<Region *> *mapped_regions;
  public:
   SymtabReader(std::string file_);
   SymtabReader(const char *buffer, unsigned long size);
   virtual ~SymtabReader();

   virtual Symbol_t getSymbolByName(std::string symname);
   virtual unsigned long getSymbolSize(const Symbol_t &sym);
   virtual Symbol_t getContainingSymbol(Dyninst::Offset offset);
   virtual std::string getInterpreterName();
   virtual unsigned getAddressWidth();
   
   virtual unsigned numRegions();
   virtual bool getRegion(unsigned num, SymRegion &reg); 

   virtual Dyninst::Offset getSymbolOffset(const Symbol_t &sym);
   virtual std::string getSymbolName(const Symbol_t &sym);
   virtual std::string getDemangledName(const Symbol_t &sym);
   virtual bool isValidSymbol(const Symbol_t &sym);

   virtual Section_t getSectionByName(std::string name);
   virtual Section_t getSectionByAddress(Dyninst::Address addr);
   virtual Dyninst::Address getSectionAddress(Section_t sec);
   virtual std::string getSectionName(Section_t sec);
   virtual bool isValidSection(Section_t sec);

   virtual Dyninst::Offset imageOffset();
   virtual Dyninst::Offset dataOffset();   
};

extern "C" {
   SymbolReaderFactory *getSymtabReaderFactory();
}

}
}

#endif