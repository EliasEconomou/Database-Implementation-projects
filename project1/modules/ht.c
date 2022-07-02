#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "HT.h"
#include "BF.h"


////////////////////////////////////////////////////////////////////////////////////////
//Helpful Functions
////////////////////////////////////////////////////////////////////////////////////////

/*Get the number of buckets that can fit in a hash block
Used in all functions.*/
int get_buckets_per_block(int buckets)
{
    /*We need blocks that will contain indices to buckets and to the last overflow blocks - let's call them hash blocks.
    Hash blocks will have 1 'pointer' (sizeof(int)) to the next hash block. 
    Substracting this pointer plus a space****, it leaves us with '(BLOCK_SIZE-2*sizeof(int))/sizeof(int)' positions.*/
    
    //****I changed this because i could not use readblock for more than 20 buckets when iterating to get all records,
    //****so now avail_positions is not used, instead buckets per block are always 20
    //****Uncomment bellow and use avail_positions to calculate buckets_per_block if you want

    //int avail_positions=(BLOCK_SIZE-2*sizeof(int))/sizeof(int); //number of available positions for buckets and last overflow blocks
    int buckets_per_block=1;  //maximum number of buckets in a block
    return buckets_per_block;
}

/*Get number of blocks to use for hash mapping
Used in HT_CreateIndex. */
int get_hash_blocks(int buckets)
{
    /*the number of blocks we'll need will be the ceil of (buckets)/(buckets_per_block),
    Example: 
    126 positions, 64 bucket number given, and 126/2=63 buckets per block : 64/63=1.015 and ceil(1.015)=2*/
    return ceil((double)(buckets)/(get_buckets_per_block(buckets)));
}

/*Searches for a record and returns a pointer to it while also 1) updating the 'block_number' of the block
that has the record and 2) counts the 'number_of_blocks read' to find it. If it doesn't find the record returns NULL.
Used in HT_InsertEntry, HT GetAllEntries and HT_DeleteEntry.*/
void* find_entry(int value, int* block_number, int fileDesc, int* number_of_blocks)
{
    int check;
    void* block;
    int *next_block;
    int num_of_records;
    do
    {   
        check=BF_ReadBlock(fileDesc,*block_number,&block);
        if (check<0)
        {
            BF_PrintError("Error in find_entry-BF_ReadBlock");
            return NULL;
        }
        (*number_of_blocks)++;
        //get number of records and next_block pointer of read block
        memcpy(&(num_of_records),block+BLOCK_SIZE-2*sizeof(int),sizeof(int));
        next_block=block+BLOCK_SIZE-sizeof(int);

        for (int i = 0; i < num_of_records; i++)
        {
            void* block_record=(block+i*sizeof(Record));
            if (value==*(int*)block_record)
            {
                return block_record;
            }   
        }
        block_number=next_block;
    }while(*next_block!=-1);
    
    return NULL;
}

/*Prints all entries by passing 'ALL' in HT_GetAllEntries.*/
int print_all_entries(HT_info header_info)
{
    int check;
    int fileDesc=header_info.fileDesc;

    int num_of_blocks=BF_GetBlockCounter(fileDesc);
    if (num_of_blocks<0)
    {
        BF_PrintError("Error in HashStatistics-BF_GetBlockCounter");
        return -1;
    }
    int num_of_buckets=header_info.numBuckets;
    //read block 0 to get the first hash block
    void* ptr_block0;
    check=BF_ReadBlock(fileDesc,0,&ptr_block0);
    if (check<0)
    {
        BF_PrintError("Error in print_all_entries-BF_ReadBlock");
        return -1;
    }
    int hash_block_number=*(int*)(ptr_block0+BLOCK_SIZE-sizeof(int));   //first hash block
    
    //get number of hash_blocks
    int num_hash_blocks=get_hash_blocks(num_of_buckets);
    void* ptr_hash_block;
    //iterate hash blocks
    for (int i = 1; i <= num_hash_blocks; i++)
    {
        check=BF_ReadBlock(fileDesc,hash_block_number,&ptr_hash_block);
        if (check<0)
        {
            BF_PrintError("Error in print_all_entries-BF_ReadBlock");
            return -1;
        }
        hash_block_number=*(int*)(ptr_hash_block+BLOCK_SIZE-sizeof(int));
        int buckets_per_block=get_buckets_per_block(num_of_buckets);
        //iterate buckets
        for (int j = 0; j < 2*(buckets_per_block); j=j+2)
        {
            int bucket_number=*(int*)(ptr_hash_block+j*sizeof(int));
            int last_overflow_block=*(int*)(ptr_hash_block+(j+1)*sizeof(int));
            if ((bucket_number<=0) || (bucket_number>=num_of_blocks))
            {
                break;
            }
            else if ((bucket_number>0) && (bucket_number<num_of_blocks))
            {
                //iterate blocks
                void* ptr_block;
                int block_number=bucket_number;
                do
                {
                    check=BF_ReadBlock(fileDesc,block_number,&ptr_block);
                    if (check<0)
                    {
                        BF_PrintError("Error in print_all_entries-BF_ReadBlock");
                        return -1;
                    }
                    int num_of_records=*(int*)(ptr_block+BLOCK_SIZE-2*sizeof(int));
                    //iterate records
                    for (int k = 0; k < num_of_records; k++)
                    {
                        Record record;
                        memcpy(&record,ptr_block+k*sizeof(Record),sizeof(Record));
                        printf("%d %s %s %s\n",record.id, record.name, record.surname, record.address);
                    }

                if (block_number==last_overflow_block)
                {
                    break;
                }
                block_number=*(int*)(ptr_block+BLOCK_SIZE-sizeof(int));
                }while(1);
            }
        }
    }
    return 0;
}

