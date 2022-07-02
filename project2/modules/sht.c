#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "SHT.h"
#include "BF.h"


////////////////////////////////////////////////////////////////////////////////////////
//Helpful Functions
////////////////////////////////////////////////////////////////////////////////////////

/*Get the number of buckets that can fit in a hash block
Used in SHT_CreateSecondaryIndex,SHT_SecondaryInsertEntry,SHT_SecondaryGetAllEntries. */
int sht_get_buckets_per_block(int buckets)
{
    //this can range between 1 to 63 - the higher the less hash blocks - change if problem with readblock
    int buckets_per_block=63;  //maximum number of buckets in a block
    return buckets_per_block;
}

/*Get number of blocks to use for hash mapping
Used in HT_CreateIndex. */
int sht_get_hash_blocks(int buckets)
{
    return ceil((double)(buckets)/(sht_get_buckets_per_block(buckets)));
}

/*Used in SHT_SecondaryInsertEntry,SHT_SecondaryGetAllEntries */
int hash_fun_str(char* key, int num_buckets)
{
    int sum=0;
    size_t key_length=strlen(key);
    for (size_t i=0; i<key_length; i++)
        sum+=key[i];
    int hash=sum%num_buckets;
    return hash;
}

/*Finds and returns the appropriate hash block
Used in SHT_SecondaryInsertEntry,SHT_SecondaryGetAllEntries */
int* sht_find_hash_block(int hash_block_number, int fileDesc)
{
    int check;
    void* block;
    //we have the hash block number, now we need to find the actual hash block by 1) reading block 0,
    // 2) getting the first hash block and 3) use the next_hash_block to get to the appropriate hash block
    check=BF_ReadBlock(fileDesc,0,&block);
    if (check<0)
    {
        BF_PrintError("Error in sht_find_hash_block-BF_ReadBlock");
        return NULL;
    }
    int* hash_block=block+BLOCK_SIZE-sizeof(int); //initially first hash block
    int i=1;
    //looping until hash_block points to our wanted hash block
    while (i < hash_block_number)
    {
        check=BF_ReadBlock(fileDesc,*hash_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in sht_find_hash_block-BF_ReadBlock");
            return NULL;
        }
        hash_block=block+BLOCK_SIZE-sizeof(int);
        i++;
    }
    return hash_block;
}

/*Checks for duplicates of surname-blockId in SHT_SecondaryInsertEntry */
int find_duplicate(char* surname,int blockId,int fileDesc,int block_number)
{
    int check;
    void* block;
    int *next_block;
    int num_of_surnames;
    do
    {   
        check=BF_ReadBlock(fileDesc,block_number,&block);
        if (check<0)
        {
            BF_PrintError("Error in find_entry-BF_ReadBlock");
            return -1;
        }

        //get number of records and next_block pointer of read block
        memcpy(&(num_of_surnames),block+BLOCK_SIZE-2*sizeof(int),sizeof(int));
        next_block=block+BLOCK_SIZE-sizeof(int);

        for (int i = 0; i < num_of_surnames; i++)
        {
            void* block_surname=(block+i*(25*sizeof(char)+sizeof(int)));
            void* block_blockId=(block+25*sizeof(char)+i*(sizeof(int)+25*sizeof(char)));
            if ((strcmp(surname,(char*)block_surname)==0) && (blockId==*(int*)block_blockId))
            {
                return -1;
            }
        }
        block_number=*(int*)next_block;
    }while(*next_block!=-1);
    
    return 0;
}


int sht_record_counter=0;    //keeps track of the number of records - used in statistics
////////////////////////////////////////////////////////////////////////////////////////
//SHT_CreateSecondaryIndex
////////////////////////////////////////////////////////////////////////////////////////

