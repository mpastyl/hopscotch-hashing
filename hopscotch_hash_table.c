#include<stdio.h>
#include<stdlib.h>

int BUSY=656565;

struct Bucket{

    int key;
    int data;
    int timestamp;
    unsigned int hop_info;
    int lock;

};


struct Bucket * B;

int size=512;
int HOP_RANGE = 32;
int ADD_RANGE = 256;


int contains(int);
void resize();
void find_closer_bucket(struct Bucket **,int *);


void lock(struct Bucket * bucket){

    
    while (1){
        if (!bucket->lock){
            if(!__sync_lock_test_and_set(&(bucket->lock),1)) break;
        }
    }
}

void unlock(struct Bucket * bucket){

    bucket->lock = 0;
}




int add(int key,int data,int reentrant){
    
    int hash=key%size;
    struct Bucket * start_bucket = &B[hash];
    if (reentrant) lock(start_bucket);
    
    if(contains(key)){
        if (reentrant) unlock(start_bucket);
        return 0;
    }

    struct Bucket * free_bucket = start_bucket;
    int free_distance =0;
    int temp_hash=hash;
    for(free_distance=0;free_distance<ADD_RANGE; ++free_distance){
        if((0==free_bucket->key) &&(__sync_bool_compare_and_swap(&(free_bucket->key),0,BUSY))) //danger
            break;
        temp_hash++;
        free_bucket=&B[temp_hash]; //danger
     }
    
    if( free_distance <ADD_RANGE){
        do{
            if  (free_distance <HOP_RANGE){
                start_bucket->hop_info |=(1<< free_distance);
                free_bucket->data = data;
                free_bucket->key = key;
                if(reentrant) unlock(start_bucket);
                return 1;
             }
             find_closer_bucket(&free_bucket,&free_distance);
         }while( NULL!= free_bucket);
    }
    if(reentrant)unlock(start_bucket);
    resize();
    return add(key,data,0);
}



void find_closer_bucket(struct Bucket **free_bucket, int * free_distance){


    struct Bucket * move_bucket = *free_bucket -(HOP_RANGE -1);
    if (move_bucket<(&B[0])) move_bucket=&B[0];
    int free_dist;
    for(free_dist = (HOP_RANGE -1); free_dist>0;--free_dist){
        unsigned start_hop_info = move_bucket->hop_info;
        int move_free_distance =-1;
        unsigned int mask =1;
        int i;
        for(i=0;i<free_dist; ++i,mask<<=1){
            if (mask &start_hop_info){
                move_free_distance=i;
                break;
            }
        }
        if (-1!=move_free_distance){
            lock(move_bucket);
            if (start_hop_info ==  move_bucket->hop_info){
                struct Bucket * new_free_bucket = move_bucket + move_free_distance;
                move_bucket->hop_info |= (1<<free_dist);
                (*free_bucket)->data =new_free_bucket->data;
                (*free_bucket)->key = new_free_bucket->key;
                ++(move_bucket->timestamp);
                new_free_bucket->key = BUSY;
                new_free_bucket->data = BUSY;
                move_bucket->hop_info &=~(1<< move_free_distance);
                *free_bucket = new_free_bucket;
                *free_distance -=free_dist;
                unlock(move_bucket);
                return;
            }
            unlock(move_bucket);
        }
        move_bucket++;
    }
    *free_bucket = NULL; 
    *free_distance =0;


}


void resize(){
    //TODO: fill
    printf(" @resize \n");
}

int contains(int key){
    
    int hash=key%size;
    struct Bucket * start_bucket  = &B[hash];

    int try_counter = 0;
    int timestamp;

    //TODO:include the fast path too

    int i;
    int temp_hash=hash;
    struct Bucket * check_bucket = start_bucket;
    for(i=0;i<HOP_RANGE;++i){
        
        if(key == (check_bucket->key))
            return 1;
        temp_hash++;
        check_bucket=&B[temp_hash];
    }
    return 0;
}


int main(int argc, char * argv[]){


    B=(struct Bucket *)malloc(sizeof(struct Bucket)*2048);
    
    //add(12,45,1);
    //add(13,47,1);
    //add(512+12,49,1);
    int i;
    for(i=12;i<48;i++) add(i,56,1);
    add(512+12,49,1);
    printf("%d\n",B[12].key);
    printf("%d\n",contains(12));
    printf("%d\n",contains(13));
    printf("%d\n",contains(14));
    printf("%d\n",contains(512+12));
    for(i=12;i<50;i++) printf("i = %d value %d\n",i,B[i].key);
    return 1;
}
