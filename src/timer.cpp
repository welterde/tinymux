// timer.cpp -- Mini-task scheduler for timed events.
//
// $Id: timer.cpp,v 1.10 2002-02-13 18:57:29 sdennis Exp $
//
// MUX 2.1
// Copyright (C) 1998 through 2001 Solid Vertical Domains, Ltd. All
// rights not explicitly given are reserved. Permission is given to
// use this code for building and hosting text-based game servers.
// Permission is given to use this code for other non-commercial
// purposes. To use this code for commercial purposes other than
// building/hosting text-based game servers, contact the author at
// Stephen Dennis <sdennis@svdltd.com> for another license.
//

#include "copyright.h"
#include "autoconf.h"
#include "config.h"
#include "externs.h"

#include <signal.h>

#include "interface.h"
#include "command.h"
#include "mguests.h"

extern void NDECL(pool_reset);
extern void FDECL(fork_and_dump, (int key));
extern void NDECL(pcache_trim);

CScheduler scheduler;

// Free List Reconstruction Task routine.
//
void dispatch_FreeListReconstruction(void *pUnused, int iUnused)
{
    if (mudconf.control_flags & CF_DBCHECK)
    {
        char *cmdsave = mudstate.debug_cmd;
        mudstate.debug_cmd = (char *)"< dbck >";
        do_dbck(NOTHING, NOTHING, 0);
#ifndef STANDALONE
        Guest.CleanUp();
#endif
        pcache_trim();
        pool_reset();
        mudstate.debug_cmd = cmdsave;
    }

    // Schedule ourselves again.
    //
    CLinearTimeAbsolute ltaNow;
    ltaNow.GetUTC();
    CLinearTimeDelta ltd;
    ltd.SetSeconds(mudconf.check_interval);
    mudstate.check_counter = ltaNow + ltd;
    scheduler.DeferTask(mudstate.check_counter, PRIORITY_SYSTEM, dispatch_FreeListReconstruction, 0, 0);
}

// Database Dump Task routine.
//
void dispatch_DatabaseDump(void *pUnused, int iUnused)
{
    int nNextTimeInSeconds = mudconf.dump_interval;

    if (mudconf.control_flags & CF_CHECKPOINT)
    {
        char *cmdsave = mudstate.debug_cmd;
        mudstate.debug_cmd = (char *)"< dump >";
#ifndef WIN32
        if (mudstate.dumping)
        {
            // There is a dump in progress. These usually happen very quickly.
            // We will reschedule ourselves to try again in 20 seconds.
            // Ordinarily, you would think "...a dump is a dump...", but some
            // dumps might not be the type of dump we're going to do.
            //
            nNextTimeInSeconds = 20;
        }
        else
#endif
        {
            fork_and_dump(0);
        }
        mudstate.debug_cmd = cmdsave;
    }

    // Schedule ourselves again.
    //
    CLinearTimeAbsolute ltaNow;
    ltaNow.GetUTC();
    CLinearTimeDelta ltd;
    ltd.SetSeconds(nNextTimeInSeconds);
    mudstate.dump_counter = ltaNow + ltd;
    scheduler.DeferTask(mudstate.dump_counter, PRIORITY_SYSTEM, dispatch_DatabaseDump, 0, 0);
}

// Idle Check Task routine.
//
void dispatch_IdleCheck(void *pUnused, int iUnused)
{
    if (mudconf.control_flags & CF_IDLECHECK)
    {
        char *cmdsave = mudstate.debug_cmd;
        mudstate.debug_cmd = (char *)"< idlecheck >";
        check_idle();
        mudstate.debug_cmd = cmdsave;
    }

    // Schedule ourselves again.
    //
    CLinearTimeAbsolute ltaNow;
    ltaNow.GetUTC();
    CLinearTimeDelta ltd;
    ltd.SetSeconds(mudconf.idle_interval);
    mudstate.idle_counter = ltaNow + ltd;
    scheduler.DeferTask(mudstate.idle_counter, PRIORITY_SYSTEM, dispatch_IdleCheck, 0, 0);
}