int SHT_CreateSecondaryIndex(char *sfileName,char* attrName,int attrLength,int buckets,char* fileName)
{
    int check;
    int fileDesc;
    void* block;
    
    check=BF_CreateFile(sfileName);  //create a file of blocks named "sfilename"
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_CreateFile");
        return -1;
    }

    check=BF_OpenFile(sfileName);    //opens the file named "sfilename"
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_OpenFile");
        return -1;
    }
    else
        fileDesc=check;

    check=BF_AllocateBlock(fileDesc);   //allocates the first block of this file, block 0
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_AllocateBlock");
        return -1;
    }

    check=BF_GetBlockCounter(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_GetBlockCounter");
        return -1;
    }
    int blockNumber=check-1; //block 0 because first block of file

    check=BF_ReadBlock(fileDesc,blockNumber,&block); //read first block (block 0) of this file
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_ReadBlock");
        return -1;
    }
    else    //write information in block 0
    {
        //write 'SHASH' in first bytes of block 0
        memcpy(block,"SHASH",sizeof("SHASH"));

        //write the attrName in 'start of block'+sizeof("SHASH")
        memcpy(block+sizeof("SHASH"),attrName,20*sizeof(char));

        //write the attrLength in 'start of block'+sizeof("SHASH")+20*sizeof(char)
        memcpy(block+sizeof("SHASH")+20*sizeof(char),&attrLength,sizeof(int));

        //write the bucket number in 'start of block'+sizeof("SHASH")+20*sizeof(char)+sizeof(int)
        memcpy(block+sizeof("SHASH")+20*sizeof(char)+sizeof(int),&buckets,sizeof(int));

        //write the fileName in 'start of block'+sizeof("SHASH")+20*sizeof(char)+sizeof(int)+sizeof(int)
        memcpy(block+sizeof("SHASH")+20*sizeof(char)+sizeof(int)+sizeof(int),fileName,20*sizeof(char));
    }


    //Now let's allocate the very first hash block and store it's number in block 0
    //By 'hash blocks' we mean the blocks that map each key to the right bucket
    check=BF_AllocateBlock(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_AllocateBlock");
        return -1;
    }

    check=BF_GetBlockCounter(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_GetBlockCounter");
        return -1;
    }
    int last_block=check-1;


    //read block 0 again, and write first hash block's number
    check=BF_ReadBlock(fileDesc,blockNumber,&block);
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_ReadBlock");
        return -1;
    }
    memcpy(block+BLOCK_SIZE-sizeof(int),&(last_block),sizeof(int));

    check=BF_WriteBlock(fileDesc,blockNumber);  //write block 0 to disc
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_WriteBlock");
        return -1;
    }


    //Now let's allocate - initialize the rest hash blocks
    int buckets_per_block=sht_get_buckets_per_block(buckets);
    int i;  //will count blocks
    int block_pos;  //initialization - this will count initialised positions inside one block
    int pos=0;  //initialization - this will count whether all needed positions of the needed hash blocks are initialised
    //initialization - value = -1 because we have not yet entered any records so no buckets have been used and no
    //overflow blocks.
    int init_value = -1;

    //at this moment we have allocated one hash block, so begin initialising and allocating a new hash block if needed
    for (i=1; i <= sht_get_hash_blocks(buckets); i++)
    {
        check=BF_GetBlockCounter(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_GetBlockCounter");
            return -1;
        }
        int cur_block=check-1;

        check=BF_ReadBlock(fileDesc,cur_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_ReadBlock");
            return -1;
        }
        //this will be used later to update the next hash block number of the current block
        int* next_hash_block=block+BLOCK_SIZE-sizeof(int);

        //initialise all available positions of the current hash block to -1
        //reminder: 2*buckets means all needed initialisations (bucket and last overflow block)
        for (block_pos = 0; block_pos < 2*buckets_per_block; block_pos++)
        {
            //if no more positions available in current hash block, or if we are done with all hash_blocks, break
            if ((block_pos == 2*buckets_per_block) || (pos == 2*buckets))
            {
                break;
            }
            memcpy(block+block_pos*sizeof(int),&(init_value),sizeof(int));
            pos++;
        }
    
        //if this is the last hash block, next_block 'pointer' is -1
        if (i==sht_get_hash_blocks(buckets))
        {
            *next_hash_block=-1;
            check=BF_WriteBlock(fileDesc,cur_block);
            if (check<0)
            {
                BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_WriteBlock");
                return -1;
            }
            break;
        }

        //if needed allocate next block
        check=BF_AllocateBlock(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_AllocateBlock");
            return -1;
        }

        check=BF_GetBlockCounter(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_GetBlockCounter");
            return -1;
        }
        int next_block=check-1;

        check=BF_ReadBlock(fileDesc,next_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_ReadBlock");
            return -1;
        }
 
        //write to the current block (the previous) the number of the just allocated block
        memcpy(next_hash_block,&(next_block),sizeof(int));

        //current block finished
        check=BF_WriteBlock(fileDesc,cur_block);
        if (check<0)
        {
            BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_WriteBlock");
            return -1;
        }
    }
    check=BF_CloseFile(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in SHT_CreateSecondaryIndex-BF_CloseFile");
        return -1;
    }
    
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
//SHT_OpenSecondaryIndex
////////////////////////////////////////////////////////////////////////////////////////

