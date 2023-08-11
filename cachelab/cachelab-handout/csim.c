#include "cachelab.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
//做这lab的时候忘记free内存了
int Hits=0,Misses=0,Evits=0;
char *path;
typedef struct Cachestruct{
    int tag;
    int vaild;
    int t_stp;
} CacheLine;

CacheLine ** Cache=NULL;

int char2int(char c){
    return c-'0'; 
}

void  freeCache(int S){
    for(int i=0;i<S;i++){
       free(Cache[i]); 
    }
    free(Cache);
}

void InitCache(int S,int E ){
    Cache = (CacheLine**)malloc(sizeof(CacheLine*)*S );
    for(int i=0;i<S;i++){
        Cache[i] = (CacheLine*)malloc(sizeof(CacheLine)*E);
	for(int j=0;j<E;j++){
	    Cache[i][j].tag=-1;
	    Cache[i][j].vaild=0;
	    Cache[i][j].t_stp=-1;
	}
    }
}

void updateTimeStamp(int S,int E){
    for(int i=0;i<S;i++){
        for(int j=0;j<E;j++){
	    if(Cache[i][j].vaild){
	        Cache[i][j].t_stp++;
	    }
	}
    }
}

void update(uint64_t address,int s,int E, int b){
    int group=(address>>b)&(-1U>>(64-s));
    int tag=address>>(b+s);
    //先判断当前位置是否有缓存
    for(int i=0;i<E;i++){
        if(Cache[group][i].tag==tag && Cache[group][i].vaild){
	    Hits++;
	    Cache[group][i].t_stp=0;
	    return;
	}
    }
    Misses++;
    //判断是否有空位可以不覆盖直接写
    for(int i=0;i<E;i++){
        if(!Cache[group][i].vaild){
	    Cache[group][i].vaild=1;
	    Cache[group][i].tag=tag;
	    Cache[group][i].t_stp=0;
	    return;
	}
    }
    Evits++;
    int oldest_time=0;
    int oldest_idx=0;
    for(int i=0;i<E;i++){
        if(Cache[group][i].vaild && Cache[group][i].t_stp>oldest_time){
	    oldest_time=Cache[group][i].t_stp;
	    oldest_idx=i;
	}
    }
    Cache[group][oldest_idx].t_stp=0;
    Cache[group][oldest_idx].tag=tag;
    return;
}

int main(int argc,char* argv[])
{
    int s,E,b;
    for(int i=1;i<argc;i++){
    	if(argv[i][0]=='-'){
	    switch(argv[i][1]){
	    	case 's':
		i++;
		s=char2int(argv[i][0]);
		break;
		case 'E':
		i++;
		E=char2int(argv[i][0]);
		break;
		case 'b':
		i++;
		b=char2int(argv[i][0]);
		break;
		case  't':
                path=argv[++i];
                break;
		default:break;
	    }
	}
    }
    InitCache(1<<s,E);
    FILE *fp = fopen(path, "r");
    char operate;
    uint64_t address;
    int size;
    while (fscanf(fp, " %c %lx,%d", &operate, &address, &size) == 3) {  // 每行读取3个参数
        switch(operate){
	    case 'L':case 'S':
	    update(address,s,E,b);
	    break;
	    case 'M':
	    update(address,s,E,b);
	    update(address,s,E,b);
	    break;
	    default:break;
	}
	    updateTimeStamp(1<<s,E);
    }
    fclose(fp);
    printSummary(Hits, Misses, Evits);
    freeCache(1<<s);
    return 0;
}