/*Used in HT_Insert_entry, HT GetAllEntries and HT_DeleteEntry.*/
int hash_fun(int key, int num_buckets)
{
    return key%num_buckets;
}

/*Finds and returns the appropriate hash block
Used in HT_InsertEntry,HT_DeleteEntry,HT_GetAllEntries*/
int* find_hash_block(int hash_block_number, int fileDesc)
{
    int check;
    void* block;
    //we have the hash block number, now we need to find the actual hash block by 1) reading block 0,
    // 2) getting the first hash block and 3) use the next_hash_block to get to the appropriate hash block
    check=BF_ReadBlock(fileDesc,0,&block);
    if (check<0)
    {
        BF_PrintError("Error in HT_InsertEntry-BF_ReadBlock");
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
            BF_PrintError("Error in find_hash_block-BF_ReadBlock");
            return NULL;
        }
        hash_block=block+BLOCK_SIZE-sizeof(int);
        i++;
    }
    return hash_block;
}



static int record_counter=0;    //keeps track of the number of records - used in statistics
////////////////////////////////////////////////////////////////////////////////////////
//HT_CreateIndex
////////////////////////////////////////////////////////////////////////////////////////

int HT_CreateIndex(char *fileName,char attrType,char* attrName,int attrLength,int buckets)
{
  
    int check;                  //check takes the return values of BF functions and checks for errors
    int fileDesc;               //file description number
    int blockNumber=0;          //read block 0
    void* block;                //access the read block   

    BF_Init();
    check=BF_CreateFile(fileName);  //create a file of blocks named "filename"
    if (check<0)
    {
        BF_PrintError("Error in HT_CreateIndex-BF_CreateFile");
        return -1;
    }
 
    check=BF_OpenFile(fileName);    //opens the file named "filename"
    if (check<0)
    {
        BF_PrintError("Error in HT_CreateIndex-BF_OpenFile");
        return -1;
    }
    else
        fileDesc=check;

    check=BF_AllocateBlock(fileDesc);   //allocates the first block of the file
    if (check<0)
    {
        BF_PrintError("Error in HT_CreateIndex-BF_AllocateBlock");
        return -1;
    }

    check=BF_ReadBlock(fileDesc,blockNumber,&block); //read block 0 (first block)
    if (check<0)
    {
        BF_PrintError("Error in HT_CreateIndex-BF_ReadBlock");
        return -1;
    }
    else    //write information in block 0
    {
        //write 'HASH' in first bytes of block 0
        memcpy(block,"HASH",sizeof("HASH"));

        //write the attrType in 'start of block'+sizeof("HASH")
        memcpy(block+sizeof("HASH"),&attrType,sizeof(char));

        //write the attrName in 'start of block'+sizeof("HASH")+sizeof(char)
        memcpy(block+sizeof("HASH")+sizeof(char),attrName,15*sizeof(char));

        //write the attrLength in 'start of block'+sizeof("HASH")+sizeof(char)+15*sizeof(char)
        memcpy(block+sizeof("HASH")+sizeof(char)+15*sizeof(char),&attrLength,sizeof(int));

        //write the bucket number in 'start of block'+sizeof("HASH")+sizeof(char)+15*sizeof(char)+sizeof(int)
        memcpy(block+sizeof("HASH")+sizeof(char)+15*sizeof(char)+sizeof(int),&buckets,sizeof(int));
    }

    
    //Now let's allocate the very first hash block and store it's number in block 0
    //By 'hash blocks' we mean the blocks that map each key to the right bucket
    check=BF_AllocateBlock(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HT_CreateIndex-BF_AllocateBlock");
        return -1;
    }

    check=BF_GetBlockCounter(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HT_InsertEntry-BF_GetBlockCounter");
        return -1;
    }
    int last_block=check-1;


    //read block 0 again, and write first hash block's number
    check=BF_ReadBlock(fileDesc,blockNumber,&block);
    if (check<0)
    {
        BF_PrintError("Error in HT_CreateIndex-BF_ReadBlock");
        return -1;
    }
    memcpy(block+BLOCK_SIZE-sizeof(int),&(last_block),sizeof(int));

    check=BF_WriteBlock(fileDesc,blockNumber);  //write block 0 to disc
    if (check<0)
    {
        BF_PrintError("Error in HT_CreateIndex-BF_WriteBlock");
        return -1;
    }
    

    //Now let's allocate - initialize the rest hash blocks
    int buckets_per_block=get_buckets_per_block(buckets);
    int i;  //will count blocks
    int block_pos;  //initialization - this will count initialised positions inside one block
    int pos=0;  //initialization - this will count whether all needed positions of the needed hash blocks are initialised
    //initialization - value = -1 because we have not yet entered any records so no buckets have been used and no
    //overflow blocks.
    int init_value = -1;

    //at this moment we have allocated one hash block, so begin initialising and allocating a new hash block if needed
    for (i=1; i <= get_hash_blocks(buckets); i++)
    {
        check=BF_GetBlockCounter(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in HT_InsertEntry-BF_GetBlockCounter");
            return -1;
        }
        int cur_block=check-1;

        check=BF_ReadBlock(fileDesc,cur_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in HT_CreateIndex-BF_ReadBlock");
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
        if (i==get_hash_blocks(buckets))
        {
            *next_hash_block=-1;
            check=BF_WriteBlock(fileDesc,cur_block);
            if (check<0)
            {
                BF_PrintError("Error in HT_CreateIndex-BF_WriteBlock");
                return -1;
            }
            break;
        }

        //if needed allocate next block
        check=BF_AllocateBlock(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in HT_CreateIndex-BF_AllocateBlock");
            return -1;
        }

        check=BF_GetBlockCounter(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in HT_InsertEntry-BF_GetBlockCounter");
            return -1;
        }
        int next_block=check-1;

        check=BF_ReadBlock(fileDesc,next_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in HT_CreateIndex-BF_ReadBlock");
            return -1;
        }
 
        //write to the current block (the previous) the number of the just allocated block
        memcpy(next_hash_block,&(next_block),sizeof(int));

        //current block finished
        check=BF_WriteBlock(fileDesc,cur_block);
        if (check<0)
        {
            BF_PrintError("Error in HT_CreateIndex-BF_WriteBlock");
            return -1;
        }

    }
  
    check=BF_CloseFile(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HT_CreateIndex-BF_CloseFile");
        return -1;
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
//HT_OpenIndex
////////////////////////////////////////////////////////////////////////////////////////

HT_info* HT_OpenIndex(char *fileName)
{
    int check;
    void* block;
    int fileDesc;


    check=BF_OpenFile(fileName);
    if (check<0)
    {
        BF_PrintError("Error in HT_OpenIndex-BF_OpenFile");
        return NULL;
    }
    else
        fileDesc=check;

    //read block 0 and check if it has the "HASH" information
    check=BF_ReadBlock(fileDesc,0,&block);
    if (check<0)
    {
        BF_PrintError("Error in HT_OpenIndex-BF_ReadBlock");
        return NULL;
    }
    else if (strcmp(block,"HASH")!=0)
    {
        printf("Not a hash table\n");
        return NULL;
    }

    //create a struct of hash table-key information
    HT_info *hash_info;
    hash_info = (HT_info*) malloc(sizeof(HT_info));
    hash_info->attrName = malloc(15*sizeof(char));
    
    hash_info->fileDesc=fileDesc;
    char* attrTypeptr=block+sizeof("HASH");                                                      
    hash_info->attrType=*attrTypeptr;                                       
    strcpy(hash_info->attrName,(block+sizeof("HASH")+sizeof(char)));  
    int* attrLengthptr=block+sizeof("HASH")+sizeof(char)+15*sizeof(char);                     
    hash_info->attrLength=*attrLengthptr;
    int *bucketsptr=block+sizeof("HASH")+sizeof(char)+15*sizeof(char)+sizeof(int);
    hash_info->numBuckets=*bucketsptr;
    
    return hash_info;
}


////////////////////////////////////////////////////////////////////////////////////////
//HT_CloseIndex
////////////////////////////////////////////////////////////////////////////////////////

int HT_CloseIndex(HT_info* header_info )
{
    int check;
    int fileDesc;

    fileDesc=header_info->fileDesc;
    check=BF_CloseFile(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HT_CloseFile-BF_CloseFile");
        return -1;
    }
    //free allocated space from HT_OpenIndex
    free(header_info->attrName);
    free(header_info);
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
//HT_InsertEntry
////////////////////////////////////////////////////////////////////////////////////////

int HT_InsertEntry(HT_info header_info,Record record)
{
    int check;
    void* block;
    int fileDesc=header_info.fileDesc;
    int num_records_limit=(BLOCK_SIZE-2*sizeof(int))/sizeof(Record);    //records fit in a block
    
    int buckets_per_block=get_buckets_per_block(header_info.numBuckets);
    int hash_value = hash_fun(record.id,header_info.numBuckets);    //using the hash function
    //hash block's number in order
    int hash_block_number=hash_value/buckets_per_block+1;
    //the hash block itself
    int* hash_block=find_hash_block(hash_block_number,fileDesc);
    //the position of the bucket's number in the hash block we got. (*2 because of the pointer to last overflow block)
    int index_pos=(hash_value%buckets_per_block)*2;

    check=BF_ReadBlock(fileDesc,*hash_block,&block);
    if (check<0)
    {
        BF_PrintError("Error in HT_InsertEntry-BF_ReadBlock");
        return -1;
    } 

    int *bucket_value=block+index_pos*sizeof(int);  //point to the bucket/block value
    int *last_overflow_block=block+index_pos*sizeof(int)+sizeof(int);  //point to the last overflow block value
    int next_block; //will be used as the 'pointer' to the next block
    int num_of_records;
    if(*bucket_value==-1)  //first time hashing here, need to allocate bucket
    {
        check=BF_AllocateBlock(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in HT_InsertEntry-BF_AllocateBlock");
            return -1;
        }

        check=BF_GetBlockCounter(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in HT_InsertEntry-BF_GetBlockCounter");
            return -1;
        }
        int last_block=check-1; //last block - just allocated bucket
        check=BF_ReadBlock(fileDesc,last_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in HT_InsertEntry-BF_ReadBlock");
            return -1;
        }

        //update hash table
        memcpy(bucket_value,&last_block,sizeof(int));
        memcpy(last_overflow_block,&last_block,sizeof(int));
        check=BF_WriteBlock(fileDesc,*hash_block);
        if (check<0)
        {
            BF_PrintError("Error in HT_InsertEntry-BF_WriteBlock");
            return -1;
        }

        //write the record to the bucket
        memcpy(block,&record,sizeof(Record));
        record_counter++;

        //write the number of records to the 'previous of the last' position in the block
        num_of_records=1;
        memcpy(block+BLOCK_SIZE-2*sizeof(int),&(num_of_records),sizeof(int));

        //next block does not exist for now
        next_block=-1;
        memcpy(block+BLOCK_SIZE-sizeof(int),&(next_block),sizeof(int));

        check=BF_WriteBlock(fileDesc,last_block);
        if (check<0)
        {
            BF_PrintError("Error in HT_InsertEntry-BF_WriteBlock");
            return -1;
        }
        return last_block;
    }
    else    //if(*bucket_value!=-1) - hashing here again
    {
        //Is record a duplicate? - Search for the value in the block chain
        int block_number=*bucket_value;  //iterate block chain
        int number_of_blocks=0; //number of blocks is actually used only in HT_GetAllEntries
        if (find_entry(record.id,&block_number,fileDesc,&number_of_blocks))
        {
            printf("Record with key : %d already in file.\n",record.id);
            return block_number;
        }
        
        //2 options:
        //either we insert to the last overflow block or we allocate a new one
        //either way we'll use the last overflow block, so read it
        check=BF_ReadBlock(fileDesc,*last_overflow_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in HT_InsertEntry-BF_ReadBlock");
            return -1;
        }
        memcpy(&num_of_records,block+BLOCK_SIZE-2*sizeof(int),sizeof(int));
        
        if (num_of_records==num_records_limit)  //no space left
        {
            check=BF_AllocateBlock(fileDesc);
            if (check<0)
            {
                BF_PrintError("Error in HT_InsertEntry-BF_AllocateBlock");
                return -1;
            }
            check=BF_GetBlockCounter(fileDesc);
            if (check<0)
            {
                BF_PrintError("Error in HT_InsertEntry-BF_GetBlockCounter");
                return -1;
            }
            int last_block=check-1;

            //at this time block points to the last overflow block
            //we want to update its next_block_pointer to the just allocated block (last_block)
            memcpy(block+BLOCK_SIZE-sizeof(int),&last_block,sizeof(int));
            check=BF_WriteBlock(fileDesc,*last_overflow_block);
            if (check<0)
            {
                BF_PrintError("Error in HT_InsertEntry-BF_WriteBlock");
                return -1;
            }

            //the just allocated block
            check=BF_ReadBlock(fileDesc,last_block,&block);
            if (check<0)
            {
                BF_PrintError("Error in HT_InsertEntry-BF_ReadBlock");
                return -1;
            }


            //update hash table
            //last_overflow_block becomes the just allocated block
            memcpy(last_overflow_block,&last_block,sizeof(int));
            check=BF_WriteBlock(fileDesc,*hash_block);
            if (check<0)
            {
                BF_PrintError("Error in HT_InsertEntry-BF_WriteBlock");
                return -1;
            }

            //write the record to the block
            memcpy(block,&record,sizeof(Record));
            record_counter++;

            //write the number of records
            num_of_records=1;
            memcpy(block+BLOCK_SIZE-2*sizeof(int),&(num_of_records),sizeof(int));

            //next block not exists for now
            next_block=-1;
            memcpy(block+BLOCK_SIZE-sizeof(int),&(next_block),sizeof(int));

            check=BF_WriteBlock(fileDesc,last_block);
            if (check<0)
            {
                BF_PrintError("Error in HT_InsertEntry-BF_WriteBlock");
                return -1;
            }
            return last_block;
        }
        else if (num_of_records<num_records_limit)   //we have space - insert in last_overflow_block
        {
            memcpy(block+num_of_records*sizeof(Record),&record,sizeof(Record));
            record_counter++;
            num_of_records++;
            memcpy(block+BLOCK_SIZE-2*sizeof(int),&(num_of_records),sizeof(int));
            check=BF_WriteBlock(fileDesc,*last_overflow_block);
            if (check<0)
            {
                BF_PrintError("Error in HT_InsertEntry-BF_WriteBlock");
                return -1;
            }
            return *last_overflow_block;
        }
    }
    return -1;  //if program reach this point, insert failed
}


////////////////////////////////////////////////////////////////////////////////////////
//HT_DeleteEntry
////////////////////////////////////////////////////////////////////////////////////////

int HT_DeleteEntry(HT_info header_info,void *value)
{
    int value_int=atoi(value);  //key used to delete will be integer
    int check;
    void* block;
    int fileDesc=header_info.fileDesc;
    int buckets_per_block=get_buckets_per_block(header_info.numBuckets);
    int hash_value = hash_fun(value_int,header_info.numBuckets);
    int hash_block_number=hash_value/buckets_per_block+1; //hash block's number in order
    int* hash_block=find_hash_block(hash_block_number,fileDesc); //the hashblock itself
    int index_pos=(hash_value%buckets_per_block)*2; //position of index

    check=BF_ReadBlock(fileDesc,*hash_block,&block);
    if (check<0)
    {
        BF_PrintError("Error in HT_DeleteEntry-BF_ReadBlock");
        return -1;
    }

    int *bucket_value=block+index_pos*sizeof(int);  //point to the bucket value
    if (*bucket_value==-1)  //no records in this bucket
    {
        printf("Record with key : %d was not found.\n",value_int);
        return -1;
    }
    int *last_overflow_block=block+index_pos*sizeof(int)+sizeof(int);  //point to the last overflow block value
    
    int block_number=*bucket_value;  //here will be the block number of the "to delete" record
    int number_of_blocks=0; //number of blocks is actually used only in HT_GetAllEntries
    
    //this will point to the record that will be deleted - if we find it
    void* to_delete=find_entry(value_int,&block_number,fileDesc,&number_of_blocks);
    if (!to_delete)
    {
        printf("Record with key : %d does not exist\n",value_int);
        return -1;
    }
    //find entry already opened the needed block
    
    //read last_overflow_block to access it's last entry
    check=BF_ReadBlock(fileDesc,*last_overflow_block,&block);
    if (check<0)
    {
        BF_PrintError("Error in HT_DeleteEntry-BF_ReadBlock");
        return -1;
    }
    //to_replace is a pointer to the last entry of the last overflow block
    int num_of_records;
    memcpy(&num_of_records,block+BLOCK_SIZE-2*sizeof(int),sizeof(int));
    void* to_replace=block+(num_of_records-1)*sizeof(Record);
    //replace the "to-delete" record with the "replace" record
    memcpy(to_delete,to_replace,sizeof(Record));
    record_counter--;

    //reducing number of records of the last block - we can't access it's last record from there
    //and when another insert happens it will write right there.
    num_of_records-=1;
    memcpy(block+BLOCK_SIZE-2*sizeof(int),&num_of_records,sizeof(int));
    
    check=BF_WriteBlock(fileDesc,*last_overflow_block);
    if (check<0)
    {
        BF_PrintError("Error in HT_DeleteEntry-BF_WriteBlock");
        return -1;
    }
    
    check=BF_WriteBlock(fileDesc,block_number);
    if (check<0)
    {
        BF_PrintError("Error in HT_DeleteEntry-BF_WriteBlock");
        return -1;
    }
    
    printf("Entry with key : %d deleted.\n",value_int);
    return 0;  
}


////////////////////////////////////////////////////////////////////////////////////////
//HT_GetAllEntries
////////////////////////////////////////////////////////////////////////////////////////

int HT_GetAllEntries(HT_info header_info,void *value)
{
    int number_of_blocks=0; //finally actually using this :')
    if (strcmp(value,"ALL")==0 || strcmp(value,"all")==0)
    {
        number_of_blocks=print_all_entries(header_info);
        return number_of_blocks;
    }
    int value_int=atoi(value);
    int check;
    void* block;
    int fileDesc=header_info.fileDesc;
    
    int hash_value = hash_fun(value_int,header_info.numBuckets);
    int buckets_per_block=get_buckets_per_block(header_info.numBuckets);
    int hash_block_number=hash_value/buckets_per_block+1; //hash block's number in order
    int* hash_block=find_hash_block(hash_block_number,fileDesc); //the hashblock itself
    int index_pos=(hash_value%buckets_per_block)*2; //position of index

    check=BF_ReadBlock(fileDesc,*hash_block,&block);
    if (check<0)
    {
        BF_PrintError("Error in HT_GetAllEntries-BF_ReadBlock");
        return -1;
    }

    int *bucket_value=block+index_pos*sizeof(int);  //point to the bucket value
    if (*bucket_value==-1)
    {
        printf("Record with key : %d was not found.\n",value_int);
        return -1;
    }
    
    int block_number=*bucket_value;  //iterate block chain
    Record record;
    void *record_found=find_entry(value_int,&block_number,fileDesc,&number_of_blocks);
    if (!record_found)
    {
        printf("Record with key : %d was not found.\n",value_int);
        return -1;
    }
    else
    {
        memcpy(&record,record_found,sizeof(Record));
        printf("%d %s %s %s\n",record.id, record.name, record.surname, record.address);
        return number_of_blocks;
    }
    printf("Error in HT_GetAllEntries\n");
    return -1; //function failed if program gets here
}


////////////////////////////////////////////////////////////////////////////////////////
//HashStatistics
////////////////////////////////////////////////////////////////////////////////////////

int HashStatistics(char* filename)
{
    HT_info *hashinfo=HT_OpenIndex(filename);
    int num_records_limit=(BLOCK_SIZE-2*sizeof(int))/sizeof(Record);    //records fit in a block
    int check;
    int fileDesc=hashinfo->fileDesc;
    int num_of_blocks=BF_GetBlockCounter(fileDesc);
    if (num_of_blocks<0)
    {
        BF_PrintError("Error in HashStatistics-BF_GetBlockCounter");
        return -1;
    }
    printf("1) File %s has %d blocks and %d records.\n",filename,num_of_blocks,record_counter);

    /*Minimum/average/maximum number of records per bucket*/
    int num_of_buckets=hashinfo->numBuckets;
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
    
    //get number of hash_blocks
    int num_hash_blocks=get_hash_blocks(num_of_buckets);
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
        int buckets_per_block=get_buckets_per_block(num_of_buckets);
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
        int buckets_per_block=get_buckets_per_block(num_of_buckets);
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
    HT_CloseIndex(hashinfo);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////