#include <math.h>
#include <stdio.h>
#include "cachetable/checkpoint.h"
#include "test.h"
#include <string.h>

#define NUM_WORKER 1

static TOKUTXN const null_txn = 0;
static size_t valsize;
static size_t keysize;
static size_t numrows;
static const double epsilon = 0.5;
static int fanout;  
static size_t nodesize;
static size_t basementsize;
static size_t random_numbers;
static int run_rand;

bool does_file_exist (const char * name);

static int uint64_dbt_cmp (DB *db, const DBT *a, const DBT *b) {
  assert(db && a && b);
  assert(a->size == sizeof(uint64_t)*keysize);
  assert(b->size == sizeof(uint64_t)*keysize);

  uint64_t x = *(uint64_t *) a->data;
  uint64_t y = *(uint64_t *) b->data;

    if (x<y) return -1;
    if (x>y) return 1;
    return 0;
}

static inline double
parse_args_for_nodesize (int argc, const char *argv[]) {
    const char *progname=argv[0];
    if(argc != 6) {
	fprintf(stderr, "Usage:\n %s [rand/seq] [fanout] [nodesize.MBs] [keysize.B] [valuesize.B]]\n", progname);
	exit(1);
    }

    argc--; argv++;
    run_rand = strcmp(argv[0], "seq");

    argc--; argv++;
    sscanf(argv[0], "%d", &fanout);
    if (fanout <= 0) {
	fprintf(stderr, "Fanout must be greater than 0");
	exit(1);
    }

    argc--; argv++;
    double nodeMB;
    sscanf(argv[0], "%lf", &nodeMB);  

    argc--; argv++;
    sscanf(argv[0], "%zu", &keysize);
    if (keysize % 8 != 0) {
	fprintf(stderr, "Keysize must be a multiple of 8");
	exit(1);
    }
    keysize = keysize/8;

    argc--; argv++;
    sscanf(argv[0], "%zu", &valsize);
    return nodeMB;
}

static inline void calculate_numrows() {
    /* calculated based on wanting a 16GiB database */
    size_t desiredsize = 4 * 1024 * 1024;
    size_t pairsize = keysize + valsize;
    // add 1 to division if desiredsize is not a multiple of pairsize
    numrows = (desiredsize / pairsize);
    numrows = numrows *1024*2;
}

static inline void calculate_randomnumbers() {
    random_numbers =  numrows/2;
}

static inline void randomize(uint64_t *array) {
    srand(12321);
    for (uint64_t i = 0; i < numrows; i++) {
	array[i] = i;
    }
    for (uint64_t i = (run_rand) ? (numrows - 1) : (numrows/2 - 1); i > 0; i--) {
	uint64_t j = random() % (i + 1);
	uint64_t temp = array[j];
	array[j] = array[i];
	array[i] = temp;
    }

}

/* static inline void randomize(uint64_t *array, int seed, int setup) {
    srand(seed);
    for (uint64_t i = 0; i < random_numbers; i++) {
        array[i] = ((random() % (random_numbers)) << 32) + i + (setup) ? 0 : random_numbers;
    }
} */

bool does_file_exist (const char * name) {
    if (FILE *file = fopen(name, "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}
