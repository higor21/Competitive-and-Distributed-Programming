
/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/*  Integer radix sort of non-negative integers.                         */
/*                                                                       */
/*  Command line options:                                                */
/*                                                                       */
/*  -pP : P = number of processors.                                      */
/*  -rR : R = radix for sorting.  Must be power of 2.                    */
/*  -nN : N = number of keys to sort.                                    */
/*  -mM : M = maximum key value.  Integer keys k will be generated such  */
/*        that 0 <= k <= M.                                              */
/*  -s  : Print individual processor timing statistics.                  */
/*  -t  : Check to make sure all keys are sorted correctly.              */
/*  -o  : Print out sorted keys.                                         */
/*  -h  : Print out command line options.                                */
/*                                                                       */
/*  Default: RADIX -p1 -n262144 -r1024 -m524288                          */
/*                                                                       */
/*  Note: This version works under both the FORK and SPROC models        */
/*                                                                       */
/*************************************************************************/
//to build you need use gcc -g -Wall -o radix.o radix.c -lpthread
//To run ./radix.o
//To build with gprof you need gcc -g -Wall -o radix.o radix.c -lpthread -pg
// ./radix.o
// gprof radix.o

#include <stdio.h>
#include <math.h>

#define DEFAULT_P 1
#define DEFAULT_N 262144
#define DEFAULT_R 1024
#define DEFAULT_M 524288
#define MAX_PROCESSORS 128
#define RADIX_S 8388608.0e0
#define RADIX 70368744177664.0e0
#define SEED 314159265.0e0
#define RATIO 1220703125.0e0
#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define MAX_RADIX 4096

#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#define MAX_THREADS 1024

struct prefix_node
{
  int densities[MAX_RADIX];
  int ranks[MAX_RADIX];

  struct
  {
    unsigned long Flag;
  } done;

  char pad[PAGE_SIZE];
};

struct global_memory
{
  int Index;                      

  struct
  {
    unsigned long counter;
    unsigned long cycle;
  }(barrier_rank);

  struct
  {
    unsigned long counter;
    unsigned long cycle;
  }(barrier_key);
  
  double *ranktime;
  double *sorttime;
  double *totaltime;
  int final;
  unsigned long starttime;
  unsigned long rs;
  unsigned long rf;
  struct prefix_node prefix_tree[2 * MAX_PROCESSORS];
} * global;

struct global_private
{
  char pad[PAGE_SIZE];
  int *rank_ff;
} gp[MAX_PROCESSORS];

int *key[2];         /* sort from one index into the other */
int **rank_me;       /* individual processor ranks */
int *key_partition;  /* keys a processor works on */
int *rank_partition; /* ranks a processor works on */

int number_of_processors = DEFAULT_P;
int max_num_digits;
int radix = DEFAULT_R;
int num_keys = DEFAULT_N;
int max_key = DEFAULT_M;
int log2_radix;
int log2_keys;
int dostats = 0;
int test_result = 0;
int doprint = 0;

double ran_num_init(unsigned int, double, double);
double product_mod_46(double, double);
int get_max_digits(int);
int get_log2_radix(int);
int get_log2_keys(int);
void slave_sort();
int log_2(int);
void printerr(char *);
void init(int, int, int);
void test_sort(int);
void printout();

int main(argc, argv)

    int argc;
char *argv;