// Check Events Task routine.
//
void dispatch_CheckEvents(void *pUnused, int iUnused)
{
    if (mudconf.control_flags & CF_EVENTCHECK)
    {
        char *cmdsave = mudstate.debug_cmd;
        mudstate.debug_cmd = (char *)"< eventcheck >";
        check_events();
        mudstate.debug_cmd = cmdsave;
    }

    // Schedule ourselves again.
    //
    CLinearTimeAbsolute ltaNow;
    ltaNow.GetUTC();
    CLinearTimeDelta ltd;
    ltd.SetSeconds(900);
    mudstate.events_counter = ltaNow + ltd;
    scheduler.DeferTask(mudstate.events_counter, PRIORITY_SYSTEM, dispatch_CheckEvents, 0, 0);
}

#ifndef MEMORY_BASED
void dispatch_CacheTick(void *pUnused, int iUnused)
{
    char *cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *)"< cachetick >";
    cache_tick();

    // Schedule ourselves again.
    //
    CLinearTimeAbsolute ltaNextTime;
    ltaNextTime.GetUTC();
    CLinearTimeDelta ltd;
    ltd.SetSeconds(30);
    ltaNextTime += ltd;
    scheduler.DeferTask(ltaNextTime, PRIORITY_SYSTEM, dispatch_CacheTick, 0, 0);
    mudstate.debug_cmd = cmdsave;
}
#endif

#if 0
void dispatch_CleanChannels(void *pUnused, int iUnused)
{
    char *cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *)"< cleanchannels >";
    do_cleanupchannels();

    // Schedule ourselves again.
    //
    CLinearTimeAbsolute ltaNextTime;
    ltaNextTime.GetUTC();
    CLinearTimeDelta ltd;
    ltd.SetSeconds(900);
    ltaNextTime += ltd;
    scheduler.DeferTask(ltaNextTime, PRIORITY_SYSTEM, dispatch_CleanChannels, 0, 0);
    mudstate.debug_cmd = cmdsave;
}
#endif

void dispatch_CanRestart(void *pUnused, int iUnused)
{
    mudstate.bCanRestart = TRUE;
}

void init_timer(void)
{
    CLinearTimeAbsolute ltaNow;
    ltaNow.GetUTC();

    // Setup re-occuring Free List Reconstruction task.
    //
    CLinearTimeDelta ltd;
    ltd.SetSeconds((mudconf.check_offset == 0) ? mudconf.check_interval : mudconf.check_offset);
    mudstate.check_counter  = ltaNow + ltd;
    scheduler.DeferTask(mudstate.check_counter, PRIORITY_SYSTEM, dispatch_FreeListReconstruction, 0, 0);

    // Setup re-occuring Database Dump task.
    //
    ltd.SetSeconds((mudconf.dump_offset == 0) ? mudconf.dump_interval : mudconf.dump_offset);
    mudstate.dump_counter  = ltaNow + ltd;
    scheduler.DeferTask(mudstate.dump_counter, PRIORITY_SYSTEM, dispatch_DatabaseDump, 0, 0);

    // Setup re-occuring Idle Check task.
    //
    ltd.SetSeconds(mudconf.idle_interval);
    mudstate.idle_counter   = ltaNow + ltd;
    scheduler.DeferTask(mudstate.idle_counter, PRIORITY_SYSTEM, dispatch_IdleCheck, 0, 0);

    // Setup re-occuring Check Events task.
    //
    ltd.SetSeconds(900);
    mudstate.events_counter = ltaNow + ltd;
    scheduler.DeferTask(mudstate.events_counter, PRIORITY_SYSTEM, dispatch_CheckEvents, 0, 0);

#ifndef MEMORY_BASED
    // Setup re-occuring cache_tick task.
    //
    ltd.SetSeconds(30);
    scheduler.DeferTask(ltaNow+ltd, PRIORITY_SYSTEM, dispatch_CacheTick, 0, 0);
#endif

#if 0
    // Setup comsys channel scrubbing.
    //
    ltd.SetSeconds(45);
    scheduler.DeferTask(ltaNow+ltd, PRIORITY_SYSTEM, dispatch_CleanChannels, 0, 0);
#endif

    // Setup one-shot task to enable restarting 10 seconds after startmux.
    //
    ltd.SetSeconds(15);
    scheduler.DeferTask(ltaNow+ltd, PRIORITY_OBJECT, dispatch_CanRestart, 0, 0);
}

