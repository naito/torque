/*
*         OpenPBS (Portable Batch System) v2.3 Software License
*
* Copyright (c) 1999-2000 Veridian Information Solutions, Inc.
* All rights reserved.
*
* ---------------------------------------------------------------------------
* For a license to use or redistribute the OpenPBS software under conditions
* other than those described below, or to purchase support for this software,
* please contact Veridian Systems, PBS Products Department ("Licensor") at:
*
*    www.OpenPBS.org  +1 650 967-4675                  sales@OpenPBS.org
*                        877 902-4PBS (US toll-free)
* ---------------------------------------------------------------------------
*
* This license covers use of the OpenPBS v2.3 software (the "Software") at
* your site or location, and, for certain users, redistribution of the
* Software to other sites and locations.  Use and redistribution of
* OpenPBS v2.3 in source and binary forms, with or without modification,
* are permitted provided that all of the following conditions are met.
* After December 31, 2001, only conditions 3-6 must be met:
*
* 1. Commercial and/or non-commercial use of the Software is permitted
*    provided a current software registration is on file at www.OpenPBS.org.
*    If use of this software contributes to a publication, product, or
*    service, proper attribution must be given; see www.OpenPBS.org/credit.html
*
* 2. Redistribution in any form is only permitted for non-commercial,
*    non-profit purposes.  There can be no charge for the Software or any
*    software incorporating the Software.  Further, there can be no
*    expectation of revenue generated as a consequence of redistributing
*    the Software.
*
* 3. Any Redistribution of source code must retain the above copyright notice
*    and the acknowledgment contained in paragraph 6, this list of conditions
*    and the disclaimer contained in paragraph 7.
*
* 4. Any Redistribution in binary form must reproduce the above copyright
*    notice and the acknowledgment contained in paragraph 6, this list of
*    conditions and the disclaimer contained in paragraph 7 in the
*    documentation and/or other materials provided with the distribution.
*
* 5. Redistributions in any form must be accompanied by information on how to
*    obtain complete source code for the OpenPBS software and any
*    modifications and/or additions to the OpenPBS software.  The source code
*    must either be included in the distribution or be available for no more
*    than the cost of distribution plus a nominal fee, and all modifications
*    and additions to the Software must be freely redistributable by any party
*    (including Licensor) without restriction.
*
* 6. All advertising materials mentioning features or use of the Software must
*    display the following acknowledgment:
*
*     "This product includes software developed by NASA Ames Research Center,
*     Lawrence Livermore National Laboratory, and Veridian Information
*     Solutions, Inc.
*     Visit www.OpenPBS.org for OpenPBS software support,
*     products, and information."
*
* 7. DISCLAIMER OF WARRANTY
*
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT
* ARE EXPRESSLY DISCLAIMED.
*
* IN NO EVENT SHALL VERIDIAN CORPORATION, ITS AFFILIATED COMPANIES, OR THE
* U.S. GOVERNMENT OR ANY OF ITS AGENCIES BE LIABLE FOR ANY DIRECT OR INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This license will be governed by the laws of the Commonwealth of Virginia,
* without reference to its choice of law rules.
*/

/*
 * @file svr_task.c
 *
 * Contains functions to deal with the server's task list
 *
 * A task list is a set of pending functions usually associated with
 * processing a reply message.
 *
 * Functions included are:
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include "portability.h"
#include <stdlib.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include "server_limits.h"
#include "list_link.h"
#include "work_task.h"
#include "utils.h"
#include "threadpool.h"
#include "../lib/Liblog/pbs_log.h"


extern void check_nodes(struct work_task *);

/* Global Data Items: */

extern all_tasks task_list_timed;
extern all_tasks task_list_event;




/*
 * set_task - add the job entry to the task list
 *
 * Task time depends on the type of task.  The list is time ordered.
 */

struct work_task *set_task(

  enum work_type   type,
  long             event_id,  /* I - based on type can be time of event */
  void           (*func)(),
  void            *parm,
  int              get_lock)

  {
  work_task *pnew;
  work_task *pold;
  int        iter = -1;

  pnew = (struct work_task *)malloc(sizeof(struct work_task));

  if (pnew == NULL)
    {
    return(NULL);
    }

  memset(pnew,0,sizeof(work_task));

  pnew->wt_event    = event_id;
  pnew->wt_type     = type;
  pnew->wt_func     = func;
  pnew->wt_parm1    = parm;

  if (type == WORK_Immed)
    {
    enqueue_threadpool_request((void *(*)(void *))func,pnew);
    }
  else
    {
    pnew->wt_mutex = malloc(sizeof(pthread_mutex_t));
    
    if (pnew->wt_mutex == NULL)
      {
      free(pnew);
      return(NULL);
      }
    
    pthread_mutex_init(pnew->wt_mutex,NULL);

    /* only acquire the lock if they want it */
    if (get_lock == TRUE)
      pthread_mutex_lock(pnew->wt_mutex);
   
    if (type == WORK_Timed)
      {
      while ((pold = next_task(&task_list_timed,&iter)) != NULL)
        {
        if (pold->wt_event > pnew->wt_event)
          break;
        
        pthread_mutex_unlock(pold->wt_mutex);
        }
      
      if (pold != NULL)
        {
        insert_task_before(&task_list_timed,pnew,pold);
        
        pthread_mutex_unlock(pold->wt_mutex);
        }
      else
        {
        insert_task(&task_list_timed,pnew,FALSE);
        }
      
      }
    else
      {
      insert_task(&task_list_event,pnew,FALSE);
      }
    }

  return(pnew);
  }  /* END set_task() */




/* 
 * checks if the task has been queued into a threadpool
 * right now, check_nodes is the only one done that way, but all
 * future ones should be added here
 */