{
  int i;
  int p;
  int quotient;
  int remainder;
  int sum_i;
  int sum_f;
  int size;
  int **temp;
  int **temp2;
  int *a;
  int c;
  extern char *optarg;
  double mint, maxt, avgt;
  double minrank, maxrank, avgrank;
  double minsort, maxsort, avgsort;
  unsigned long start;

  {
    struct timeval FullTime;

    gettimeofday(&FullTime, NULL);
    (start) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000.0);
  }

  while ((c = getopt(argc, argv, "p:r:n:m:stoh")) != -1)
  {
    switch (c)
    {
    case 'p':
      number_of_processors = atoi(optarg);
      if (number_of_processors < 1)
      {
        printerr("P must be >= 1\n");
        exit(-1);
      }
      if (number_of_processors > MAX_PROCESSORS)
      {
        printerr("Maximum processors (MAX_PROCESSORS) exceeded\n");
        exit(-1);
      }
      break;
    case 'r':
      radix = atoi(optarg);
      if (radix < 1)
      {
        printerr("Radix must be a power of 2 greater than 0\n");
        exit(-1);
      }
      log2_radix = log_2(radix);
      if (log2_radix == -1)
      {
        printerr("Radix must be a power of 2\n");
        exit(-1);
      }
      break;
    case 'n':
      num_keys = atoi(optarg);
      if (num_keys < 1)
      {
        printerr("Number of keys must be >= 1\n");
        exit(-1);
      }
      break;
    case 'm':
      max_key = atoi(optarg);
      if (max_key < 1)
      {
        printerr("Maximum key must be >= 1\n");
        exit(-1);
      }
      break;
    case 's':
      dostats = !dostats;
      break;
    case 't':
      test_result = !test_result;
      break;
    case 'o':
      doprint = !doprint;
      break;
    case 'h':
      printf("Usage: RADIX <options>\n\n");
      printf("   -pP : P = number of processors.\n");
      printf("   -rR : R = radix for sorting.  Must be power of 2.\n");
      printf("   -nN : N = number of keys to sort.\n");
      printf("   -mM : M = maximum key value.  Integer keys k will be generated such\n");
      printf("         that 0 <= k <= M.\n");
      printf("   -s  : Print individual processor timing statistics.\n");
      printf("   -t  : Check to make sure all keys are sorted correctly.\n");
      printf("   -o  : Print out sorted keys.\n");
      printf("   -h  : Print out command line options.\n\n");
      printf("Default: RADIX -p%1d -n%1d -r%1d -m%1d\n",
             DEFAULT_P, DEFAULT_N, DEFAULT_R, DEFAULT_M);
      exit(0);
    }
  }

  {
    ;
  }

  log2_radix = log_2(radix);
  log2_keys = log_2(num_keys);
  global = (struct global_memory *)malloc(sizeof(struct global_memory));
  key[0] = (int *)malloc(num_keys * sizeof(int));
  ;
  key[1] = (int *)malloc(num_keys * sizeof(int));
  ;
  key_partition = (int *)malloc((number_of_processors + 1) * sizeof(int));
  ;
  rank_partition = (int *)malloc((number_of_processors + 1) * sizeof(int));
  ;
  global->ranktime = (double *)malloc(number_of_processors * sizeof(double));
  ;
  global->sorttime = (double *)malloc(number_of_processors * sizeof(double));
  ;
  global->totaltime = (double *)malloc(number_of_processors * sizeof(double));
  ;
  size = number_of_processors * (radix * sizeof(int) + sizeof(int *));
  rank_me = (int **)malloc(size);
  ;
  if ((global == NULL) || (key[0] == NULL) || (key[1] == NULL) ||
      (key_partition == NULL) || (rank_partition == NULL) ||
      (rank_me == NULL))
  {
    fprintf(stderr, "ERROR: Cannot malloc enough memory\n");
    exit(-1);
  }

  temp = rank_me;
  temp2 = temp + number_of_processors;
  a = (int *)temp2;
  for (i = 0; i < number_of_processors; i++)
  {
    *temp = (int *)a;
    temp++;
    a += radix;
  }
  for (i = 0; i < number_of_processors; i++)
  {
    gp[i].rank_ff = (int *)malloc(radix * sizeof(int) + PAGE_SIZE);;
  }
 
  for (i = 0; i < 2 * number_of_processors; i++)
  {
    {
      global->prefix_tree[i].done.Flag = 0;
    };
  }

  global->Index = 0;
  max_num_digits = get_max_digits(max_key);
  printf("\n");
  printf("Integer Radix Sort\n");
  printf("     %d Keys\n", num_keys);
  printf("     %d Processors\n", number_of_processors);
  printf("     Radix = %d\n", radix);
  printf("     Max key = %d\n", max_key);
  printf("\n");

  quotient = num_keys / number_of_processors;
  remainder = num_keys % number_of_processors;
  sum_i = 0;
  sum_f = 0;
  p = 0;
  while (sum_i < num_keys)
  {
    key_partition[p] = sum_i;
    p++;
    sum_i = sum_i + quotient;
    sum_f = sum_f + remainder;
    sum_i = sum_i + sum_f / number_of_processors;
    sum_f = sum_f % number_of_processors;
  }
  key_partition[p] = num_keys;

  quotient = radix / number_of_processors;
  remainder = radix % number_of_processors;
  sum_i = 0;
  sum_f = 0;
  p = 0;
  while (sum_i < radix)
  {
    rank_partition[p] = sum_i;
    p++;
    sum_i = sum_i + quotient;
    sum_f = sum_f + remainder;
    sum_i = sum_i + sum_f / number_of_processors;
    sum_f = sum_f % number_of_processors;
  }
  rank_partition[p] = radix;

