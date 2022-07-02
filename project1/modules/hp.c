#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "HP.h"
#include "BF.h"


////////////////////////////////////////////////////////////////////////////////////////
//Helpful Functions
////////////////////////////////////////////////////////////////////////////////////////

/*Searches for a record and returns a pointer to it while also 1) updating the 'block_number' of the block
that has the record and 2) counts the 'number_of_blocks read' to find it. If it doesn't find the record returns NULL.
Used in HP_InsertEntry, HP GetAllEntries and HP_DeleteEntry.*/
void* find_entry(int value, int* block_number, int fileDesc, int* number_of_blocks)
{
    int check;
    void* block;
    int *next_block;
    int num_of_records;
    //read block 0 to get the number of the first block to search
    check=BF_ReadBlock(fileDesc,0,&block);
    if (check<0)
    {
        BF_PrintError("Error in find_entry-BF_ReadBlock");
        return NULL;
    }
    memcpy(block_number,block+BLOCK_SIZE-sizeof(int),sizeof(int));

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

/*Prints all entries by passing 'ALL' in HP_GetAllEntries.*/
int print_all_entries(HP_info header_info)
{
    int check;
    int fileDesc=header_info.fileDesc;

    //read block 0 to get the first block
    void* block;
    check=BF_ReadBlock(fileDesc,0,&block);
    if (check<0)
    {
        BF_PrintError("Error in print_all_entries-BF_ReadBlock");
        return -1;
    }
    int block_number=*(int*)(block+BLOCK_SIZE-sizeof(int));   //first block
    
    //iterate blocks
    while(block_number!=-1)
    {
        check=BF_ReadBlock(fileDesc,block_number,&block);
        if (check<0)
        {
            BF_PrintError("Error in print_all_entries-BF_ReadBlock");
            return -1;
        }
        int num_of_records=*(int*)(block+BLOCK_SIZE-2*sizeof(int));
        //iterate records
        for (int i = 0; i < num_of_records; i++)
        {
            Record record;
            memcpy(&record,block+i*sizeof(Record),sizeof(Record));
            printf("%d %s %s %s\n",record.id, record.name, record.surname, record.address);
        }
        block_number=*(int*)(block+BLOCK_SIZE-sizeof(int));
    }
    return 0;
}




////////////////////////////////////////////////////////////////////////////////////////
//HP_CreateFile
////////////////////////////////////////////////////////////////////////////////////////

int HP_CreateFile(char *fileName,char attrType,char* attrName,int attrLength)
{
    int check;                  //check takes the return values of BF functions and checks for errors
    int fileDesc;               //file description number
    int blockNumber=0;          //read block 0
    void* block;                //access the read block

    BF_Init();
    check=BF_CreateFile(fileName);  //create a file of blocks named "filename"
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_CreateFile");
        return -1;
    }
 
    check=BF_OpenFile(fileName);    //opens the file named "filename"
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_OpenFile");
        return -1;
    }
    else
        fileDesc=check; //file id

    check=BF_AllocateBlock(fileDesc);   //allocates the first block of the file (0)
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_AllocateBlock");
        return -1;
    }

    check=BF_ReadBlock(fileDesc,blockNumber,&block); //read block 0 (first block)
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_ReadBlock");
        return -1;
    }
    else    //write information in block 0
    {
        //write 'HEAP' in first bytes of block 0
        memcpy(block,"HEAP",sizeof("HEAP"));

        //write the attrType in 'start of block'+sizeof("HEAP")
        memcpy(block+sizeof("HEAP"),&attrType,sizeof(char));

        //write the attrName in 'start of block'+sizeof("HEAP")+sizeof(char)
        memcpy(block+sizeof("HEAP")+sizeof(char),attrName,15*sizeof(char));

        //write the attrLength in 'start of block'+sizeof("HEAP")+sizeof(char)+strlen(attrName)*sizeof(char)
        memcpy(block+sizeof("HEAP")+sizeof(char)+15*sizeof(char),&attrLength,sizeof(int));
    }

    //Now let's allocate the very first block and store it's number in block 0
    check=BF_AllocateBlock(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_AllocateBlock");
        return -1;
    }

    check=BF_GetBlockCounter(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_GetBlockCounter");
        return -1;
    }
    int last_block=check-1;

    //read block 0 again, and write first block's number
    check=BF_ReadBlock(fileDesc,blockNumber,&block);
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_ReadBlock");
        return -1;
    }
    memcpy(block+BLOCK_SIZE-sizeof(int),&(last_block),sizeof(int));

    check=BF_WriteBlock(fileDesc,blockNumber);  //write block 0 to disc
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_WriteBlock");
        return -1;
    }

    //read first block to write number of records and next block 'pointer'
    check=BF_ReadBlock(fileDesc,last_block,&block);
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_ReadBlock");
        return -1;
    }
    int num_of_records=0;
    memcpy(block+BLOCK_SIZE-2*sizeof(int),&(num_of_records),sizeof(int));
    int next_block=-1;
    memcpy(block+BLOCK_SIZE-sizeof(int),&(next_block),sizeof(int));

    check=BF_WriteBlock(fileDesc,last_block);
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_WriteBlock");
        return -1;
    }

    check=BF_CloseFile(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HP_CreateFile-BF_CloseFile");
        return -1;
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
//HP_OpenFile
////////////////////////////////////////////////////////////////////////////////////////

HP_info* HP_OpenFile(char *fileName)
{
    int check;
    void* block;
    int fileDesc;

    check=BF_OpenFile(fileName);
    if (check<0)
    {
        BF_PrintError("Error in HP_OpenFile-BF_OpenFile");
        return NULL;
    }
    else
        fileDesc=check; //file id

    //read block 0 and check if it has the "HEAP" information
    check=BF_ReadBlock(fileDesc,0,&block);
    if (check<0)
    {
        BF_PrintError("Error in HP_OpenFile-BF_ReadBlock");
        return NULL;
    }
    else if (strcmp(block,"HEAP")!=0)
    {
        printf("Not a heap\n");
        return NULL;
    }

    //create a struct of heap-key information
    HP_info *heap_info;
    heap_info = (HP_info*) malloc(sizeof(HP_info));
    heap_info->attrName = malloc(15*sizeof(char));
    
    heap_info->fileDesc=fileDesc;
    char* attrTypeptr=block+sizeof("HEAP");                                                      
    heap_info->attrType=*attrTypeptr;                                       
    strcpy(heap_info->attrName,(block+sizeof("HEAP")+sizeof(char)));  
    int* attrLengthptr=block+sizeof("HEAP")+sizeof(char)+15*sizeof(char);                     
    heap_info->attrLength=*attrLengthptr;

    return heap_info;
}


////////////////////////////////////////////////////////////////////////////////////////
//HP_CloseFile
////////////////////////////////////////////////////////////////////////////////////////

int HP_CloseFile(HP_info* header_info )
{
    int check;
    int fileDesc;

    fileDesc=header_info->fileDesc;
    check=BF_CloseFile(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HP_CloseFile-BF_CloseFile");
        return -1;
    }
    //free allocated space from HP_OpenFile
    free(header_info->attrName);
    free(header_info);   
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
//HP_InsertEntry
////////////////////////////////////////////////////////////////////////////////////////

int HP_InsertEntry(HP_info header_info,Record record)
{
    int check;
    void* block;
    int fileDesc=header_info.fileDesc;
    int num_records_limit=(BLOCK_SIZE-2*sizeof(int))/sizeof(Record);        //records fit in a block

    int block_number;   //iterate block chain
    int number_of_blocks=0; //number of blocks is actually used only in HP_GetAllEntries
    if (find_entry(record.id,&block_number,fileDesc,&number_of_blocks))
    {
        printf("Record with key : %d already in file.\n",record.id);
        return block_number;
    }
    
    //get last block's number
    check=BF_GetBlockCounter(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HP_InsertEntry-BF_GetBlockCounter");
        return -1;
    }
    int last_block=check-1;
    
    //getting the number of records the last block has
    check=BF_ReadBlock(fileDesc,last_block,&block);
    if (check<0)
    {
        BF_PrintError("Error in HP_InsertEntry-BF_ReadBlock");
        return -1;
    }
    int num_of_records;
    memcpy(&num_of_records,block+BLOCK_SIZE-2*sizeof(int),sizeof(int));

    //about to allocate a new block - keep a pointer to the previous block
    //it will be used to change the 'next_block' value to the allocated block's number
    void* prev_block=block;

    //Inserting entry to the appropriate block
    //Case 1: our block can't have any more records.
    //Need to allocate a new block and set its number of records to 1 and its next_block value
    if (num_of_records==num_records_limit)
    {
        check=BF_AllocateBlock(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in HP_InsertEntry-BF_AllocateBlock");
            return -1;
        }
        check=BF_GetBlockCounter(fileDesc);
        if (check<0)
        {
            BF_PrintError("Error in HP_InsertEntry-BF_GetBlockCounter");
            return -1;
        }
        //number of new-allocated block
        int new_block=check-1;
        check=BF_ReadBlock(fileDesc,new_block,&block);
        if (check<0)
        {
            BF_PrintError("Error in HP_InsertEntry-BF_ReadBlock");
            return -1;
        }
        
        //allocated block - push the first record
        memcpy(block,&record,sizeof(Record));

        //allocated block - update the number of the next block to -1 (NULL)
        int next_block=-1;
        memcpy(block+BLOCK_SIZE-sizeof(int),&next_block,sizeof(int));

        //allocated block - update the number of records to 1
        num_of_records=1;
        memcpy(block+BLOCK_SIZE-2*sizeof(int),&num_of_records,sizeof(int));

        //previous block - update the number of the next_block to the value of the allocated block
        memcpy(prev_block+BLOCK_SIZE-sizeof(int),&(new_block),sizeof(int));
        
        //write previous block to disk
        check=BF_WriteBlock(fileDesc,last_block);
        if (check<0)
        {
            BF_PrintError("Error in HP_InsertEntry-BF_WriteBlock");
            return -1;
        }

        //write allocated block to disk and return
        check=BF_WriteBlock(fileDesc,new_block);
        if (check<0)
        {
            BF_PrintError("Error in HP_InsertEntry-BF_WriteBlock");
            return -1;
        }
        return new_block;
    }
    
    //Case 2: number of records hasn't exceed the limit
    else
    {
        //insert record
        memcpy(block+num_of_records*sizeof(Record),&record,sizeof(Record));
        //increment number of records
        num_of_records++;
        memcpy(block+BLOCK_SIZE-2*sizeof(int),&num_of_records,sizeof(int));

        check=BF_WriteBlock(fileDesc,last_block);
        if (check<0)
        {
            BF_PrintError("Error in HP_InsertEntry-BF_WriteBlock");
            return -1;
        }
        return last_block;
    }
}


////////////////////////////////////////////////////////////////////////////////////////
//HP_DeleteEntry
////////////////////////////////////////////////////////////////////////////////////////

int HP_DeleteEntry(HP_info header_info,void *value)
{
    int value_int=atoi(value);  //key used to delete will be integer
    int check;
    int fileDesc=header_info.fileDesc;
    void* block;
    
    int block_number;  //here will be the block number of the "to delete" record
    int number_of_blocks=0; //number of blocks is actually used only in HP_GetAllEntries
    
    //this will point to the record that will be deleted - if we find it
    void* to_delete=find_entry(value_int,&block_number,fileDesc,&number_of_blocks);
    if (!to_delete)
    {
        printf("Record with key : %d does not exist\n",value_int);
        return -1;
    }

    //at this place we have the "to delete" record's place in to_delete and
    //we need the last record of the last block to insert on top of the "to delete" entry
    check=BF_GetBlockCounter(fileDesc);
    if (check<0)
    {
        BF_PrintError("Error in HP_DeleteEntry-BF_GetBlockCounter");
        return -1;
    }
    int last_block;
    last_block=check-1;
    check=BF_ReadBlock(fileDesc,last_block,&block);
    if (check<0)
    {
        BF_PrintError("Error in HP_DeleteEntry-BF_ReadBlock");
        return -1;
    }
    int num_of_records;
    memcpy(&num_of_records,block+BLOCK_SIZE-2*sizeof(int),sizeof(int));
    int* to_replace;    //this will point to the record that will replace the "to-delete" record
    to_replace=block+(--num_of_records)*sizeof(Record);

    //replace the "to-delete" record with the "to replace" record and save block
    memcpy(to_delete,to_replace,sizeof(Record));
    check=BF_WriteBlock(fileDesc,block_number);
    if (check<0)
    {
        BF_PrintError("Error in HP_DeleteEntry-BF_WriteBlock");
        return -1;
    }
    
    //we reduced the number of records of the last block so that we don't have access to it's last record from there
    //and when another insert happens it will write right there.
    memcpy(block+BLOCK_SIZE-2*sizeof(int),&num_of_records,sizeof(int));
    check=BF_WriteBlock(fileDesc,last_block);
    if (check<0)
    {
        BF_PrintError("Error in HP_DeleteEntry-BF_WriteBlock");
        return -1;
    }

    printf("Entry with key : %d deleted.\n",value_int);
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
//HP_GetAllEntries
////////////////////////////////////////////////////////////////////////////////////////

int HP_GetAllEntries(HP_info header_info,void *value)
{
    int number_of_blocks=0; //HP_GetAllEntries needs this
    if (strcmp(value,"ALL")==0 || strcmp(value,"all")==0)
    {
        number_of_blocks=print_all_entries(header_info);
        return number_of_blocks;
    }
    int value_int=atoi(value);
    int fileDesc=header_info.fileDesc;

    Record record;
    int block_number;
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
    printf("Error in HP_GetAllEntries\n");
    return -1; //function failed if program gets here
}

////////////////////////////////////////////////////////////////////////////////////////