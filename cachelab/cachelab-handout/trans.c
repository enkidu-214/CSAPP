/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{

    int  C=4; 
    for(int i=0;i<M ;i+=C) {
        for (int j = 0; j < N; j += C) {
            for (int i1 = i; i1 < i + C; i1++) {
                for (int j1 = j; j1 < j + C-3; j1+=C) {
                    int tmp1=A[i1][j1];
                    int tmp2= A[i1][j1+1];
                    int tmp3=A[i1][j1+2];
                    int tmp4=A[i1][j1+3];
                    //int tmp5=A[i1][j1+4];
                    //int tmp6=A[i1][j1+5];
                    //int tmp7=A[i1][j1+6];
                    //int tmp8=A[i1][j1+7];
                    B[j1][i1]=tmp1;
                    B[j1+1][i1]=tmp2;
                    B[j1+2][i1]=tmp3;
                    B[j1+3][i1]=tmp4;
                    //B[j1+4][i1]=tmp5;
                    //B[j1+5][i1]=tmp6;
                    //B[j1+6][i1]=tmp7;
                    //B[j1+7][i1]=tmp8;
                }
            }
        }
    }
}


/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