#pragma omp parallel num_threads(number_of_processors)
  slave_sort();

  printf("\n");
  printf("                 PROCESS STATISTICS\n");
  printf("               Total            Rank            Sort\n");
  printf(" Proc          Time             Time            Time\n");
  printf("    0     %10.0f      %10.0f      %10.0f\n",
         global->totaltime[0], global->ranktime[0],
         global->sorttime[0]);
  if (dostats)
  {
    maxt = avgt = mint = global->totaltime[0];
    maxrank = avgrank = minrank = global->ranktime[0];
    maxsort = avgsort = minsort = global->sorttime[0];
    for (i = 1; i < number_of_processors; i++)
    {
      if (global->totaltime[i] > maxt)
      {
        maxt = global->totaltime[i];
      }
      if (global->totaltime[i] < mint)
      {
        mint = global->totaltime[i];
      }
      if (global->ranktime[i] > maxrank)
      {
        maxrank = global->ranktime[i];
      }
      if (global->ranktime[i] < minrank)
      {
        minrank = global->ranktime[i];
      }
      if (global->sorttime[i] > maxsort)
      {
        maxsort = global->sorttime[i];
      }
      if (global->sorttime[i] < minsort)
      {
        minsort = global->sorttime[i];
      }
      avgt += global->totaltime[i];
      avgrank += global->ranktime[i];
      avgsort += global->sorttime[i];
    }
    avgt = avgt / number_of_processors;
    avgrank = avgrank / number_of_processors;
    avgsort = avgsort / number_of_processors;
    for (i = 1; i < number_of_processors; i++)
    {
      printf("  %3d     %10.0f      %10.0f      %10.0f\n",
             i, global->totaltime[i], global->ranktime[i],
             global->sorttime[i]);
    }
    printf("  Avg     %10.0f      %10.0f      %10.0f\n", avgt, avgrank, avgsort);
    printf("  Min     %10.0f      %10.0f      %10.0f\n", mint, minrank, minsort);
    printf("  Max     %10.0f      %10.0f      %10.0f\n", maxt, maxrank, maxsort);
    printf("\n");
  }

  printf("\n");
  global->starttime = start;
  printf("                 TIMING INFORMATION\n");
  printf("Start time                        : %16ld\n",
         global->starttime);
  printf("Initialization finish time        : %16ld\n",
         global->rs);
  printf("Overall finish time               : %16ld\n",
         global->rf);
  printf("Total time with initialization    : %16ld\n",
         global->rf - global->starttime);
  printf("Total time without initialization : %16ld\n",
         global->rf - global->rs);
  printf("\n");

  if (doprint)
  {
    printout();
  }
  if (test_result)
  {
    test_sort(global->final);
  }

  {
    exit(0);
  };
}