/*
 * ---------------------------------------------------------------------------
 * * do_timewarp: Adjust various internal timers.
 */

void do_timewarp(dbref player, dbref cause, int key, char *arg)
{
    int secs;

    secs = Tiny_atol(arg);

    // Sem/Wait queues
    //
    if ((key == 0) || (key & TWARP_QUEUE))
    {
        do_queue(player, cause, QUEUE_WARP, arg);
    }

    // Once these are adjusted, we need to Cancel and reschedule the task.
    //
    CLinearTimeDelta ltd;
    ltd.SetSeconds(secs);
    if (key & TWARP_DUMP)
    {
        mudstate.dump_counter -= ltd;
        scheduler.CancelTask(dispatch_DatabaseDump, 0, 0);
        scheduler.DeferTask(mudstate.dump_counter, PRIORITY_SYSTEM, dispatch_DatabaseDump, 0, 0);
    }
    if (key & TWARP_CLEAN)
    {
        mudstate.check_counter -= ltd;
        scheduler.CancelTask(dispatch_FreeListReconstruction, 0, 0);
        scheduler.DeferTask(mudstate.check_counter, PRIORITY_SYSTEM, dispatch_FreeListReconstruction, 0, 0);
    }
    if (key & TWARP_IDLE)
    {
        mudstate.idle_counter -= ltd;
        scheduler.CancelTask(dispatch_IdleCheck, 0, 0);
        scheduler.DeferTask(mudstate.idle_counter, PRIORITY_SYSTEM, dispatch_IdleCheck, 0, 0);
    }
    if (key & TWARP_EVENTS)
    {
        mudstate.events_counter -= ltd;
        scheduler.CancelTask(dispatch_CheckEvents, 0, 0);
        scheduler.DeferTask(mudstate.events_counter, PRIORITY_SYSTEM, dispatch_CheckEvents, 0, 0);
    }
}

#define INITIAL_TASKS 100
#define EXTRA_TASKS    50

CTaskHeap::CTaskHeap(void)
{
    m_nCurrent = 0;
    m_iVisitedMark = 0;
    m_nAllocated = INITIAL_TASKS;
    m_pHeap = new PTASK_RECORD[m_nAllocated];
    if (!m_pHeap)
    {
        m_nAllocated = 0;
    }
}

CTaskHeap::~CTaskHeap(void)
{
    while (m_nCurrent--)
    {
        PTASK_RECORD pTask = m_pHeap[m_nCurrent];
        if (pTask) delete pTask;
    }
    if (m_pHeap) delete m_pHeap;
}

void CTaskHeap::Insert(PTASK_RECORD pTask, SCHCMP *pfCompare)
{
    if (m_nCurrent == m_nAllocated)
    {
        if (!Grow()) return;
    }
    pTask->m_iVisitedMark = m_iVisitedMark-1;

    m_pHeap[m_nCurrent] = pTask;
    m_nCurrent++;
    SiftUp(m_nCurrent-1, pfCompare);
}

BOOL CTaskHeap::Grow(void)
{
    // Grow the heap.
    //
    int n = m_nAllocated + EXTRA_TASKS;
    PTASK_RECORD *p = new PTASK_RECORD[n];
    if (!p) return FALSE;

    memcpy(p, m_pHeap, sizeof(PTASK_RECORD)*m_nAllocated);
    m_nAllocated = n;
    delete m_pHeap;
    m_pHeap = p;

    return TRUE;
}

