#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)
#define COMMAND_LEN 256

#define ID_LEN 2
#define TIME_LEN 2
#define PRIORITY_LEN 2

#define H_CPU_TIME_SLICE 6
#define M_CPU_TIME_SLICE 4

#define DEBUG 1     // for debugging

/* declarations */
typedef struct _Task Task;
typedef struct _Queue Queue;
typedef enum _Type Type;

static int check_valid_id(const char *);
static Task *lookup_task(const char *);
static int check_valid_arrive_time(const char *);
static int check_valid_service_time(const char *);
static int check_valid_priority(const char *);

static void append_task(Task *);

static Queue *get_queue(Type);
static int enqueue_task(Task *);
static Task *dequeue_task(Queue *);
static bool is_empty(Queue *);
static void init_queue(Queue *);

static void long_term_schedule();


/* global variables */
static Task *tasks;           // list of tasks from the txt file.
static int time;              // track current time.
static int H_time;            // track remaining time can be used for H tasks (time slicing)
static int M_time;            // track remaining time can be used for M tasks (time slicing)

/* queues */
/* 
 * input_queue: it stores tasks from the txt file given in order of arrive_time. 
 *              later each task in it moves to a appropriate queue when time is 
 *              arrive_time. And it's FCFS queue by its nature.
 * H_queue    : it's priority queue. 
 * M_queue    : it's SJF queue.
 * L_queue    : it's FCFS queue.
 *
 */
static Queue input_queue;       
static Queue H_queue;
static Queue M_queue;
static Queue L_queue;




enum _Type {

  H, M, L

};

struct _Task {

  Task *next;                 // Pointer which point the next Task.

  Type type;                  // Type of the Task. H, M, L
  char id[ID_LEN+1];    // Id of the Task.
  int arrive_time;      // Arrive-time of the Task.
  int service_time;     // Service-time of the Task. 
  int priority;         // Priority of the Task.
  int remaining_time;    // Remaining time to service this Task.
  int complete_time;          // Complete-time of the Task.
};

struct _Queue {

  Task *head;
  Task *tail;
  int size;
};


// DEBUG
static void print_tasks() {

  Task *t = tasks;

  if(!tasks) {
    MSG("no tasks to print\n");
  }

  while (t != NULL) {
    MSG ("%s -> ", t->id);
    t = t->next;
  }
  MSG("\n");
}

// DEBUG
static void print_queue(Queue *q) {
  Task *t = q->head;
  if (t == NULL) {
    MSG("queue is EMPTY\n");
  } else {
    MSG("%d queue : ", q->head->type);
    while (t != NULL) {
      MSG("%s -> ", t->id);
      t = t->next;
    }
    MSG("\n");
  }
}

static int check_valid_id(const char *str) {

  size_t len;
  int    i;

  len = strlen (str);
  if (len != ID_LEN)
    return -1;

  if (!(isupper(str[0]) && isdigit(str[1]))) 
    return -1;

  return 0;
}


static Task *lookup_task (const char *id)
{
  Task *task;

  for (task = tasks; task != NULL; task = task->next)
    if (!strcmp (task->id, id))
      return task;

  return NULL;
}

static int check_valid_arrive_time(const char *str) {

  size_t len;
  int val;

  len = strlen(str);
  if (len > TIME_LEN)
    return -1;

  for (int i = 0; i < len; i++) {
    if (!isdigit(str[i]))
      return -1;
  }

  val = atoi(str);
  if (val < 0 || val > 30)
    return -1;

  return 0;
}
static int check_valid_service_time(const char *str) {

  size_t len;
  int val;

  len = strlen(str);
  if (len > TIME_LEN)
    return -1;

  for (int i = 0; i < len; i++) {
    if (!isdigit(str[i]))
      return -1;
  }

  val = atoi(str);
  if (val < 1 || val > 30)
    return -1;

  return 0;
}
static int check_valid_priority(const char *str) {

  size_t len;
  int val;

  len = strlen(str);
  if (len > PRIORITY_LEN)
    return -1;

  for (int i = 0; i < len; i++) {
    if (!isdigit(str[i]))
      return -1;
  }

  val = atoi(str);
  if (val < 1 || val > 10)
    return -1;

  return 0;
}
static char *
strstrip (char *str)
{
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

static void append_task(Task *task) {

  Task *new_task;

  new_task = (Task *) malloc(sizeof(task));

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

}

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

    //TODO: append whole information here
    if (DEBUG)
      MSG ("id:%s type:%d arrive-time:%d service-time:%d priority:%d\n",
          task.id, task.type, task.arrive_time, task.service_time, task.priority);

    append_task(&task);

    continue;

invalid_line:
    MSG ("invalid format in line %d, ignored\n", line_nr);
  }

  fclose (fp);


  return 0;

}

static Queue *get_queue(Type type) {
  switch(type) {
    case H:
      return &H_queue;
    case M:
      return &M_queue;
    case L:
      return &L_queue;
    default:
      MSG("get_enqueue error no such queue type\n");
      return NULL;
  }
}

static int enqueue_task(Task *new_task) {

  //Task *t = q->head;
  Task *t;

  if (!new_task) {
    MSG("enqueue_task error no task is given\n");
    return -1;
  }

  Type task_type = new_task->type;
  Queue *q = get_queue(task_type);

  //Task *new_task; 

  //TODO: do not need to alloc here. because I want to use this after alloc all tasks.
  //new_task = (Task *) malloc(sizeof(Task));

  //*new_task = *task;

  if (is_empty(q)) {
    q->head = q->tail = new_task;
  } else if (task_type == H) {

    if (new_task->priority < q->head->priority) {
      new_task->next = q->head;
      q->head = new_task;
    } else {
      while (t->next != NULL 
             && new_task->priority > t->next->priority) 
        t = t->next;
        if (t->next == NULL) {
          t->next = new_task;
          q->tail = new_task;
        } else {
          new_task->next = t->next;
          t->next = new_task;
        }
    }

  } else if (task_type == M) {

    if (new_task->remaining_time < q->head->remaining_time) {
      new_task->next = q->head;
      q->head = new_task;
    } else {
      while (t->next != NULL 
             && new_task->remaining_time > t->next->remaining_time) 
        t = t->next;
        if (t->next == NULL) {
          t->next = new_task;
          q->tail = new_task;
        } else {
          new_task->next = t->next;
          t->next = new_task;
        }
    }

  } else if (task_type == L) {

    q->tail->next = new_task;
    q->tail = new_task;
  }

  q->size++;
}

static Task *dequeue_task(Queue *q) {
  if (q->size == 0) {
    MSG ("no element to dequeue\n");
    return NULL;
  }

  Task *t = q->head;

  q->head = q->head->next;
  q->size--;
  t->next = NULL;

  return t;

}

static bool is_empty(Queue *q) {
  return (q->tail == NULL);
}
static void init_queue(Queue *q) {
  q->size = 0;
  q->head = NULL;
  q->tail = NULL;
}


static void long_term_schedule() {

  Task *t;
  Task *tmp;

  if (!tasks) return;

  t = tasks;
  while (t) {
    if (t->arrive_time == time) {
      tmp = t;
      tasks = t->next;
      t = t->next;
      tmp->next = NULL;
      enqueue_task(tmp);
    } else {
      t = t->next;
    }
  }
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
  init_queue(&H_queue);
  init_queue(&M_queue);
  init_queue(&L_queue);

  // DEBUG
  while (tasks) {

    printf("\n==========time %d===========\n", time);
    long_term_schedule();

    print_queue(&H_queue);
    print_queue(&M_queue);
    print_queue(&L_queue);

    time++;
  }

  return 0;

}