SHT_info* SHT_OpenSecondaryIndex(char *sfileName)
{
    int check;
    void* block;
    int fileDesc;
    

    check=BF_OpenFile(sfileName);
    if (check<0)
    {
        BF_PrintError("Error in SHT_OpenSecondaryIndex-BF_OpenFile");
        return NULL;
    }
    else
        fileDesc=check;

    //read block 0 and check if it has the "SHASH" information
    check=BF_ReadBlock(fileDesc,0,&block);
    if (check<0)
    {
        BF_PrintError("Error in SHT_OpenSecondaryIndex-BF_ReadBlock");
        return NULL;
    }
    else if (strcmp(block,"SHASH")!=0)
    {
        return NULL;
    }

    //create a struct of secondary hash table-key information
    SHT_info *shash_info;
    shash_info = (SHT_info*) malloc(sizeof(SHT_info));
    shash_info->attrName = malloc(20*sizeof(char));
    shash_info->fileName = malloc(20*sizeof(char));
    
    shash_info->fileDesc=fileDesc;                                     
    strcpy(shash_info->attrName,(block+sizeof("SHASH")));  
    int* attrLengthptr=block+sizeof("SHASH")+20*sizeof(char);                     
    shash_info->attrLength=*attrLengthptr;
    int *bucketsptr=block+sizeof("SHASH")+20*sizeof(char)+sizeof(int);
    shash_info->numBuckets=*bucketsptr;
    strcpy(shash_info->fileName,(block+sizeof("SHASH")+20*sizeof(char)+sizeof(int)+sizeof(int)));
    return shash_info;
}


////////////////////////////////////////////////////////////////////////////////////////
//SHT_CloseSecondaryIndex
////////////////////////////////////////////////////////////////////////////////////////