void slave_sort()
{
  int i;
  int MyNum;
  int this_key;
  int tmp;
  int loopnum;
  int shiftnum;
  int bb;
  int my_key;
  int key_start;
  int key_stop;
  int rank_stop;
  int from = 0;
  int to = 1;
  int *key_density; /* individual processor key densities */
  unsigned long time1;
  unsigned long time2;
  unsigned long time3;
  unsigned long time4;
  unsigned long time5;
  unsigned long time6;
  double ranktime = 0;
  double sorttime = 0;
  int *key_from;
  int *key_to;
  int *rank_me_mynum;
  int *rank_ff_mynum;
  int stats;
  struct prefix_node *n;
  struct prefix_node *r;
  struct prefix_node *l;
  struct prefix_node *my_node;
  struct prefix_node *their_node;
  int index;
  int level;
  int base;
  int offset;

  stats = dostats;

  MyNum = omp_get_thread_num();

  key_density = (int *)malloc(radix * sizeof(int));

  key_start = key_partition[MyNum];
  key_stop = key_partition[MyNum + 1];
  rank_stop = rank_partition[MyNum + 1];
  if (rank_stop == radix)
  {
    rank_stop--;
  }

  init(key_start, key_stop, from);

  if ((MyNum == 0) || (stats))
  {
    {
      struct timeval FullTime;

      gettimeofday(&FullTime, NULL);
      (time1) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
    }
  }

  rank_me_mynum = rank_me[MyNum];
  rank_ff_mynum = gp[MyNum].rank_ff;
  for (loopnum = 0; loopnum < max_num_digits; loopnum++)
  {
    shiftnum = (loopnum * log2_radix);
    bb = (radix - 1) << shiftnum;

    if ((MyNum == 0) || (stats))
    {
      {
        struct timeval FullTime;

        gettimeofday(&FullTime, NULL);
        (time2) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
      }
    }

    for (i = 0; i < radix; i++)
    {
      rank_me_mynum[i] = 0;
    }
    key_from = (int *)key[from];
    key_to = (int *)key[to];
    for (i = key_start; i < key_stop; i++)
    {
      my_key = key_from[i] & bb;
      my_key = my_key >> shiftnum;
      rank_me_mynum[my_key]++;
    }
    key_density[0] = rank_me_mynum[0];
    for (i = 1; i < radix; i++)
    {
      key_density[i] = key_density[i - 1] + rank_me_mynum[i];
    }

    n = &(global->prefix_tree[MyNum]);
    for (i = 0; i < radix; i++)
    {
      n->densities[i] = key_density[i];
      n->ranks[i] = rank_me_mynum[i];
    }
    offset = MyNum;
    level = number_of_processors >> 1;
    base = number_of_processors;

    if ((MyNum & 0x1) == 0)
    {
      {

#pragma omp critical(reg1)
        global->prefix_tree[base + (offset >> 1)].done.Flag = 1;
      };
    }
    while ((offset & 0x1) != 0)
    {
      offset >>= 1;
      r = n;
      l = n - 1;
      index = base + offset;
      n = &(global->prefix_tree[index]);

      while (n->done.Flag == 0){}

#pragma omp critical(reg2)
      n->done.Flag = 0;

      if (offset != (level - 1))
      {
        for (i = 0; i < radix; i++)
        {
          n->densities[i] = r->densities[i] + l->densities[i];
          n->ranks[i] = r->ranks[i] + l->ranks[i];
        }
      }
      else
      {
        for (i = 0; i < radix; i++)
        {
          n->densities[i] = r->densities[i] + l->densities[i];
        }
      }
      base += level;
      level >>= 1;
      if ((offset & 0x1) == 0)
      {
        {
#pragma omp critical(reg3)
          global->prefix_tree[base + (offset >> 1)].done.Flag = 1;
        };
      }
    }

    if (MyNum != (number_of_processors - 1))
    {
      offset = MyNum;
      level = number_of_processors;
      base = 0;
      while ((offset & 0x1) != 0)
      {
        offset >>= 1;
        base += level;
        level >>= 1;
      }
      my_node = &(global->prefix_tree[base + offset]);
      offset >>= 1;
      base += level;
      level >>= 1;
      while ((offset & 0x1) != 0)
      {
        offset >>= 1;
        base += level;
        level >>= 1;
      }
      their_node = &(global->prefix_tree[base + offset]);

      while (my_node->done.Flag == 0){}

#pragma omp critical(reg4)
      my_node->done.Flag = 0;

      for (i = 0; i < radix; i++)
      {
        my_node->densities[i] = their_node->densities[i];
      }
    }
    else
    {
      my_node = &(global->prefix_tree[(2 * number_of_processors) - 2]);
    }
    offset = MyNum;
    level = number_of_processors;
    base = 0;
    while ((offset & 0x1) != 0)
    {
      {
#pragma omp critical(reg5)
        global->prefix_tree[base + offset - 1].done.Flag = 1;
      };
      offset >>= 1;
      base += level;
      level >>= 1;
    }
    offset = MyNum;
    level = number_of_processors;
    base = 0;
    for (i = 0; i < radix; i++)
    {
      rank_ff_mynum[i] = 0;
    }
    while (offset != 0)
    {
      if ((offset & 0x1) != 0)
      {
        l = &(global->prefix_tree[base + offset - 1]);
        for (i = 0; i < radix; i++)
        {
          rank_ff_mynum[i] += l->ranks[i];
        }
      }
      base += level;
      level >>= 1;
      offset >>= 1;
    }
    for (i = 1; i < radix; i++)
    {
      rank_ff_mynum[i] += my_node->densities[i - 1];
    }

    if ((MyNum == 0) || (stats))
    {
      {
        struct timeval FullTime;

        gettimeofday(&FullTime, NULL);
        (time3) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
      };
    }

    if ((MyNum == 0) || (stats))
    {
      {
        struct timeval FullTime;

        gettimeofday(&FullTime, NULL);
        (time4) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
      };
    }

    for (i = key_start; i < key_stop; i++)
    {
      this_key = key_from[i] & bb;
      this_key = this_key >> shiftnum;
      tmp = rank_ff_mynum[this_key];
      key_to[tmp] = key_from[i];
      rank_ff_mynum[this_key]++;
    } /*  i */

    if ((MyNum == 0) || (stats))
    {
      {
        struct timeval FullTime;

        gettimeofday(&FullTime, NULL);
        (time5) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
      };
    }

    if (loopnum != max_num_digits - 1)
    {
      from = from ^ 0x1;
      to = to ^ 0x1;
    }

    if ((MyNum == 0) || (stats))
    {
      ranktime += (time3 - time2);
      sorttime += (time5 - time4);
    }
  } /* for */

  if ((MyNum == 0) || (stats))
  {
    {
      struct timeval FullTime;

      gettimeofday(&FullTime, NULL);
      (time6) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
    }
    global->ranktime[MyNum] = ranktime;
    global->sorttime[MyNum] = sorttime;
    global->totaltime[MyNum] = time6 - time1;
  }
  if (MyNum == 0)
  {
    global->rs = time1;
    global->rf = time6;
    global->final = to;
  }
}

