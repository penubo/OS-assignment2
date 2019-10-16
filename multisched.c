#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)

/** constraints **/
#define COMMAND_LEN 256						// maximum command length

#define ID_LEN 2									// id string length limit
#define TIME_LEN 2								// time string length limit
#define PRIORITY_LEN 2						// priority string length limit

#define MAX_PRIORITY 10						// priority limit
#define MAX_ARRIVE_TIME 30				// arrive time limit
#define MAX_SERVICE_TIME 30				// service time limit
#define MAX_PROCESS_ID 26*2*10		// all process id limit
#define MIN_ARRIVE_TIME 0
#define MIN_SERVICE_TIME 1
#define MIN_PRIORITY 1

#define H_TIME_QUANTUM 6					// time quantum of H tasks
#define M_TIME_QUANTUM 4					// time quantum of M tasks

#define DEBUG 0


/** declarations **/

/* type declarations */
typedef struct _Task Task;					
typedef struct _Queue Queue;
typedef struct _CPU CPU;
typedef struct _GanttNode Node;
typedef struct _GanttList GanttList;
typedef enum _Type Type;				

/* parser related function declarations */
static int check_valid_id(const char *);
static Task *lookup_task(const char *);
static int check_valid_arrive_time(const char *);
static int check_valid_service_time(const char *);
static int check_valid_priority(const char *);
static void append_task(Task *);

/* queue related function declarations */
static Queue *get_queue(Type);
static void enqueue_task(Task *);
static Task *dequeue_task(Queue *);
static bool is_empty(Queue *);
static void init_queue(Queue *);

/* scheduling algorithm related function declarations */
static void long_term_schedule();
static void process();
static void short_term_schedule();
static void priority_interrupt_check();
static void timeout_check();

/* gantt related function declarations */
static void add_gantt_node(Task *);
static void record_to_gantt(char *);
static void print_gantt();

/* global variables declarations */
static Task *tasks;           // list of tasks from the txt file.
static int time;              // track current time.
static CPU *cpu;              // cpu
static bool volatile running; // running flag
static GanttList gantt_list;  // linked list of gantt node

/* queue pointers */
static Queue *H_queue;
static Queue *M_queue;
static Queue *L_queue;


/** struct & enum definitions **/

/* task Type */
enum _Type {

  H, M, L
};

/* task structure */
struct _Task {

  Task *next;                 // Pointer which point the next Task.

  Type type;                  // Type of the Task. H, M, L
  char id[ID_LEN+1];          // Id of the Task.
  int arrive_time;            // Arrive-time of the Task.
  int service_time;           // Service-time of the Task. 
  int priority;               // Priority of the Task.
  int remaining_time;         // Remaining time to service this Task.
  int complete_time;          // Complete-time of the Task.
};

/* queue structure */
struct _Queue {

  Task *head;                 // head or front pointer
  Task *tail;                 // tail or rear pointer
};

/* cpu structure */
struct _CPU {             

  Task *task;               // task currently running
  Type task_type;           // task type for recording which task was running
  int timeout;              // timeout value for H and M
};

/* gantt node */
struct _GanttNode {

  Node *next;               // pointer which points the next gantt node
  Task *task;               // each gantt node keep one task

  char id[ID_LEN+1];        // id of task
  bool record[MAX_PROCESS_ID*MAX_SERVICE_TIME+MAX_SERVICE_TIME]; // record of execution of its task
  int turn_around_time;     // complete time - arrive time
  int waiting_time;         // turnaround time - arrive time
};

/* gantt list */
struct _GanttList {

  Node *head;               // head of a list
  
};


/** function definitions **/

/* make gantt node with a task */
static void add_gantt_node(Task *task) {

  Node *new_node;
	Node *n;

  n = gantt_list.head;
	new_node = (Node *) malloc(sizeof(Node));
  strcpy(new_node->id, task->id);
  new_node->task = task;

  if (n == NULL) {								// if gantt node list is empty
    gantt_list.head = new_node;
    return;
  }
  while (n->next != NULL) n = n->next;
  n->next = new_node;
}

/* record executed task information to corresponding gantt node */
static void record_to_gantt(char *id) {

  if (gantt_list.head == NULL) return;

  for (Node *n = gantt_list.head; n != NULL; n = n->next) {
    if (!strcmp(n->id, id)) {
      n->record[time] = true; 		// record that the task was executed at time
      break;
    }
  }

}

