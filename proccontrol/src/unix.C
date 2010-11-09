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

#include "proccontrol/src/unix.h"
#include "proccontrol/src/snippets.h"
#include "proccontrol/src/procpool.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "common/h/Types.h"
#if defined(os_linux)
#include "common/h/linuxKludges.h"
#elif defined(os_freebsd)
#include "common/h/freebsdKludges.h"
#endif


void int_notify::writeToPipe()
{
   if (pipe_out == -1) 
      return;

   char c = 'e';
   ssize_t result = write(pipe_out, &c, 1);
   if (result == -1) {
      int error = errno;
      setLastError(err_internal, "Could not write to notification pipe\n");
      perr_printf("Error writing to notification pipe: %s\n", strerror(error));
      return;
   }
   pthrd_printf("Wrote to notification pipe %d\n", pipe_out);
}

void int_notify::readFromPipe()
{
   if (pipe_out == -1)
      return;

   char c;
   ssize_t result;
   int error;
   do {
      result = read(pipe_in, &c, 1);
      error = errno;
   } while (result == -1 && error == EINTR);
   if (result == -1) {
      int error = errno;
      if (error == EAGAIN) {
         pthrd_printf("Notification pipe had no data available\n");
         return;
      }
      setLastError(err_internal, "Could not read from notification pipe\n");
      perr_printf("Error reading from notification pipe: %s\n", strerror(error));
   }
   assert(result == 1 && c == 'e');
   pthrd_printf("Cleared notification pipe %d\n", pipe_in);
}

bool int_notify::createPipe()
{
   if (pipe_in != -1 || pipe_out != -1)
      return true;

   int fds[2];
   int result = pipe(fds);
   if (result == -1) {
      int error = errno;
      setLastError(err_internal, "Error creating notification pipe\n");
      perr_printf("Error creating notification pipe: %s\n", strerror(error));
      return false;
   }
   assert(fds[0] != -1);
   assert(fds[1] != -1);

   result = fcntl(fds[0], F_SETFL, O_NONBLOCK);
   if (result == -1) {
      int error = errno;
      setLastError(err_internal, "Error setting properties of notification pipe\n");
      perr_printf("Error calling fcntl for O_NONBLOCK on %d: %s\n", fds[0], strerror(error));
      return false;
   }
   pipe_in = fds[0];
   pipe_out = fds[1];


   pthrd_printf("Created notification pipe: in = %d, out = %d\n", pipe_in, pipe_out);
   return true;
}

unix_process::unix_process(Dyninst::PID p, std::string e, std::vector<std::string> a, std::map<int,int> f) :
   int_process(p, e, a, f)
{
}

unix_process::unix_process(Dyninst::PID pid_, int_process *p) : 
   int_process(pid_, p)
{
}

unix_process::~unix_process()
{
}

void unix_process::plat_execv() {
    // Never returns
    typedef const char * const_str;

    const_str *new_argv = (const_str *) calloc(argv.size()+2, sizeof(char *));
    for (unsigned i=0; i<argv.size(); ++i) {
        new_argv[i] = argv[i].c_str();
    }
    new_argv[argv.size()] = (char *) NULL;

    for(std::map<int,int>::iterator fdit = fds.begin(),
            fdend = fds.end();
            fdit != fdend;
            ++fdit) {
        int oldfd = fdit->first;
        int newfd = fdit->second;

        int result = close(newfd);
        if (result == -1) {
            pthrd_printf("Could not close old file descriptor to redirect.\n");
            setLastError(err_internal, "Unable to close file descriptor for redirection");
            exit(-1);
        }

        result = dup2(oldfd, newfd);
        if (result == -1) {
            pthrd_printf("Could not redirect file descriptor.\n");
            setLastError(err_internal, "Failed dup2 call.");
            exit(-1);
        }
        pthrd_printf("DEBUG redirected file!\n");
    }

    execv(executable.c_str(), const_cast<char * const*>(new_argv));
    int errnum = errno;         
    pthrd_printf("Failed to exec %s: %s\n", 
               executable.c_str(), strerror(errnum));
    if (errnum == ENOENT)
        setLastError(err_nofile, "No such file");
    if (errnum == EPERM || errnum == EACCES)
        setLastError(err_prem, "Permission denied");
    else
        setLastError(err_internal, "Unable to exec process");
    exit(-1);
}

bool unix_process::post_forked()
{
   ProcPool()->condvar()->lock();
   int_thread *thrd = threadPool()->initialThread();
   //The new process is currently stopped, but should be moved to 
   // a running state when handlers complete.
   pthrd_printf("Setting state of initial thread after fork in %d\n",
                getPid());
   thrd->setGeneratorState(int_thread::stopped);
   thrd->setHandlerState(int_thread::stopped);
   thrd->setInternalState(int_thread::running);
   thrd->setUserState(int_thread::running);
   ProcPool()->condvar()->broadcast();
   ProcPool()->condvar()->unlock();
   return true;
}

