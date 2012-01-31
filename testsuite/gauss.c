/**
 * gauss.c
 *
 * Gaussian elimination example.
 *
 * Originally written by Mike Lam, March 2010
 *
 */

/* {{{ dependencies */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
/* }}} */

/* {{{ macros */

/*#define FMT "%12g"*/
/*#define REAL float*/
/*#define EQUAL f_approx_equal*/
#define FMT "%10.3lg"
#define REAL double
#define EQUAL d_approx_equal

#define GET(M,I,J)   ((M)->orient==ROW ? (M)->dat[(I)*((M)->cols)+(J)] : (M)->dat[(I)+(J)*((M)->rows)])
#define SET(M,I,J,V) if ((M)->orient==ROW) { (M)->dat[(I)*((M)->cols)+(J)] = (V); } else { (M)->dat[(I)+(J)*((M)->rows)] = (V); }

#define DEFAULT_ORIENTATION ROW

/* }}} */
/* {{{ types */

typedef enum {
    ROW,
    COLUMN
} orientation;

typedef struct {
    int rows;
    int cols;
    orientation orient;
    REAL *dat;
} matrix;

/* }}} */
/* {{{ function definitions */

int gauss();

void init_empty_matrix(matrix *m, int r, int c);
void init_test_matrix(matrix *m);

void print_matrix(matrix *m);

void mult_matrices(matrix *c, matrix *a, matrix *b);        /*  c=a*b  */

void do_gauss(matrix *m);

int check(matrix *m);

/* }}} */
/* {{{ global variables */
matrix ref, mat, lower, upper, mult;
int dbg = 0;
/* }}} */

/* {{{ program entry point */
int main(int argc, char *argv[]) {
    int i;
    for (i=1; i<argc; i++) {
        if (strcmp(argv[i], "-d")==0) {
            dbg = 1;
        }
    }
    return gauss();
}

int gauss() {
    int status;

    init_test_matrix(&ref);
    init_test_matrix(&mat);

    do_gauss(&mat);

#if PRINT_OUTPUT
    printf("== gauss ==\n");
    if (dbg) {
        printf("original matrix:\n");
        print_matrix(&ref);
        printf("after classic w/ zero pivoting:\n");
    }
    print_matrix(&mat);
#endif

    status = check(&mat);
    if (status == EXIT_SUCCESS) {
        printf("== gauss: pass\n");
    } else {
        printf("== gauss: FAIL!!!\n");
    }
    return status;
} /* }}} */

/* {{{ init_empty_matrix */
void init_empty_matrix(matrix *m, int r, int c)
{
    int i, size = r*c;
    m->rows = r;
    m->cols = c;
    m->orient = DEFAULT_ORIENTATION;
    m->dat = (REAL*)malloc(size*sizeof(REAL));
    if (!m->dat) {
        fprintf(stderr, "Out of memory!\n");
        exit(-1);
    }
    for (i=0; i<size; i++) {
        m->dat[i] = (REAL)0.0L;
    }
} /* }}} */
/* {{{ init_test_matrix */
void init_test_matrix(matrix *m)
{
    m->rows = 10;
    m->cols = 10;
    m->orient = DEFAULT_ORIENTATION;
    m->dat = (REAL*)malloc(m->rows*m->cols*sizeof(REAL));
    if (!m->dat) {
        fprintf(stderr, "Out of memory!\n");
        exit(-1);
    }
    SET(m,0,0,-0.24593720094939861);
    SET(m,0,1,0.13961344877996923);
    SET(m,0,2,0.14828145016471075);
    SET(m,0,3,-0.16307135234142317);
    SET(m,0,4,0.22604953984557818);
    SET(m,0,5,-0.20922681511041558);
    SET(m,0,6,0.2083010750895043);
    SET(m,0,7,-0.20503448855642076);
    SET(m,0,8,0.5021708125285308);
    SET(m,0,9,-0.22218794381750978);
    SET(m,1,0,0.23984234592611228);
    SET(m,1,1,0.099317100161458644);
    SET(m,1,2,-0.023411057023450609);
    SET(m,1,3,-0.064452027961241221);
    SET(m,1,4,0.025626789787705315);
    SET(m,1,5,0.088761971866869183);
    SET(m,1,6,-0.32709098036729162);
    SET(m,1,7,-0.045364799210717656);
    SET(m,1,8,0.27342026112708351);
    SET(m,1,9,-0.23366021002771503);
    SET(m,2,0,0.071311450082655839);
    SET(m,2,1,0.29774609160557225);
    SET(m,2,2,-0.19922712685658089);
    SET(m,2,3,-0.49285649224545991);
    SET(m,2,4,0.050156228626672086);
    SET(m,2,5,0.0078273041234832308);
    SET(m,2,6,0.021002683996936642);
    SET(m,2,7,-0.15492038026351279);
    SET(m,2,8,-0.01718729444034707);
    SET(m,2,9,-0.086340372321957126);
    SET(m,3,0,-0.30636984615700807);
    SET(m,3,1,0.062640380039704568);
    SET(m,3,2,-0.12845321756794451);
    SET(m,3,3,-0.018201137339942226);
    SET(m,3,4,-0.057626962386474523);
    SET(m,3,5,-0.095791180563245037);
    SET(m,3,6,0.28068938174815677);
    SET(m,3,7,-0.030273159174550354);
    SET(m,3,8,-0.21116358779792443);
    SET(m,3,9,-0.044410914223404069);
    SET(m,4,0,0.21051375114445994);
    SET(m,4,1,-0.44163142975543979);
    SET(m,4,2,-0.11685348115280351);
    SET(m,4,3,0.18202094476311101);
    SET(m,4,4,0.3372115431961869);
    SET(m,4,5,-0.22641080592190904);
    SET(m,4,6,0.056061486595050919);
    SET(m,4,7,0.050972003645997147);
    SET(m,4,8,0.15180257603661543);
    SET(m,4,9,-0.13686229753576398);
    SET(m,5,0,-0.55917674912507287);
    SET(m,5,1,-0.35055880037788228);
    SET(m,5,2,-0.4433750721417194);
    SET(m,5,3,-0.1451847587000287);
    SET(m,5,4,-0.083164638988414433);
    SET(m,5,5,-0.037253966506140032);
    SET(m,5,6,0.035522636271634586);
    SET(m,5,7,-0.042923246835586751);
    SET(m,5,8,-0.10897288390920522);
    SET(m,5,9,0.03301696615475759);
    SET(m,6,0,-0.30310783872299929);
    SET(m,6,1,-0.047867225505228275);
    SET(m,6,2,0.58663292442866821);
    SET(m,6,3,0.2322486858921185);
    SET(m,6,4,0.18614743071092657);
    SET(m,6,5,-0.2035338657487612);
    SET(m,6,6,0.0013235188059922663);
    SET(m,6,7,-0.24673018493232626);
    SET(m,6,8,-0.18746656871454656);
    SET(m,6,9,-0.038995927541238443);
    SET(m,7,0,0.2935594564940277);
    SET(m,7,1,0.13360625055534245);
    SET(m,7,2,-0.10479307478660202);
    SET(m,7,3,0.20332079319473051);
    SET(m,7,4,0.065084058021855157);
    SET(m,7,5,-0.053326804203619751);
    SET(m,7,6,0.079283204928436143);
    SET(m,7,7,-0.16234928799035367);
    SET(m,7,8,-0.55316646348005394);
    SET(m,7,9,-0.024238526073171646);
    SET(m,8,0,-0.091591941277208197);
    SET(m,8,1,-0.10792684829969959);
    SET(m,8,2,-0.20096569254193553);
    SET(m,8,3,0.049980458216167978);
    SET(m,8,4,0.46647666262962184);
    SET(m,8,5,0.3250725838440176);
    SET(m,8,6,-0.073713550231384678);
    SET(m,8,7,0.27128826736352218);
    SET(m,8,8,0.029192193944838685);
    SET(m,8,9,0.063784369269451413);
    SET(m,9,0,0.02216646541811125);
    SET(m,9,1,-0.048352595947739663);
    SET(m,9,2,0.29765330301201737);
    SET(m,9,3,-0.1848497032957036);
    SET(m,9,4,-0.41128153037471105);
    SET(m,9,5,0.30075196239432894);
    SET(m,9,6,0.30930445615032726);
    SET(m,9,7,0.16039006681772106);
    SET(m,9,8,0.11362127558844121);
    SET(m,9,9,0.25449415617508914);
} /* }}} */