/* print gantt chart */
static void print_gantt() {

  Node *n = gantt_list.head;

  if (n == NULL) return;
  
  while (n != NULL) {
    printf("%s ", n->id);
    for (int i = 0; i < 60; i++) {
      if (n->record[i] == true) {
        printf("*");
      } else {
        printf(" ");
      }
    }
    printf("\n");
    n = n->next;
  }
}

/* check id is valid */
static int check_valid_id(const char *str) {

  size_t len;						

  len = strlen (str);
  if (len != ID_LEN)														// if ID length is invalid
    return -1;

  if (!(isupper(str[0]) && isdigit(str[1])))    // if ID is over ranged
    return -1;

  return 0;
}

/* look up task is existed */
static Task *lookup_task (const char *id) {

  Task *task;

  for (task = tasks; task != NULL; task = task->next)
    if (!strcmp (task->id, id))
      return task;

  return NULL;
}

/* check arrive time is valid */
static int check_valid_arrive_time(const char *str) {

  size_t len;
  int val;

  len = strlen(str);
  if (len > TIME_LEN)											// if time length is invalid
    return -1;

  for (int i = 0; i < len; i++) {
    if (!isdigit(str[i])) 								// if it is not digit values
      return -1;
  }

  val = atoi(str);
  if (val < MIN_ARRIVE_TIME || val > MAX_ARRIVE_TIME)			// if it is over ranged
    return -1;

  return 0;
}

/* check service time is valid */
static int check_valid_service_time(const char *str) {

  size_t len;
  int val;

  len = strlen(str);
  if (len > TIME_LEN)											// if its length is invalid
    return -1;

  for (int i = 0; i < len; i++) {
    if (!isdigit(str[i]))									// if it is not digit value
      return -1;
  }

  val = atoi(str);
  if (val < MIN_SERVICE_TIME || val > MAX_SERVICE_TIME)		// if it is over ranged
    return -1;

  return 0;
}

/* check priority is valid */
static int check_valid_priority(const char *str) {

  size_t len;
  int val;

  len = strlen(str);
  if (len > PRIORITY_LEN)									// if its length is invalid
    return -1;

  for (int i = 0; i < len; i++) {					// if it is not digit value
    if (!isdigit(str[i]))
      return -1;
  }

  val = atoi(str);
  if (val < MIN_PRIORITY || val > MAX_PRIORITY)   // if it is over ranged
    return -1;

  return 0;
}

static char * strstrip (char *str) {

  char  *start;
  size_t len;

  len = strlen (str);
  while (len--)
  {
    if (!isspace (str[len]))
      break;
    str[len] = '\0';
  }

  for (start = str; *start && isspace (*start); start++);
  memmove (str, start, strlen (start) + 1);

  return str;
}

/* append task to tasks list */
static void append_task(Task *task) {

  Task *new_task;

  new_task = (Task *) calloc(1, sizeof(Task));

  if (!new_task) {
    MSG ("failed to allocate a task: %s\n", STRERROR);
    return;
  }

  *new_task = *task;
  new_task->next = NULL;

  if (!tasks) {
    tasks = new_task;
  } else {

    Task *t = tasks;

    while(t->next != NULL) t = t->next;
    new_task->next = t->next;
    t->next = new_task;

  }

  if(DEBUG) MSG("new task %d\n", new_task->priority);

  add_gantt_node(new_task);

}

