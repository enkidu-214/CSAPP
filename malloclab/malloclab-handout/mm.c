/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* 头部/脚部的大小 */
#define WSIZE 4
/* 双字 */
#define DSIZE 8

#define CLASS_SIZE 18

#define MAX(a,b) a>b?a:b;

/* 扩展堆时的默认大小，单位为字节 */
#define CHUNKSIZE (1 << 12)

/* 设置头部和脚部的值, 块大小+分配位 */
#define PACK(size, alloc) ((size) | (alloc))

/* 读写指针p的位置 */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) ((*(unsigned int *)(p)) = (val))
/* 给定序号，找到链表头节点位置 */
#define GET_HEAD(num) ((unsigned int *)(long)(GET(heap_list + WSIZE * num)))

/* 读写当前块的前驱和后置块,一开始解引用了
一开始的写法是#define GET_PREV(p) ((unsigned int *)(p))，但是这样只是获取了指针p的位置，应该先GET(p)，将p指向的内容作为指针计算
 */
#define GET_PREV(p) ((unsigned int *)(long)(GET(bp)))
#define GET_NEXT(p) ((unsigned int *)(long)(GET((unsigned int *)bp + 1)))



/* 从头部或脚部获取大小或分配位 */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* 给定有效载荷指针, 找到头部和脚部 */
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* 给定有效载荷指针, 找到前一块或下一块 */
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))

static char* heap_list;

static void* extend_heap(size_t words);
static void* coalesce(void *bp);
static void place(void *bp,size_t size);
static void* first_fit(size_t size);
static int search(size_t size);
static void insert(void * bp);
static void delete(void *bp);


int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
void *mm_realloc(void *ptr, size_t size);

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */
/* 
 * mm_init - initialize the malloc package.
 */
 
int mm_init(void)
{
    /* 申请四个字空间 */
    if((heap_list = mem_sbrk((4+CLASS_SIZE) *WSIZE)) == (void *)-1)
        return -1;

    for(int i=0;i<CLASS_SIZE;i++){
    //之前写成了PUT(heap_list+CLASS_SIZE*i,NULL);
        PUT(heap_list+WSIZE*i,NULL);
    }

    PUT(heap_list+CLASS_SIZE*WSIZE, 0);                              /* 对齐 */
    /* 
     * 序言块和结尾块均设置为已分配, 方便考虑边界情况
     */
    PUT(heap_list+CLASS_SIZE*WSIZE + (1*WSIZE), PACK(DSIZE, 1));     /* 填充序言块 */
    PUT(heap_list+CLASS_SIZE*WSIZE + (2*WSIZE), PACK(DSIZE, 1));     /* 填充序言块 */
    PUT(heap_list+CLASS_SIZE*WSIZE + (3*WSIZE), PACK(0, 1));         /* 结尾块 */

    /* 扩展空闲空间，使用/WSIZE和之后*WSIZE的方法保证双字对齐 */
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * 扩展heap, 传入的是字数
*/
void *extend_heap(size_t words)
{
    /* bp总是指向有效载荷 */
    char *bp;
    size_t size;
    /* 根据传入字数奇偶, 考虑对齐，双字对齐 */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;

    /* 分配 */
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* 设置头部和脚部 */
    PUT(HDRP(bp), PACK(size, 0));           /* 空闲块头 */
    PUT(FTRP(bp), PACK(size, 0));           /* 空闲块脚 */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));   /* 片的新结尾块 */

    /* 判断相邻块是否是空闲块, 进行合并 */
    return coalesce(bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize,extend_size;
    char* bp;
    if(size == 0) return NULL;
    //双字起步，判断该size属于哪个链表
    else if(size <= DSIZE){
        asize=DSIZE*2;
    }
    else{
        asize=DSIZE*((size + DSIZE + DSIZE-1)/DSIZE);
    }

    if( (bp =  first_fit(asize))!=NULL){
        place(bp,asize);
        return bp;
    }
    extend_size = MAX(CHUNKSIZE,asize);
    if((bp = extend_heap(extend_size/WSIZE)) == NULL)
        return NULL;
    place(bp,asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.传入的是指向有效块的指针
 */
void mm_free(void *ptr)
{
    if(ptr == NULL) return;
    size_t size = GET_SIZE(HDRP(ptr));
    if(size <= 0) return;
    //delete(ptr);
    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    size = GET_SIZE(HDRP(oldptr));
    copySize = GET_SIZE(HDRP(newptr));
    //看哪个大小小，根据大小决定复制的量
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize-WSIZE);
    mm_free(oldptr);
    return newptr;
}

//合并空闲块
void * coalesce(void * bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if(prev_alloc && next_alloc){
        insert(bp);
        return bp;
    } 
    
    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));  //增加当前块大小
        delete(NEXT_BLKP(bp));
        PUT(HDRP(bp),PACK(size,0));
        //注意，此处之前一直写PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));，但是头部的大小已经改变了，这样会造成越界问题
        PUT(FTRP(bp),PACK(size,0));
    }
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));  //增加当前块大小
        delete(PREV_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0));
        bp=PREV_BLKP(bp);
    }
    else{
 	delete(PREV_BLKP(bp));
 	delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        bp=PREV_BLKP(bp);
    }
    insert(bp);
    return bp;
}