unsigned unix_process::getTargetPageSize() {
    static unsigned pgSize = 0;
    if( !pgSize ) pgSize = getpagesize();
    return pgSize;
}

// For compatibility 
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

bool unix_process::plat_collectAllocationResult(int_thread *thr, reg_response::ptr resp)
{
   switch (getTargetArch())
   {
      case Arch_x86_64: {
         bool result = thr->getRegister(x86_64::rax, resp);
         assert(result);
         break;
      }
      case Arch_x86: {
         bool result = thr->getRegister(x86::eax, resp);
         assert(result);
         break;
      }
      default:
         assert(0);
         break;
   }
   return true;
} 

bool unix_process::plat_createAllocationSnippet(Dyninst::Address addr, bool use_addr, unsigned long size, 
                                                void* &buffer, unsigned long &buffer_size, 
                                                unsigned long &start_offset)
{
   const void *buf_tmp = NULL;
   unsigned addr_size = 0;
   unsigned addr_pos = 0;
   unsigned flags_pos = 0;
   unsigned size_pos = 0;

   int flags = MAP_ANONYMOUS | MAP_PRIVATE;
   if (use_addr) 
      flags |= MAP_FIXED;
   else
      addr = 0x0;

   switch (getTargetArch())
   {
      case Arch_x86_64:
         buf_tmp = x86_64_call_mmap;
         buffer_size = x86_64_call_mmap_size;
         start_offset = x86_64_mmap_start_position;
         addr_pos = x86_64_mmap_addr_position;
         flags_pos = x86_64_mmap_flags_position;
         size_pos = x86_64_mmap_size_position;
         addr_size = 8;
         break;
      case Arch_x86:
         buf_tmp = x86_call_mmap;
         buffer_size = x86_call_mmap_size;
         start_offset = x86_mmap_start_position;
         addr_pos = x86_mmap_addr_position;
         flags_pos = x86_mmap_flags_position;
         size_pos = x86_mmap_size_position;
         addr_size = 4;
         break;
      default:
         assert(0);
   }
   
   buffer = malloc(buffer_size);
   memcpy(buffer, buf_tmp, buffer_size);

   //Assuming endianess of debugger and debugee match.
   *((unsigned int *) (((char *) buffer)+size_pos)) = size;
   *((unsigned int *) (((char *) buffer)+flags_pos)) = flags;
   if (addr_size == 8)
      *((unsigned long *) (((char *) buffer)+addr_pos)) = addr;
   else if (addr_size == 4)
      *((unsigned *) (((char *) buffer)+addr_pos)) = (unsigned) addr;
   else 
      assert(0);
   return true;
}

bool unix_process::plat_createDeallocationSnippet(Dyninst::Address addr, 
                                            unsigned long size, void* &buffer, 
                                            unsigned long &buffer_size, 
                                            unsigned long &start_offset)
{
   const void *buf_tmp = NULL;
   unsigned addr_size = 0;
   unsigned addr_pos = 0;
   unsigned size_pos = 0;

   switch (getTargetArch())
   {
      case Arch_x86_64:
         buf_tmp = x86_64_call_munmap;
         buffer_size = x86_64_call_munmap_size;
         start_offset = x86_64_munmap_start_position;
         addr_pos = x86_64_munmap_addr_position;
         size_pos = x86_64_munmap_size_position;
         addr_size = 8;
         break;
      case Arch_x86:
         buf_tmp = x86_call_munmap;
         buffer_size = x86_call_munmap_size;
         start_offset = x86_munmap_start_position;
         addr_pos = x86_munmap_addr_position;
         size_pos = x86_munmap_size_position;
         addr_size = 4;
         break;
      default:
         assert(0);
   }
   
   buffer = malloc(buffer_size);
   memcpy(buffer, buf_tmp, buffer_size);

   //Assuming endianess of debugger and debugee match.
   *((unsigned int *) (((char *) buffer)+size_pos)) = size;
   if (addr_size == 8)
      *((unsigned long *) (((char *) buffer)+addr_pos)) = addr;
   else if (addr_size == 4)
      *((unsigned *) (((char *) buffer)+addr_pos)) = (unsigned) addr;
   else 
      assert(0);
   return true;
}

Dyninst::Address unix_process::plat_mallocExecMemory(Dyninst::Address min, unsigned size) {
    Dyninst::Address result = 0x0;
    bool found_result = false;
    unsigned maps_size;
    map_entries *maps = getVMMaps(getPid(), maps_size);
    assert(maps); //TODO, Perhaps go to libraries for address map if no /proc/
    for (unsigned i=0; i<maps_size; i++) {
        if (!(maps[i].prems & PREMS_EXEC))
            continue;
        if (min + size > maps[i].end)
            continue;
        if (maps[i].end - maps[i].start < size)
            continue;

        if (maps[i].start > min)
            result = maps[i].start;
        else
            result = min;
        found_result = true;
        break;
    }
    assert(found_result);
    free(maps);
    return result;
}