int task_is_in_threadpool(

  struct work_task *ptask)

  {
  if (ptask->wt_func == check_nodes)
    return(TRUE);

  return(FALSE);
  } /* END task_is_in_threadpool() */





/*
 * dispatch_task - dispatch a work task found on a work list
 *
 * delinks the work task entry, calls the associated function with
 * the parameters from the work task entry, and then frees the entry.
 */

void dispatch_task(

  struct work_task *ptask) /* M */

  {
  if (ptask->wt_tasklist)
    remove_task(ptask->wt_tasklist,ptask);

  if (ptask->wt_obj_tasklist)
    remove_task(ptask->wt_obj_tasklist,ptask);

  /* unlock and free the mutex */
  pthread_mutex_unlock(ptask->wt_mutex);
  free(ptask->wt_mutex);

  if (ptask->wt_func != NULL)
    enqueue_threadpool_request((void *(*)(void *))ptask->wt_func,ptask);

  return;
  }  /* END dispatch_task() */





/*
 * delete_task - unlink and free a work_task structure.
 */

void delete_task(
    
  struct work_task *ptask) /* M */

  {
  if (ptask->wt_tasklist)
    remove_task(ptask->wt_tasklist,ptask);

  if (ptask->wt_obj_tasklist)
    remove_task(ptask->wt_obj_tasklist,ptask);

  pthread_mutex_unlock(ptask->wt_mutex);
  free(ptask->wt_mutex);

  (void)free(ptask);

  } /* END delete_task() */




/* 
 * initialize_all_tasks_array
 *
 * initializes the all_tasks object
 */

void initialize_all_tasks_array(

  all_tasks *at) /* O */

  {
  static char         *id = "initialize_all_tasks_array";

  at->ra = initialize_resizable_array(INITIAL_ALL_TASKS_SIZE);

  if (at->ra == NULL)
    {
    log_err(ENOMEM,id,"Cannot allocate space for array...FAILURE");
    }
  
  at->alltasks_mutex = malloc(sizeof(pthread_mutex_t));

  if (at->alltasks_mutex == NULL)
    {
    log_err(ENOMEM,id,"Cannot allocate space for mutex...FAILURE");
    }
  else
    {
    pthread_mutex_init(at->alltasks_mutex,NULL);
    }
  } /* END initialize_all_tasks_array() */




/* 
 * return the next task in this array using iter
 */
work_task *next_task(

  all_tasks *at,
  int       *iter)

  {
  work_task *wt;

  pthread_mutex_lock(at->alltasks_mutex);

  wt = next_thing(at->ra,iter);

  pthread_mutex_unlock(at->alltasks_mutex);

  if (wt != NULL)
    pthread_mutex_lock(wt->wt_mutex);

  return(wt);
  } /* END next_task() */




/*
 * adds a task to the specified array
 */

int insert_task(

  all_tasks *at,
  work_task *wt,
  int        object)

  {
  static char *id = "insert_task";
  int rc;

  pthread_mutex_lock(at->alltasks_mutex);

  if ((rc = insert_thing(at->ra,wt)) == -1)
    {
    rc = ENOMEM;
    log_err(rc,id,"Cannot allocate space to resize the array");
    }

  pthread_mutex_unlock(at->alltasks_mutex);

  if (object == TRUE)
    wt->wt_obj_tasklist = at;
  else
    wt->wt_tasklist = at;

  return(rc);
  } /* END insert_task() */




/*
 * adds a task to the array after another
 */
int insert_task_before(

  all_tasks *at,
  work_task *before,
  work_task *after)

  {
  static char *id = "insert_task_after";
  int          rc;
  int          i;

  pthread_mutex_lock(at->alltasks_mutex);

  i = get_index(at->ra,after);

  if (i == THING_NOT_FOUND)
    {
    rc = i;
    }
  else
    {
    if ((rc = insert_thing_before(at->ra,before,i)) == -1)
      {
      rc = ENOMEM;
      log_err(rc,id,"Cannot allocate space to resize the array");
      }
    }

  pthread_mutex_unlock(at->alltasks_mutex);

  before->wt_tasklist = at;

  return(rc);
  } /* insert_task_after() */





int insert_task_first(
    
  all_tasks *at,
  work_task *wt)

  {
  static char *id = "insert_task_first";
  int          rc;

  pthread_mutex_lock(at->alltasks_mutex);

  if ((rc = insert_thing_after(at->ra,wt,ALWAYS_EMPTY_INDEX)) == -1)
    {
    rc = ENOMEM;
    log_err(rc,id,"Cannot allocate space to resize the array");
    }

  pthread_mutex_unlock(at->alltasks_mutex);
  
  wt->wt_tasklist = at;

  return(rc);
  } /* END insert_task_first() */




/*
 * checks if this all_tasks array has any tasks 
 *
 * @param at - the all_tasks array to check
 * @return TRUE is there are any tasks, FALSE otherwise
 */

int has_task(

  all_tasks *at) /* I */

  {
  int rc = FALSE;

  pthread_mutex_lock(at->alltasks_mutex);

  if (at->ra->num > 0)
    rc = TRUE;

  pthread_mutex_unlock(at->alltasks_mutex);

  return(rc);
  } /* END has_task() */




/*
 * removes a specific tasks from this array
 */
int remove_task(

  all_tasks *at,
  work_task *wt)

  {
  int rc;

  if (pthread_mutex_trylock(at->alltasks_mutex))
    {
    pthread_mutex_unlock(wt->wt_mutex);
    pthread_mutex_lock(at->alltasks_mutex);
    pthread_mutex_lock(wt->wt_mutex);
    }

  rc = remove_thing(at->ra,wt);

  pthread_mutex_unlock(at->alltasks_mutex);

  return(rc);
  } /* END remove_task() */




