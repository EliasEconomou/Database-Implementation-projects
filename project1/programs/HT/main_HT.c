#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HT.h"

int main(int argc, char** argv)
{
	FILE *fp;
	fp = fopen("../../files/records1K.txt","r");
	Record record;
	HT_CreateIndex("ht_file",'i',"id", 5, 199);
	HT_info *hashinfo=HT_OpenIndex("ht_file");

	////////////////////////////////////////////////////////////////////////////////////////////////
	////This section gets all records of the file and stores them in blocks using HT_InsertEntry////
	////////////////////////////////////////////////////////////////////////////////////////////////

	char temp_rec[100];
	char* temp_rec2;
	char tempid[5];
	char tempname[15];
	char tempsur[25];
	char tempadd[50];

	printf("\n--Inserting all entries of the file...\n...\n");
	while (!feof(fp)) 
	{
		fscanf(fp,"%s",temp_rec);
		//strtok to remove the first character '{'
		temp_rec2 = strtok(temp_rec,"{}");
		//removing the rest characters using sscanf
		sscanf(temp_rec2,"%[^,\"]%*[,\"] %[^\",\"]%*[\",\"] %[^\",\"]%*[\",\"] %[^\"]%*[\"]",tempid,tempname,tempsur,tempadd);
		//passing attributes to disk
		record.id=atoi(tempid);
		strcpy(record.name,tempname);
		strcpy(record.surname,tempsur);
		strcpy(record.address,tempadd);
		HT_InsertEntry(*hashinfo, record);
		//unclomment below to print return value of HT_InsertEntry and see where records go
		//printf("Record with key : %d, is in block %d.\n",record.id,insert_return_value);
	}

	fclose(fp);

	//////////////////////////////////////////////////////////////////////////////
	////This section tests all functions//////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	//getting all entries of 'key to find'
	char* key_to_find="36";
	printf("--Getting all entries of key : %s.\n",key_to_find);
	int blocks_read=HT_GetAllEntries(*hashinfo,key_to_find);
	printf("To search for key : %s, %d blocks have been read.\n",key_to_find,blocks_read);
	
	//deletion
	printf("--Deleting entry with key : %s.\n",key_to_find);
	HT_DeleteEntry(*hashinfo,key_to_find);
	printf("--Trying to get the entries of key : %s.\n",key_to_find);
	HT_GetAllEntries(*hashinfo,key_to_find);

	//inserting premade record
	char* key_to_insert="1066";
	Record premade_record;
	premade_record.id=atoi(key_to_insert);
	strcpy(premade_record.name,"Ilias");
	strcpy(premade_record.surname,"Leouras");
	strcpy(premade_record.address,"Zwgrafou_1");
	printf("--Inserting a new premade record with key : %d.\n",premade_record.id);
	int block = HT_InsertEntry(*hashinfo, premade_record);
	printf("Record with key : %s, is in block %d.\n",key_to_insert,block);
	printf("--Getting all entries of key : %d.\n",premade_record.id);
	HT_GetAllEntries(*hashinfo,key_to_insert);
	printf("--Trying to insert record with key : %d again.\n",premade_record.id);
	HT_InsertEntry(*hashinfo, premade_record);

	//optional - print all records using 'ALL'
	char answer[2];
	printf("\n--Do you want to print ALL entries 'y' or 'n': ");
	scanf("%s",answer);
	if (strcmp(answer,"y")==0 || strcmp(answer,"Y")==0)
	{
		printf("\n--Printing all entries of FILE using 'ALL'.\n");
		HT_GetAllEntries(*hashinfo,"ALL");
	}
	
	HT_CloseIndex(hashinfo);
	//statistics
	printf("\n--Printing statistics.\n");
	HashStatistics("ht_file");
	
	return 0;
}