/* {{{ print_matrix */
void print_matrix(matrix *m)
{
    int r, c;
    for (r=0; r<m->rows; r++) {
        printf("   ");
        for (c=0; c<m->cols; c++) {
            printf(FMT " ", GET(m,r,c));
        }
        printf("\n");
    }
} /* }}} */

/* {{{ mult_matrices */
void mult_matrices(matrix *c, matrix *a, matrix *b)
{
    // calculates c=a*b
    int rows, cols, idim, i, j, k;
    REAL sum;
    rows = a->rows;
    cols = b->cols;
    idim = a->cols;
    init_empty_matrix(c, rows, cols);
    for (i=0; i<rows; i++) {
        for (j=0; j<cols; j++) {
            sum = (REAL)0.0L;
            for (k=0; k<idim; k++) {
                sum = sum + GET(a,i,k)*GET(b,k,j);
            }
            SET(c,i,j,sum);
        }
    }
} /* }}} */

/* {{{ do_gauss */
void do_gauss(matrix *m)
{
    int i, j, k;
    REAL temp, temp2;
    for (i=0; i<m->rows; i++) {
        temp = GET(m,i,i);
        for (j=i+1; j<m->rows; j++) {
            SET(m,j,i,GET(m,j,i)/temp);
            temp2 = GET(m,j,i);
            for (k=i+1; k<m->cols; k++) {
                if (j != k) {
                    SET(m,j,k,GET(m,j,k)-(temp2*GET(m,i,k)));
                } else {
                    SET(m,j,j,GET(m,j,j)-(temp2*GET(m,i,j)));
                }
            }
        }
    }
} /* }}} */

/* {{{ check */
int check(matrix *m) {
    int i, j;
    REAL temp;
    init_empty_matrix(&lower, m->rows, m->cols);
    init_empty_matrix(&upper, m->rows, m->cols);
    for (i=0; i<m->rows; i++) {
        for (j=0; j<m->cols; j++) {
            if (i>j) {
                temp = GET(m,i,j);
                SET(&lower,i,j,temp);
            }
            if (i<=j) {
                temp = GET(m,i,j);
                SET(&upper,i,j,temp);
            }
        }
        SET(&lower,i,i,(REAL)1.0L);
    }
    mult_matrices(&mult, &lower, &upper);
    if (dbg) {
        printf("lower:\n");
        print_matrix(&lower);
        printf("upper:\n");
        print_matrix(&upper);
        printf("multiplied:\n");
        print_matrix(&mult);
    }
    for (i=0; i<m->rows; i++) {
        for (j=0; j<m->cols; j++) {
            if (!EQUAL(GET(&mult,i,j),GET(&ref,i,j))) {
                printf("(%d,%d): " FMT " != " FMT "\n",
                        i, j, GET(&mult,i,j), GET(&ref,i,j));
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
} /* }}} */