double product_mod_46(t1, t2) double t1;
double t2;

{
  double a1;
  double b1;
  double a2;
  double b2;

  a1 = (double)((int)(t1 / RADIX_S)); /* Decompose the arguments.  */
  a2 = t1 - a1 * RADIX_S;
  b1 = (double)((int)(t2 / RADIX_S));
  b2 = t2 - b1 * RADIX_S;
  t1 = a1 * b2 + a2 * b1; /* Multiply the arguments.  */
  t2 = (double)((int)(t1 / RADIX_S));
  t2 = t1 - t2 * RADIX_S;
  t1 = t2 * RADIX_S + a2 * b2;
  t2 = (double)((int)(t1 / RADIX));

  return (t1 - t2 * RADIX); /* Return the product.  */
}

double ran_num_init(k, b, t) unsigned int k;
double b;
double t;

{
  unsigned int j;

  while (k != 0)
  {
    j = k >> 1;
    if ((j << 1) != k)
    {
      b = product_mod_46(b, t);
    }
    t = product_mod_46(t, t);
    k = j;
  }

  return b;
}

int get_max_digits(max_key)

    int max_key;

{
  int done = 0;
  int temp = 1;
  int key_val;

  key_val = max_key;
  while (!done)
  {
    key_val = key_val / radix;
    if (key_val == 0)
    {
      done = 1;
    }
    else
    {
      temp++;
    }
  }
  return temp;
}