int SHT_CloseSecondaryIndex(SHT_info* header_info)
{
    int check;
    int fileDesc;

    fileDesc=header_info->fileDesc;
    check=BF_CloseFile(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in SHT_CloseSecondaryIndex-BF_CloseFile");
        return -1;
    }
    //free allocated space from HT_OpenIndex
    free(header_info->attrName);
    free(header_info->fileName);
    free(header_info);
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
//SHT_SecondaryInsertEntry
////////////////////////////////////////////////////////////////////////////////////////

int SHT_SecondaryInsertEntry(SHT_info header_info,SecondaryRecord record)
{
    int check;
    void* block;
    int fileDesc=header_info.fileDesc;
    int num_surnames_limit=(BLOCK_SIZE-2*sizeof(int))/(25*sizeof(char)+sizeof(int));
    
    int buckets_per_block=sht_get_buckets_per_block(header_info.numBuckets);
    int hash_value = hash_fun_str(record.record.surname,header_info.numBuckets);   //using the hash function
    //hash block's number in order
    int hash_block_number=hash_value/buckets_per_block+1;
    //the hash block itself
    int* hash_block=sht_find_hash_block(hash_block_number,fileDesc);
    //the position of the bucket's number in the hash block we got. (*2 because of the pointer to last overflow block)
    int index_pos=(hash_value%buckets_per_block)*2;

    check=BF_ReadBlock(fileDesc,*hash_block,&block);
    if (check<0)
    {
        BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_ReadBlock");
        return -1;
    }
    int *bucket_value=block+index_pos*sizeof(int);  //point to the bucket/block value
    int *last_overflow_block=block+index_pos*sizeof(int)+sizeof(int);  //point to the last overflow block value
    int next_block; //will be used as the 'pointer' to the next block
    int num_of_surnames;
    if(*bucket_value==-1)  //first time hashing here, need to allocate bucket
    {
        
        check=BF_AllocateBlock(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_AllocateBlock");
            return -1;
        }

        check=BF_GetBlockCounter(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_GetBlockCounter");
            return -1;
        }
        int last_block=check-1; //last block - just allocated bucket
        check=BF_ReadBlock(fileDesc,last_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_ReadBlock");
            return -1;
        }

        //update hash table
        memcpy(bucket_value,&last_block,sizeof(int));
        memcpy(last_overflow_block,&last_block,sizeof(int));
        check=BF_WriteBlock(fileDesc,*hash_block);
        if (check<0)
        {
            BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_WriteBlock");
            return -1;
        }

        //write surname-blockId to the bucket
        memcpy(block,&record.record.surname,25*sizeof(char));
        memcpy(block+25*sizeof(char),&record.blockId,sizeof(int));
        sht_record_counter++;

        //write the number of surnames to the 'previous of the last' position in the block
        num_of_surnames=1;
        memcpy(block+BLOCK_SIZE-2*sizeof(int),&(num_of_surnames),sizeof(int));

        //next block does not exist for now
        next_block=-1;
        memcpy(block+BLOCK_SIZE-sizeof(int),&(next_block),sizeof(int));

        check=BF_WriteBlock(fileDesc,last_block);
        if (check<0)
        {
            BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_WriteBlock");
            return -1;
        }
        return 0;
    }
    else    //if(*bucket_value!=-1) - hashing here again
    {
        //if the pair surname/blockId already exists - no need to insert again
        int block_number=*bucket_value;  //iterate block chain

        if (find_duplicate(record.record.surname,record.blockId,fileDesc,block_number)==-1)
        {
            return -1;
        }
 
        //2 options:
        //either we insert to the last overflow block or we allocate a new one
        //either way we'll use the last overflow block, so read it
        check=BF_ReadBlock(fileDesc,*last_overflow_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_ReadBlock");
            return -1;
        }
        memcpy(&num_of_surnames,block+BLOCK_SIZE-2*sizeof(int),sizeof(int));
        
        if (num_of_surnames==num_surnames_limit)  //no space left
        {
            
            check=BF_AllocateBlock(fileDesc);
            if (check<0)
            {
                BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_AllocateBlock");
                return -1;
            }
            check=BF_GetBlockCounter(fileDesc);
            if (check<0)
            {
                BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_GetBlockCounter");
                return -1;
            }
            int last_block=check-1;

            //at this time block points to the last overflow block
            //we want to update its next_block_pointer to the just allocated block (last_block)
            memcpy(block+BLOCK_SIZE-sizeof(int),&last_block,sizeof(int));
            check=BF_WriteBlock(fileDesc,*last_overflow_block);
            if (check<0)
            {
                BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_WriteBlock");
                return -1;
            }

            //the just allocated block
            check=BF_ReadBlock(fileDesc,last_block,&block);
            if (check<0)
            {
                BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_ReadBlock");
                return -1;
            }


            //update hash table
            //last_overflow_block becomes the just allocated block
            memcpy(last_overflow_block,&last_block,sizeof(int));
            check=BF_WriteBlock(fileDesc,*hash_block);
            if (check<0)
            {
                BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_WriteBlock");
                return -1;
            }

            
            //write the surname-blockId to the block
            memcpy(block,&record.record.surname,25*sizeof(char));
            memcpy(block+25*sizeof(char),&record.blockId,sizeof(int));
            sht_record_counter++;

            //write the number of surnames
            num_of_surnames=1;
            memcpy(block+BLOCK_SIZE-2*sizeof(int),&(num_of_surnames),sizeof(int));

            //next block not exists for now
            next_block=-1;
            memcpy(block+BLOCK_SIZE-sizeof(int),&(next_block),sizeof(int));

            check=BF_WriteBlock(fileDesc,last_block);
            if (check<0)
            {
                BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_WriteBlock");
                return -1;
            }
            return 0;
        }
        else if (num_of_surnames<num_surnames_limit)   //we have space - insert in last_overflow_block
        {
            memcpy(block+num_of_surnames*(25*sizeof(char)+sizeof(int)),&record.record.surname,25*sizeof(char));
            memcpy(block+25*sizeof(char)+num_of_surnames*(sizeof(int)+25*sizeof(char)),&record.blockId,sizeof(int));
            sht_record_counter++;

            num_of_surnames++;
            memcpy(block+BLOCK_SIZE-2*sizeof(int),&(num_of_surnames),sizeof(int));
            check=BF_WriteBlock(fileDesc,*last_overflow_block);
            if (check<0)
            {
                BF_PrintError("Error in SHT_SecondaryInsertEntry-BF_WriteBlock");
                return -1;
            }
            return 0;
        }
    }
    return -1;  //if program reach this point, insert failed
}


////////////////////////////////////////////////////////////////////////////////////////
//SHT_SecondaryGetAllEntries
////////////////////////////////////////////////////////////////////////////////////////

int SHT_SecondaryGetAllEntries(SHT_info header_info_sht,HT_info header_info_ht,void *value)
{
    int number_of_blocks=0;
    int check;
    void* block;
    int sht_fileDesc=header_info_sht.fileDesc;
    int ht_fileDesc=header_info_ht.fileDesc;
    
    int hash_value = hash_fun_str(value,header_info_sht.numBuckets);   //using the hash function
    int buckets_per_block=sht_get_buckets_per_block(header_info_sht.numBuckets);
    int hash_block_number=hash_value/buckets_per_block+1;
    int* hash_block=sht_find_hash_block(hash_block_number,sht_fileDesc);
    int index_pos=(hash_value%buckets_per_block)*2;

    check=BF_ReadBlock(sht_fileDesc,*hash_block,&block);
    if (check<0)
    {
        BF_PrintError("Error in SHT_SecondaryGetAllEntries-BF_ReadBlock");
        return -1;
    }
    number_of_blocks++;

    int *bucket_value=block+index_pos*sizeof(int);  //point to the bucket value
    if (*bucket_value==-1)
    {
        printf("Record with %s : %s was not found.\n",header_info_sht.attrName,(char*)value);
        return -1;
    }
    
    int block_number=*bucket_value;  //iterate block chain
    int *next_block;
    int num_of_surnames;
    int value_exists=0;
    do
    {   
        check=BF_ReadBlock(sht_fileDesc,block_number,&block);
        if (check<0)
        {
            BF_PrintError("Error in SHT_SecondaryGetAllEntries-BF_ReadBlock");
            return -1;
        }
        number_of_blocks++;
        //get number of records and next_block pointer of read block
        memcpy(&(num_of_surnames),block+BLOCK_SIZE-2*sizeof(int),sizeof(int));
        next_block=block+BLOCK_SIZE-sizeof(int);

        for (int i = 0; i < num_of_surnames; i++)
        {
            void* block_surname=(block+i*(25*sizeof(char)+sizeof(int)));
            void* block_blockId=(block+25*sizeof(char)+i*(sizeof(int)+25*sizeof(char)));

            if ((strcmp((char*)value,(char*)block_surname)==0))
            {
                int blockId=*(int*)block_blockId;
                void* ht_block;
                check=BF_ReadBlock(ht_fileDesc,blockId,&ht_block);
                if (check<0)
                {
                    BF_PrintError("Error in SHT_SecondaryGetAllEntries-BF_ReadBlock");
                    return -1;
                }
                number_of_blocks++;
                //get number of records of read block
                int num_of_records;
                memcpy(&(num_of_records),ht_block+BLOCK_SIZE-2*sizeof(int),sizeof(int));
                for (int j = 0; j < num_of_records; j++)
                {
                    void* block_surname=(ht_block+sizeof(int)+15*sizeof(char)+j*sizeof(Record));
                    if ((strcmp((char*)value,(char*)block_surname)==0))
                    {
                        Record record;
                        memcpy(&record,ht_block+j*sizeof(Record),sizeof(Record));
                        printf("%d %s %s %s\n",record.id, record.name, record.surname, record.address);
                        value_exists=1;
                    }   
                }  
            } 
        }
        block_number=*next_block;
    }while(*next_block!=-1);
    if (value_exists==0)
    {
        printf("-%s : %s does not exist.\n",header_info_sht.attrName,(char*)value);
        return -1;
    }

    return number_of_blocks;
}


////////////////////////////////////////////////////////////////////////////////////////
//HashStatistics
////////////////////////////////////////////////////////////////////////////////////////

/*Can be used by primary or secondary hash table */
int HashStatistics(char* filename)
{
    SHT_info *shashinfo;
    HT_info *hashinfo;
    char hash_kind='p';    //'p' is for primary hash table
    hashinfo=HT_OpenIndex(filename);    //try to open filename as primary hash table
    if (hashinfo==NULL)    //if not ht,
    {
        hash_kind='s';  //'s' is for secondary hash table
        shashinfo=SHT_OpenSecondaryIndex(filename); //try to open filename as secondary hash table
        
        if (shashinfo==NULL) //if not sht return.
        {
            printf("Not a hash table.\n");
            return -1;
        }
    }
    
    int num_records_limit=(BLOCK_SIZE-2*sizeof(int))/sizeof(Record);    //records fit in a block
    int check;
    int fileDesc;
    int record_counter;
    int num_of_buckets;
    int num_hash_blocks;
    int buckets_per_block;
    if (hash_kind=='s')
    {
        fileDesc=shashinfo->fileDesc;
        record_counter=sht_record_counter;
        num_of_buckets=shashinfo->numBuckets;
        num_hash_blocks=sht_get_hash_blocks(num_of_buckets);
        buckets_per_block=sht_get_buckets_per_block(num_of_buckets);
    }
    else if (hash_kind=='p')
    {
        fileDesc=hashinfo->fileDesc;
        record_counter=ht_record_counter;
        num_of_buckets=hashinfo->numBuckets;
        num_hash_blocks=get_hash_blocks(num_of_buckets);
        buckets_per_block=get_buckets_per_block(num_of_buckets);
    }
    
    int num_of_blocks=BF_GetBlockCounter(fileDesc);
    if (num_of_blocks<0)
    {
        BF_PrintError("Error in HashStatistics-BF_GetBlockCounter");
        return -1;
    }

    printf("1) File %s has %d blocks and %d records.\n",filename,num_of_blocks,record_counter);

    /*Minimum/average/maximum number of records per bucket*/
    int min_records=INT_MAX,max_records=INT_MIN;
    float avg_records=(float)record_counter/num_of_buckets;
    
    //read block 0 to get the first hash block
    void* ptr_block0;
    check=BF_ReadBlock(fileDesc,0,&ptr_block0);
    if (check<0)
    {
        BF_PrintError("Error in 1HashStatistics-BF_ReadBlock");
        return -1;
    }
    int hash_block_number=*(int*)(ptr_block0+BLOCK_SIZE-sizeof(int));   //first hash block
    
    void* ptr_hash_block;
    //iterate hash blocks
    int blocks=0;   //counts blocks per bucket
    int ovfl_buckets=0; //counts buckets with overflow blocks,
    //as overflow blocks we consider all blocks after the first block in a bucket
    for (int i = 1; i <= num_hash_blocks; i++)
    {
        check=BF_ReadBlock(fileDesc,hash_block_number,&ptr_hash_block);
        if (check<0)
        {
            BF_PrintError("Error in 2HashStatistics-BF_ReadBlock");
            return -1;
        }
        hash_block_number=*(int*)(ptr_hash_block+BLOCK_SIZE-sizeof(int));
        //iterate buckets  
        for (int j = 0; j < 2*(buckets_per_block); j=j+2)
        {  
            int sum_records=0;
            int bucket_number=*(int*)(ptr_hash_block+j*sizeof(int));   
            int last_overflow_block=*(int*)(ptr_hash_block+(j+1)*sizeof(int));
            if ((bucket_number>0) && (bucket_number<num_of_blocks))
            {   
                //iterate blocks
                void* ptr_block;
                int block_number=bucket_number;
                do
                {
                    blocks++;
                    check=BF_ReadBlock(fileDesc,block_number,&ptr_block);
                    if (check<0)
                    {
                        BF_PrintError("Error in 3HashStatistics-BF_ReadBlock");
                        return -1;
                    }
                    int num_of_records=*(int*)(ptr_block+BLOCK_SIZE-2*sizeof(int));
                    sum_records+=num_of_records;

                if (block_number==last_overflow_block)
                    break;
                block_number=*(int*)(ptr_block+BLOCK_SIZE-sizeof(int));
                }while(1);
                if (min_records>sum_records)
                    min_records=sum_records;
                if (max_records<sum_records)
                    max_records=sum_records;
            }
            else
            {
                break;
            }
            //if records in current bucket more than the limit we have overflow blocks
            if (sum_records>num_records_limit)
            {
                ovfl_buckets++;
            }
        }
        
    }
    printf("2) Minimum number of records per bucket is %d.\n"   
    "   Average number of records per bucket is %.2f.\n"   
    "   Maximum number of records per bucket is %d.\n",min_records,avg_records,max_records);

    /*Average number of blocks per bucket*/
    float avg_blocks=(float)blocks/num_of_buckets;
    printf("3) Average number of blocks per bucket is %.2f.\n",avg_blocks);
    
    /*Number of buckets with overflow blocks and number of overflow blocks per bucket*/
    printf("4) Number of buckets with overflow blocks is %d.\n",ovfl_buckets);


    //looping again to get number of overflow-blocks per overflow-bucket
    check=BF_ReadBlock(fileDesc,0,&ptr_block0);
    if (check<0)
    {
        BF_PrintError("Error in 1HashStatistics-BF_ReadBlock");
        return -1;
    }
    hash_block_number=*(int*)(ptr_block0+BLOCK_SIZE-sizeof(int));   //first hash block

    //iterate hash blocks
    for (int i = 1; i <= num_hash_blocks; i++)
    {
        check=BF_ReadBlock(fileDesc,hash_block_number,&ptr_hash_block);
        if (check<0)
        {
            BF_PrintError("Error in 2HashStatistics-BF_ReadBlock");
            return -1;
        }
        hash_block_number=*(int*)(ptr_hash_block+BLOCK_SIZE-sizeof(int));
        //iterate buckets  
        for (int j = 0; j < 2*(buckets_per_block); j=j+2)
        {  
            int sum_records=0;
            int bucket_number=*(int*)(ptr_hash_block+j*sizeof(int));   
            int last_overflow_block=*(int*)(ptr_hash_block+(j+1)*sizeof(int));
            if ((bucket_number>0) && (bucket_number<num_of_blocks))
            {   
                //iterate blocks
                void* ptr_block;
                int block_number=bucket_number;
                do
                {
                    blocks++;
                    check=BF_ReadBlock(fileDesc,block_number,&ptr_block);
                    if (check<0)
                    {
                        BF_PrintError("Error in 3HashStatistics-BF_ReadBlock");
                        return -1;
                    }
                    int num_of_records=*(int*)(ptr_block+BLOCK_SIZE-2*sizeof(int));
                    sum_records+=num_of_records;

                if (block_number==last_overflow_block)
                    break;
                block_number=*(int*)(ptr_block+BLOCK_SIZE-sizeof(int));
                }while(1);
                if (min_records>sum_records)
                    min_records=sum_records;
                if (max_records<sum_records)
                    max_records=sum_records;
            }
            else
            {
                break;
            }
            //if records in current bucket more than the limit we have overflow blocks
            if (sum_records>num_records_limit)
            {
                int num_ovfl_blocks=sum_records/num_records_limit;
                printf("   Bucket with block number %d has %d overflow-blocks.\n",bucket_number,num_ovfl_blocks);
            }
        }   
    }
    if (hash_kind=='s')
    {
        SHT_CloseSecondaryIndex(shashinfo);
    }
    else if (hash_kind=='p')
    {
        HT_CloseIndex(hashinfo);
    }
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////