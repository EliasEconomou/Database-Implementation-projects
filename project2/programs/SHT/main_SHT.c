#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SHT.h"
#include "BF.h"

int main(int argc, char** argv)
{
	FILE *fp;
	fp = fopen("../../files/myrandrecords.txt","r");
	
	HT_CreateIndex("ht_file",'i',"id", 5, 11);
	HT_info *hashinfo=HT_OpenIndex("ht_file");

	SHT_CreateSecondaryIndex("sht_file","surname",15,9,"ht_file");
	SHT_info *shashinfo=SHT_OpenSecondaryIndex("sht_file");

	/////////////////////////////////////////////////////////////////////////////////
	////This section gets all records of the file, inserts in blocks and searches////
	/////////////////////////////////////////////////////////////////////////////////

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
		Record record;
		record.id=atoi(tempid);
		strcpy(record.name,tempname);
		strcpy(record.surname,tempsur);
		strcpy(record.address,tempadd);
		SecondaryRecord srecord;
		srecord.record=record;
		srecord.blockId=HT_InsertEntry(*hashinfo, record);
		if (srecord.blockId!=-1)	//if everything is ok with insert in primary hash table
		{							//insert in secondary hash table
			SHT_SecondaryInsertEntry(*shashinfo,srecord);
		}
	}
	fclose(fp);

	char* search_value="moustoksydis";
	printf("--Searching for %s : %s.\n",shashinfo->attrName,search_value);
	int num_of_blocks=SHT_SecondaryGetAllEntries(*shashinfo,*hashinfo,search_value);
	if (num_of_blocks>0)
	{
		printf("-Number of blocks read (from both ht - sht) : %d.\n",num_of_blocks);
	}

	SHT_CloseSecondaryIndex(shashinfo);
	HT_CloseIndex(hashinfo);
	printf("\n--Printing statistics for primary hash table.\n");
	HashStatistics("ht_file");
	printf("\n--Printing statistics for secondary hash table.\n");
	HashStatistics("sht_file");
}