void place(void *bp,size_t size){
    size_t csize=GET_SIZE(HDRP(bp));
    //表明分配的空间除了头部和尾部以外还能放下
    //之前没有在这里删除bp
    delete(bp);
    if(csize - size >=2*DSIZE){
        PUT(HDRP(bp),PACK(size,1));
        PUT(FTRP(bp),PACK(size,1));
        bp=NEXT_BLKP(bp);
        PUT(HDRP(bp),PACK(csize - size,0));
        PUT(FTRP(bp),PACK(csize - size,0));
        insert(bp);
    }
    //虽然实际内存可能大了点，但是就当填充了
    else{
        PUT(HDRP(bp),PACK(csize,1));
        PUT(FTRP(bp),PACK(csize,1));
    }
}

//找到链表中较大的空闲块
void *first_fit(size_t size){
    unsigned int* bp;
    //找到当前指针指向数据的大小
    int num=search(size);
    while(num<CLASS_SIZE){
        bp = GET_HEAD(num);
        while(bp){
            //之前写成了GET_SIZE(bp) >= size
            if(GET_SIZE(HDRP(bp)) >= size){
                return (void *)bp;
            }
            bp =  GET_NEXT(bp);
        }
        num++;
    }

    return NULL;
}

//返回能够容纳该size的头结点位置
int search(size_t size){
    for(int i=4;i<=22;i++){
        if(size <= 1<<i){
            return i-4;
        }
    }
    return CLASS_SIZE-1;
}

//将当前大小的空白块放到链表的合适位置
void insert(void * bp){
    size_t size = GET_SIZE(HDRP(bp));
    int num = search(size);
    //插入到某个链表的头结点中，假如当前链表无值直接插，有值则插第一个
    //注意指针，GET_HEAD已经完成了解引用，所以我们想要修改头结点本身的地址就不能用这个
    if(GET_HEAD(num) == NULL){
        PUT(heap_list + WSIZE * num, bp);
        /* 前驱 */
        PUT(bp, NULL);
		/* 后继 */
        PUT((unsigned int *)bp + 1, NULL);
	} 
    else {
        /* bp的后继放第一个节点 */
	PUT((unsigned int *)bp + 1, GET_HEAD(num));
		/* 第一个节点的前驱放bp */
        PUT(GET_HEAD(num), bp);
        /* bp的前驱为空 */  	
	PUT(bp, NULL);
        /* 头节点放bp */
	PUT(heap_list + WSIZE * num, bp);
	}
}
    

void delete(void * bp){
    size_t size = GET_SIZE(HDRP(bp));
    int num = search(size);
    //分三种情况，即在头结点且无后继，在头结点且有后继，不在头结点
    /* 
     * 唯一节点,后继为null,前驱为null 
     * 头节点设为null
     */
    if (GET_PREV(bp) == NULL && GET_NEXT(bp) == NULL) { 
        PUT(heap_list + WSIZE * num, NULL);
    } 
    /* 
     * 最后一个节点 
     * 前驱的后继设为null
     */
    else if (GET_PREV(bp) != NULL && GET_NEXT(bp) == NULL) {
        PUT(GET_PREV(bp) + 1, NULL);
    } 
    /* 
     * 第一个结点 
     * 头节点设为bp的后继
     */
    else if (GET_NEXT(bp) != NULL && GET_PREV(bp) == NULL){
        PUT(heap_list + WSIZE * num, GET_NEXT(bp));
        PUT(GET_NEXT(bp), NULL);
    }
    /* 
     * 中间结点 
     * 前驱的后继设为后继
     * 后继的前驱设为前驱
     */
    else if (GET_NEXT(bp) != NULL && GET_PREV(bp) != NULL) {
        PUT(GET_PREV(bp) + 1, GET_NEXT(bp));
        PUT(GET_NEXT(bp), GET_PREV(bp));
    }
}