PTASK_RECORD CTaskHeap::PeekAtTopmost(void)
{
    if (m_nCurrent <= 0) return NULL;
    return m_pHeap[0];
}

PTASK_RECORD CTaskHeap::RemoveTopmost(SCHCMP *pfCompare)
{
    return Remove(0, pfCompare);
}

void CTaskHeap::CancelTask(FTASK *fpTask, void *arg_voidptr, int arg_Integer)
{
    for (int i = 0; i < m_nCurrent; i++)
    {
        PTASK_RECORD p = m_pHeap[i];
        if ((p->fpTask == fpTask) && (p->arg_voidptr == arg_voidptr) && (p->arg_Integer == arg_Integer))
        {
            p->fpTask = NULL;
        }
    }
}

int ComparePriority(PTASK_RECORD pTaskA, PTASK_RECORD pTaskB)
{
    int i = (pTaskA->iPriority) - (pTaskB->iPriority);
    if (i == 0)
    {
        // Must subtract so that ticket rollover is handled properly.
        //
        return  (pTaskA->m_Ticket) - (pTaskB->m_Ticket);
    }
    return i;
}

int CompareWhen(PTASK_RECORD pTaskA, PTASK_RECORD pTaskB)
{
    // Can't simply subtract because comparing involves a truncation cast.
    //
    if (pTaskA->ltaWhen < pTaskB->ltaWhen)
    {
        return -1;
    }
    else if (pTaskA->ltaWhen > pTaskB->ltaWhen)
    {
        return 1;
    }
    return 0;
}

void CScheduler::DeferTask(const CLinearTimeAbsolute& ltaWhen, int iPriority,
                           FTASK *fpTask, void *arg_voidptr, int arg_Integer)
{
    PTASK_RECORD pTask = new TASK_RECORD;
    if (!pTask) return;

    pTask->ltaWhen = ltaWhen;
    pTask->iPriority = iPriority;
    pTask->fpTask = fpTask;
    pTask->arg_voidptr = arg_voidptr;
    pTask->arg_Integer = arg_Integer;
    pTask->m_Ticket = m_Ticket++;

    // Must add to the WhenHeap so that network is still serviced.
    //
    m_WhenHeap.Insert(pTask, CompareWhen);
}

void CScheduler::DeferImmediateTask(int iPriority, FTASK *fpTask, void *arg_voidptr, int arg_Integer)
{
    PTASK_RECORD pTask = new TASK_RECORD;
    if (!pTask) return;

    //pTask->ltaWhen = ltaWhen;
    pTask->iPriority = iPriority;
    pTask->fpTask = fpTask;
    pTask->arg_voidptr = arg_voidptr;
    pTask->arg_Integer = arg_Integer;
    pTask->m_Ticket = m_Ticket++;

    // Must add to the WhenHeap so that network is still serviced.
    //
    m_WhenHeap.Insert(pTask, CompareWhen);
}

void CScheduler::CancelTask(FTASK *fpTask, void *arg_voidptr, int arg_Integer)
{
    m_WhenHeap.CancelTask(fpTask, arg_voidptr, arg_Integer);
    m_PriorityHeap.CancelTask(fpTask, arg_voidptr, arg_Integer);
}

void CScheduler::ReadyTasks(const CLinearTimeAbsolute& ltaNow)
{
    // Move ready-to-run tasks off the WhenHeap and onto the PriorityHeap.
    //
    PTASK_RECORD pTask = m_WhenHeap.PeekAtTopmost();
    while (pTask && (pTask->ltaWhen < ltaNow))
    {
        pTask = m_WhenHeap.RemoveTopmost(CompareWhen);
        if (pTask)
        {
            if (pTask->fpTask)
            {
                m_PriorityHeap.Insert(pTask, ComparePriority);
            }
            else
            {
                delete pTask;
            }
        }
        pTask = m_WhenHeap.PeekAtTopmost();
    }
}

