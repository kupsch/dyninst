/*
 * Copyright (c) 1996-2004 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
 * 
 * This license is for research uses.  For such uses, there is no
 * charge. We define "research use" to mean you may freely use it
 * inside your organization for whatever purposes you see fit. But you
 * may not re-distribute Paradyn or parts of Paradyn, in any form
 * source or binary (including derivatives), electronic or otherwise,
 * to any other organization or entity without our permission.
 * 
 * (for other uses, please contact us at paradyn@cs.wisc.edu)
 * 
 * All warranties, including without limitation, any warranty of
 * merchantability or fitness for a particular purpose, are hereby
 * excluded.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * Even if advised of the possibility of such damages, under no
 * circumstances shall we (or any other person or entity with
 * proprietary rights in the software licensed hereunder) be liable
 * to you or any third party for direct, indirect, or consequential
 * damages of any character regardless of type of action, including,
 * without limitation, loss of profits, loss of use, loss of good
 * will, or computer failure or malfunction.  You agree to indemnify
 * us (and any other person or entity with proprietary rights in the
 * software licensed hereunder) for any and all liability it may
 * incur to third parties resulting from your use of Paradyn.
 */

/*
 * Report statistics about dyninst and data collection.
 * $Id: stats.C,v 1.2 2008/07/01 19:26:49 legendre Exp $
 */

#include <sstream>
#include <string>
#include "common/h/stats.h"
#include "dynutil/h/util.h"

#if 0
#include "dyninstAPI/src/stats.h"

CntStatistic trampBytes;
CntStatistic pointsUsed;
CntStatistic insnGenerated;
CntStatistic totalMiniTramps;
double timeCostLastChanged=0;
// HTable<resourceListRec*> fociUsed;
// HTable<metric*> metricsUsed;
CntStatistic ptraceOtherOps, ptraceOps, ptraceBytes;

void printDyninstStats()
{
    sprintf(errorLine, "    %ld total points used\n", pointsUsed.value());
    logLine(errorLine);
    sprintf(errorLine, "    %ld mini-tramps used\n", totalMiniTramps.value());
    logLine(errorLine);
    sprintf(errorLine, "    %ld tramp bytes\n", trampBytes.value());
    logLine(errorLine);
    sprintf(errorLine, "    %ld ptrace other calls\n", ptraceOtherOps.value());
    logLine(errorLine);
    sprintf(errorLine, "    %ld ptrace write calls\n", 
                ptraceOps.value()-ptraceOtherOps.value());
    logLine(errorLine);
    sprintf(errorLine, "    %ld ptrace bytes written\n", ptraceBytes.value());
    logLine(errorLine);
    sprintf(errorLine, "    %ld instructions generated\n", 
                insnGenerated.value());
    logLine(errorLine);
}
#endif

StatContainer::StatContainer() 
   //stats_(::Dyninst::hash)
{
}

Statistic*
StatContainer::operator[](std::string &name) 
{
    if (stats_.find(name) != stats_.end()) {
        return stats_[name];
    } else {
        return NULL;
    }
}

void
StatContainer::add(std::string name, StatType type)
{
    Statistic *s = NULL;

    switch(type) {
        case CountStat:
            s = (Statistic *)(new CntStatistic(this));
            break;            
        case TimerStat:
            s = (Statistic *)(new TimeStatistic(this));
            break;
        default:
          fprintf(stderr,"Error adding statistic %s: type %d not recognized\n",
                name.c_str(), type);
          return;
    }

    stats_[name] = s;
}

void StatContainer::startTimer(std::string name) 
{
    TimeStatistic *timer = dynamic_cast<TimeStatistic *>(stats_[name]);
    if (!timer) return;
    timer->start();
}

void StatContainer::stopTimer(std::string name) {
    TimeStatistic *timer = dynamic_cast<TimeStatistic *>(stats_[name]);
    if (!timer) return;
    timer->stop();
}

void StatContainer::incrementCounter(std::string name) {
    CntStatistic *counter = dynamic_cast<CntStatistic *>(stats_[name]);
    if (!counter) return;
    (*counter)++;
}

void StatContainer::decrementCounter(std::string name) {
    CntStatistic *counter = dynamic_cast<CntStatistic *>(stats_[name]);
    if (!counter) return;
    (*counter)--;
}

void StatContainer::addCounter(std::string name, int val) {
    CntStatistic *counter = dynamic_cast<CntStatistic *>(stats_[name]);
    if (!counter) return;
    (*counter) += val;
}

                                  
CntStatistic 
CntStatistic::operator++( int )
{
    CntStatistic ret = *this;
    ++(*this);
    return ret;
}


CntStatistic& 
CntStatistic::operator++()
{
    cnt_++;
    return *this;
}
                               
CntStatistic 
CntStatistic::operator--( int )
{
    CntStatistic ret = *this;
    --(*this);
    return ret;
}

CntStatistic&
CntStatistic::operator--()
{
    cnt_--;
    return *this;
}
                                 
CntStatistic& 
CntStatistic::operator=( long int v )
{
    cnt_ = v;
    return *this;
}

CntStatistic& 
CntStatistic::operator=(CntStatistic & v )
{
    if( this != &v ) {
        cnt_ = v.cnt_;
    }
    return *this;
}
                                     
CntStatistic& 
CntStatistic::operator+=( long int v )
{
    cnt_ += v;
    return *this;
}

CntStatistic&
CntStatistic::operator+=( CntStatistic & v )
{
    cnt_ += v.cnt_;
    return *this;
}

    
CntStatistic& 
CntStatistic::operator-=( long int v )
{
    cnt_ -= v;
    return *this;
}

CntStatistic& 
CntStatistic::operator-=( CntStatistic & v)
{
    cnt_ -= v.cnt_;
    return *this;
}

inline long int 
CntStatistic::operator*()
{
    return cnt_;
}

long int 
CntStatistic::value()
{
    return cnt_;
}


TimeStatistic& 
TimeStatistic::operator=(TimeStatistic & t)
{
    if( this != &t ) {
        t_ = t.t_;
    }
    return *this;
}

TimeStatistic& 
TimeStatistic::operator+=(TimeStatistic & t)
{
    t_ += t.t_;
    return *this;
}

TimeStatistic& 
TimeStatistic::operator+(TimeStatistic & t) 
{
    TimeStatistic * ret = new TimeStatistic;
    *ret = *this;
    *ret += t;
    return *ret;
}

void 
TimeStatistic::clear()
{
    t_.clear();
}

void 
TimeStatistic::start()
{
    t_.start();
}

void
TimeStatistic::stop()
{
    t_.stop();
}

double 
TimeStatistic::usecs() 
{
    return t_.usecs();
}

double 
TimeStatistic::ssecs() 
{
    return t_.ssecs();
}

double 
TimeStatistic::wsecs() 
{
    return t_.wsecs();
}

bool 
TimeStatistic::is_running() 
{
    return t_.is_running();
}