int get_log2_radix(rad)

    int rad;

{
  int cumulative = 1;
  int out;

  for (out = 0; out < 20; out++)
  {
    if (cumulative == rad)
    {
      return (out);
    }
    else
    {
      cumulative = cumulative * 2;
    }
  }
  fprintf(stderr, "ERROR: Radix %d not a power of 2\n", rad);
  exit(-1);
}

int get_log2_keys(num_keys)

    int num_keys;

{
  int cumulative = 1;
  int out;

  for (out = 0; out < 30; out++)
  {
    if (cumulative == num_keys)
    {
      return (out);
    }
    else
    {
      cumulative = cumulative * 2;
    }
  }
  fprintf(stderr, "ERROR: Number of keys %d not a power of 2\n", num_keys);
  exit(-1);
}

int log_2(number)

    int number;

{
  int cumulative = 1;
  int out = 0;
  int done = 0;

  while ((cumulative < number) && (!done) && (out < 50))
  {
    if (cumulative == number)
    {
      done = 1;
    }
    else
    {
      cumulative = cumulative * 2;
      out++;
    }
  }

  if (cumulative == number)
  {
    return (out);
  }
  else
  {
    return (-1);
  }
}

void printerr(s)

    char *s;

{
  fprintf(stderr, "ERROR: %s\n", s);
}

void init(key_start, key_stop, from)

    int key_start;
int key_stop;
int from;

{
  double ran_num;
  double sum;
  int i;
  int *key_from;

  ran_num = ran_num_init((key_start << 2) + 1, SEED, RATIO);
  sum = ran_num / RADIX;
  key_from = (int *)key[from];
  for (i = key_start; i < key_stop; i++)
  {
    ran_num = product_mod_46(ran_num, RATIO);
    sum = sum + ran_num / RADIX;
    ran_num = product_mod_46(ran_num, RATIO);
    sum = sum + ran_num / RADIX;
    ran_num = product_mod_46(ran_num, RATIO);
    sum = sum + ran_num / RADIX;
    key_from[i] = (int)((sum / 4.0) * max_key);
    ran_num = product_mod_46(ran_num, RATIO);
    sum = ran_num / RADIX;
  }
}

void test_sort(final)

    int final;

{
  int i;
  int mistake = 0;
  int *key_final;

  printf("\n");
  printf("                  TESTING RESULTS\n");
  key_final = key[final];
  for (i = 0; i < num_keys - 1; i++)
  {
    if (key_final[i] > key_final[i + 1])
    {
      fprintf(stderr, "error with key %d, value %d %d \n",
              i, key_final[i], key_final[i + 1]);
      mistake++;
    }
  }

  if (mistake)
  {
    printf("FAILED: %d keys out of place.\n", mistake);
  }
  else
  {
    printf("PASSED: All keys in place.\n");
  }
  printf("\n");
}

void printout()

{
  int i;
  //int mistake;
  int *key_final;

  key_final = (int *)key[global->final];
  printf("\n");
  printf("                 SORTED KEY VALUES\n");
  printf("%8d ", key_final[0]);
  for (i = 0; i < num_keys - 1; i++)
  {
    printf("%8d ", key_final[i + 1]);
    if ((i + 2) % 5 == 0)
    {
      printf("\n");
    }
  }
  printf("\n");
}