int CScheduler::RunTasks(const CLinearTimeAbsolute& ltaNow)
{
    ReadyTasks(ltaNow);
    if (mudconf.active_q_chunk)
    {
        return RunTasks(mudconf.active_q_chunk);
    }
    else
    {
        return RunAllTasks();
    }
}

int CScheduler::RunTasks(int iCount)
{
    int nTasks = 0;
    while (iCount--)
    {
        PTASK_RECORD pTask = m_PriorityHeap.PeekAtTopmost();
        if (!pTask) break;

        if (pTask->iPriority > m_minPriority)
        {
            // This is related to CF_DEQUEUE and also to untimed (SUSPENDED)
            // semaphore entries that we would like to manage together with
            // the timed ones.
            //
            break;
        }
        pTask = m_PriorityHeap.RemoveTopmost(ComparePriority);
        if (pTask)
        {
            if (pTask->fpTask)
            {
                pTask->fpTask(pTask->arg_voidptr, pTask->arg_Integer);
                nTasks++;
            }
            delete pTask;
        }
    }
    return nTasks;
}

int CScheduler::RunAllTasks(void)
{
    int nTotalTasks = 0;

    int nTasks;
    do
    {
        nTasks = RunTasks(100);
        nTotalTasks += nTasks;
    } while (nTasks);

    return nTotalTasks;
}

BOOL CScheduler::WhenNext(CLinearTimeAbsolute  *ltaWhen)
{
    // Check the Priority Queue first.
    //
    PTASK_RECORD pTask = m_PriorityHeap.PeekAtTopmost();
    if (pTask)
    {
        if (pTask->iPriority <= m_minPriority)
        {
            ltaWhen->SetSeconds(0);
            return TRUE;
        }
    }

    // Check the When Queue next.
    //
    pTask = m_WhenHeap.PeekAtTopmost();
    if (pTask)
    {
        *ltaWhen = pTask->ltaWhen;
        return TRUE;
    }
    return FALSE;
}

#define HEAP_LEFT_CHILD(x) (2*(x)+1)
#define HEAP_RIGHT_CHILD(x) (2*(x)+2)
#define HEAP_PARENT(x) (((x)-1)/2)

void CTaskHeap::SiftDown(int iSubRoot, SCHCMP *pfCompare)
{
    int parent = iSubRoot;
    int child = HEAP_LEFT_CHILD(parent);

    PTASK_RECORD Ref = m_pHeap[parent];

    while (child < m_nCurrent)
    {
        int rightchild = HEAP_RIGHT_CHILD(parent);
        if (rightchild < m_nCurrent)
        {
            if (pfCompare(m_pHeap[rightchild], m_pHeap[child]) < 0)
            {
                child = rightchild;
            }
        }
        if (pfCompare(Ref, m_pHeap[child]) <= 0)
            break;

        m_pHeap[parent] = m_pHeap[child];
        parent = child;
        child = HEAP_LEFT_CHILD(parent);
    }
    m_pHeap[parent] = Ref;
}

void CTaskHeap::SiftUp(int child, SCHCMP *pfCompare)
{
    while (child)
    {
        int parent = HEAP_PARENT(child);
        if (pfCompare(m_pHeap[parent], m_pHeap[child]) <= 0)
            break;

        PTASK_RECORD Tmp;
        Tmp = m_pHeap[child];
        m_pHeap[child] = m_pHeap[parent];
        m_pHeap[parent] = Tmp;

        child = parent;
    }
}

PTASK_RECORD CTaskHeap::Remove(int iNode, SCHCMP *pfCompare)
{
    if (iNode < 0 || m_nCurrent <= iNode) return NULL;

    PTASK_RECORD pTask = m_pHeap[iNode];

    m_nCurrent--;
    m_pHeap[iNode] = m_pHeap[m_nCurrent];
    SiftDown(iNode, pfCompare);
    SiftUp(iNode, pfCompare);

    return pTask;
}