/* parsing data file */
static int read_config(const char* filename) {

  FILE *fp;
  char line[COMMAND_LEN * 2];
  int line_nr = 0;

  fp = fopen (filename, "r");
  if (!fp)
    return -1;

  tasks = NULL;

  while (fgets(line, sizeof(line), fp)) {
    Task task;
    char* p;
    char* s;
    size_t len;

    memset(&task, 0x00, sizeof(task));
    line_nr++;

    len = strlen (line);
    if (line[len - 1] == '\n')
      line[len - 1] = '\0';

    /* comment or empty line */
    if (line[0] == '#' || line[0] == '\0')
      continue;

    /* id */
    s = line;
    p = strchr (s, ' ');
    if (!p)
      goto invalid_line;
    *p = '\0';
    strstrip (s);
    if (check_valid_id (s))
    {
      MSG ("invalid id '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }
    if (lookup_task (s))
    {
      MSG ("duplicate id '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }

    strcpy(task.id, s);

    /* process-type */
    s = p + 1;
    p = strchr (s, ' ');
    if (!p)
      goto invalid_line;
    *p = '\0';
    strstrip (s);

    if (!strcasecmp (s, "H")) {
      task.type = H;
    }
    else if (!strcasecmp (s, "M")) {
      task.type = M;
    }
    else if (!strcasecmp (s, "L")) {
      task.type = L;
    }
    else
    {
      MSG ("invalid action '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }

    /* arrive-time */
    s = p + 1;
    p = strchr (s, ' ');
    if (!p)
      goto invalid_line;
    *p = '\0';
    strstrip (s);
    if (check_valid_arrive_time(s)) {
      MSG ("invalid arrive_time '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }

    task.arrive_time = atoi(s);

    /* service-time */
    s = p + 1;
    p = strchr (s, ' ');
    if (!p)
      goto invalid_line;
    *p = '\0';
    strstrip (s);
    if (check_valid_service_time(s)) {
      MSG ("invalid service_time '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }

    task.service_time = atoi(s);
    task.remaining_time = task.service_time;

    /* priority */
    s = p + 1;
    strstrip(s);
    if (s[0] == '\0')
    {
      MSG ("empty priority in line %d, ignored\n", line_nr);
      continue;
    }
    if (check_valid_priority(s)) {
      MSG ("invalid priority '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }

    task.priority = atoi(s);
  
    if (DEBUG)
      MSG ("id:%s type:%d arrive-time:%d service-time:%d priority:%d\n",
          task.id, task.type, task.arrive_time, task.service_time, task.priority);

		/* append task */
    append_task(&task);

    continue;

invalid_line:
    MSG ("invalid format in line %d, ignored\n", line_nr);
  }

  fclose (fp);

  return 0;

}

/* get queue */
static Queue *get_queue(Type type) {
  switch(type) {
    case H:
      return H_queue;
    case M:
      return M_queue;
    case L:
      return L_queue;
    default:
      MSG("get_enqueue error no such queue type\n");
      return NULL;
  }
}

/* enqueue task to corresponding queue */
static void enqueue_task(Task *new_task) {

  Task *t;
	Type task_type;
	Queue *q;

  if (!new_task) {
    MSG("enqueue_task error no task is given\n");
    return ;
  }

  task_type = new_task->type;
  q = get_queue(task_type);					// get queue from task's type
  t = q->head;

  if (is_empty(q)) {								// when queue is empty
    q->head = q->tail = new_task;
  } else if (task_type == H) {

		// enqueue by its priority
    if (new_task->priority < q->head->priority) { 		
      new_task->next = q->head;
      q->head = new_task;
    } else { 
      while (t->next != NULL 
          && new_task->priority >= t->next->priority) t = t->next;

			new_task->next = t->next;
			t->next = new_task;
    }

  } else if (task_type == M) {

		// enqueue by its remaining time
    if (new_task->remaining_time < q->head->remaining_time) {  
      new_task->next = q->head;
      q->head = new_task;
    } else {
      while (t->next != NULL 
             && new_task->remaining_time >= t->next->remaining_time) t = t->next;

				new_task->next = t->next;
				t->next = new_task;
    }

  } else if (task_type == L) {

		// FIFO implementation
    q->tail->next = new_task;
    q->tail = new_task;
  }
}

/* dequeue a task from given queue */
static Task *dequeue_task(Queue *q) {

  Task *t;

  if (is_empty(q)) {
    MSG ("no element to dequeue\n");
    return NULL;
  }

	t = q->head;

  if (q->head == q->tail) {
    q->head = NULL;
    q->tail = NULL;
  } else {

    q->head = q->head->next;
    t->next = NULL;
  }

  return t;

}

/* check whether queue is empty */
static bool is_empty(Queue *q) {
  return (q->tail == NULL);
}

/* init queue */
static void init_queue(Queue *q) {
  q->head = NULL;
  q->tail = NULL;
}

/* long-term-scheduling function */
static void long_term_schedule() {

  Task *t;
  Task *target;

  if (!tasks) return;               // no task to schedule

  t = tasks;
  while (t) {
    if (t->arrive_time <= time) {   // schedule the task which 
      target = t;                   // arrive-time is equal to current time
      tasks = t->next;
      t = t->next;
      target->next = NULL;
      enqueue_task(target);
    } else {
      t = t->next;
    }
  }
}

/* process task in cpu */
static void process() {

  if (cpu->task != NULL) {

    record_to_gantt(cpu->task->id);						// record to gantt node
    cpu->task->remaining_time--;							// update remaining time of the task

    if (cpu->task->remaining_time == 0) {			// when task is done

      if (DEBUG) MSG ("task %s is done\n", cpu->task->id);

      cpu->task->complete_time = time + 1;		// record complete time
      cpu->task = NULL;												// we add 1 because time is not ticking yet
    }
    cpu->timeout--;			// update timeout value
  }
}

/* short-term-scheduling function */
static void short_term_schedule() {

  Task *t;

  if (cpu->task != NULL) return;

  if (cpu->task_type == H) { 						// it's time for H task
    if (!is_empty(H_queue)) {						// if there is H task to be able to run
      t = dequeue_task(H_queue);
      cpu->task = t;
    } else if (!is_empty(M_queue)) { 		// if there is M task to be able to run and 
      t = dequeue_task(M_queue);				// no H task to be able to run 
      cpu->task = t;
      cpu->task_type = M;								// reset time quantum to M
      cpu->timeout = M_TIME_QUANTUM;
    } else if (!is_empty(L_queue)) {		// if no H, M task to be able to run
      t = dequeue_task(L_queue);
      cpu->task = t;
      cpu->task_type = L;
      cpu->timeout = -1;								// no timeout for L task
    }

  } else if (cpu->task_type == M) { 		// it's time for M task 
    if (!is_empty(M_queue)) {						// if there is M task to be able to run
      t = dequeue_task(M_queue);
      cpu->task = t;
    } else if (!is_empty(H_queue)) {		// if there is H task to be able to run and
      t = dequeue_task(H_queue);				// no M task to be able to run
      cpu->task = t;
      cpu->task_type = H;								// reset time quantum to H
      cpu->timeout = H_TIME_QUANTUM;
    } else if (!is_empty(L_queue)) {		// if no H, M task to be able to run
      t = dequeue_task(L_queue);
      cpu->task = t;
      cpu->task_type = L;
      cpu->timeout = -1;								// no time out for L task
    }

  } else { 															// if L was running before or no task was run yet
    if (!is_empty(H_queue)) {
      t = dequeue_task(H_queue);				// if there is H task to be able to run
      cpu->task = t;
      cpu->task_type = H;								// reset time quantum
      cpu->timeout = H_TIME_QUANTUM;
    } else if (!is_empty(M_queue)) {		// if there is M task to be able to run
      t = dequeue_task(M_queue);
      cpu->task = t;
      cpu->task_type = M;								// reset time quantum
      cpu->timeout = M_TIME_QUANTUM;
    } else if (!is_empty(L_queue)) {		// if no H, M task to be able to run
      t = dequeue_task(L_queue);
      cpu->task = t;
      cpu->task_type = L;
      cpu->timeout = -1;
    } else {}
  }
}

/* handle priority interrupt if there is */
static void priority_interrupt_check() {

  if (cpu->task == NULL) return;

  // if M is running then no preemption occur

	// when H is running and more higher priority appear then interrupt
  if (cpu->task_type == H) 
  {
    if (!is_empty(H_queue)) {
      if (H_queue->head->priority < cpu->task->priority) {
        Task *preempted_task = cpu->task;
        Task *new_task = dequeue_task(H_queue);
        cpu->task = new_task;
        enqueue_task(preempted_task);
      }
    }
  } 
	// when L is running and either H, M tasks appear then interrupt
  else if (cpu->task_type == L) 
  {
    if (!is_empty(H_queue)) {
      Task *preempted_task = cpu->task;
      Task *new_task = dequeue_task(H_queue);
      cpu->task = new_task;
      // update time quantum to H
      cpu->timeout = H_TIME_QUANTUM;
      cpu->task_type = H;
      // preempted L task must handle first later
      preempted_task->next = L_queue->head->next;
      L_queue->head = preempted_task;
    } else if (!is_empty(M_queue)) {
      Task *preempted_task = cpu->task;
      Task *new_task = dequeue_task(M_queue);
      cpu->task = new_task;
      // update time quantum to M
      cpu->timeout = M_TIME_QUANTUM;
      cpu->task_type = M;
      // preempted L task must handle first later
      preempted_task->next = L_queue->head->next;
      L_queue->head = preempted_task;
    }
  }
}

/* handle timeout if there is */
static void timeout_check() {

	Task *preempted_task;

  if (cpu->task == NULL) return;

	// switcing H to M task
  if (cpu->task_type == H && cpu->timeout == 0) {

    cpu->task_type = M;
    cpu->timeout = M_TIME_QUANTUM;
    // remove task
    preempted_task = cpu->task;
    enqueue_task(preempted_task);
    cpu->task = NULL;

	// switching M to H
  } else if (cpu->task_type == M && cpu->timeout == 0) {

    cpu->task_type = H;
    cpu->timeout = H_TIME_QUANTUM;
    // remove task
    preempted_task = cpu->task;
    enqueue_task(preempted_task);
    cpu->task = NULL;
  }

  return;
}

/* calculate average turnaround time */
static double get_average_turn_around_time() {

  int size = 0;
  int total_turn_around_time = 0;

  if (gantt_list.head == NULL) return 0.0;

  for (Node *n = gantt_list.head; n != NULL; n = n->next) {
		// turnaround time = complete time - arrive time
    n->turn_around_time = n->task->complete_time - n->task->arrive_time;
    total_turn_around_time += n->turn_around_time;
    size++;
    if (DEBUG) MSG("tat %s, %d\n", n->id, n->turn_around_time);
  }

  return ((double) total_turn_around_time) / size; 
}

/* calculate average waiting time */
static double get_average_waiting_time() {

  int size = 0;
  int total_waiting_time = 0;

  for (Node *n = gantt_list.head; n != NULL; n = n->next) {
		// waiting time = turnaround time - arrive time
    n->waiting_time = n->turn_around_time - n->task->service_time;
    total_waiting_time += n->waiting_time;
    size++;
  }

  return ((double) total_waiting_time) / size;
}

int main(int argc, char **argv) {

  if (argc <= 1)
  {
    MSG ("usage: %s input file must specified\n", argv[0]);
    return -1; 
  }

  if (read_config (argv[1]))
  {
    MSG ("failed to load input file '%s': %s\n", argv[1], STRERROR);
    return -1; 
  }

  /* initialize all queues */
  H_queue = (Queue *) malloc(sizeof(Queue));
  M_queue = (Queue *) malloc(sizeof(Queue));
  L_queue = (Queue *) malloc(sizeof(Queue));
  init_queue(H_queue);
  init_queue(M_queue);
  init_queue(L_queue);

  /* initialize CPU */
  cpu = (CPU *) malloc(sizeof(CPU));
  cpu->timeout = -1;
  cpu->task_type = L;
  cpu->task = NULL;


  /* init time and running flag */
  time = 0;
  running = true;

  while (running) {

    /* long-term scheduling */
    long_term_schedule();

    /* short_term_scheduling */
    if (cpu->task == NULL) {
      short_term_schedule();
    } else {
      // handle interrupt here
      priority_interrupt_check();
    }

    /* process a task in CPU */
    if (DEBUG) {
      if (cpu->task == NULL) {
        MSG("cpu is empty\n");
      } else {
        MSG("cpu %s \n", cpu->task->id);
      }
    }

    /* process a task */
    process();

    /* time out check */
    timeout_check();

    /* increase time */
    time++;           

    /* check all tasks done */
    if (!tasks && is_empty(H_queue) && is_empty(M_queue) && is_empty(L_queue) && cpu->task == NULL) {
      running = false;
    }
  }

	/* print result */
  printf("\n[Multilevel Queue Scheduling]\n");
  print_gantt();
  printf("\nCPU TIME: %d\n", time);
  printf("AVERAGE TURNAROUND TIME: %.2f\n", get_average_turn_around_time());
  printf("AVERAGE WAITING TIME: %.2f\n", get_average_waiting_time());

  return 0;

}