void CTaskHeap::Update(int iNode, SCHCMP *pfCompare)
{
    if (iNode < 0 || m_nCurrent <= iNode)
    {
        return;
    }

    SiftDown(iNode, pfCompare);
    SiftUp(iNode, pfCompare);
}

void CScheduler::TraverseUnordered(SCHLOOK *pfLook)
{
    if (m_WhenHeap.TraverseUnordered(pfLook, CompareWhen))
    {
        m_PriorityHeap.TraverseUnordered(pfLook, ComparePriority);
    }
}

void CScheduler::TraverseOrdered(SCHLOOK *pfLook)
{
    m_PriorityHeap.TraverseOrdered(pfLook, ComparePriority);
    m_WhenHeap.TraverseOrdered(pfLook, CompareWhen);
}

// The following guarantees that in spite of any changes to the heap
// we will visit every record exactly once. It does not attempt to
// visit these records in any particular order.
//
int CTaskHeap::TraverseUnordered(SCHLOOK *pfLook, SCHCMP *pfCompare)
{
    // Indicate that everything has not been visited.
    //
    m_iVisitedMark++;
    if (m_iVisitedMark == 0)
    {
        // We rolled over. Yeah, this code will probably never run, but
        // let's go ahead and mark all the records as visited anyway.
        //
        for (int i = 0; i < m_nCurrent; i++)
        {
            PTASK_RECORD p = m_pHeap[i];
            p->m_iVisitedMark = m_iVisitedMark;
        }
        m_iVisitedMark++;
    }

    int bUnvisitedRecords;
    do
    {
        bUnvisitedRecords = FALSE;
        for (int i = 0; i < m_nCurrent; i++)
        {
            PTASK_RECORD p = m_pHeap[i];

            if (p->m_iVisitedMark == m_iVisitedMark)
            {
                // We have already seen this one.
                //
                continue;
            }
            bUnvisitedRecords = TRUE;
            p->m_iVisitedMark = m_iVisitedMark;

            int cmd = pfLook(p);
            switch (cmd)
            {
            case IU_REMOVE_TASK:
                Remove(i, pfCompare);
                break;

            case IU_DONE:
                return FALSE;

            case IU_UPDATE_TASK:
                Update(i, pfCompare);
                break;
            }
        }
    } while (bUnvisitedRecords);
    return TRUE;
}

// The following does not allow for changes during the traversal, but
// but it does visit each record in sorted order (When-order or
// Priority-order depending on the heap).
//
int CTaskHeap::TraverseOrdered(SCHLOOK *pfLook, SCHCMP *pfCompare)
{
    Sort(pfCompare);

    for (int i = m_nCurrent-1; i >= 0; i--)
    {
        PTASK_RECORD p = m_pHeap[i];
        int cmd = pfLook(p);
        if (IU_DONE == cmd)
        {
            break;
        }
    }

    Remake(pfCompare);
    return TRUE;
}

void CTaskHeap::Sort(SCHCMP *pfCompare)
{
    int s_nCurrent = m_nCurrent;
    while (m_nCurrent--)
    {
        PTASK_RECORD p = m_pHeap[m_nCurrent];
        m_pHeap[m_nCurrent] = m_pHeap[0];
        m_pHeap[0] = p;
        SiftDown(0, pfCompare);
    }
    m_nCurrent = s_nCurrent;
}

void CTaskHeap::Remake(SCHCMP *pfCompare)
{
    int s_nCurrent = m_nCurrent;
    m_nCurrent = 0;
    while (s_nCurrent--)
    {
        m_nCurrent++;
        SiftUp(m_nCurrent-1, pfCompare);
    }
}

void CScheduler::SetMinPriority(int arg_minPriority)
{
    m_minPriority = arg_minPriority;
}