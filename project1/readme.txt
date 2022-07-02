Ilias Oikonomou - 1115201200133
Ylopoihsh Systhmatwn Vasewn Dedomenwn
Project 1

SECTION 1 : COMPILATION

--Implemented in vscode using WSL:Ubuntu-18.04.
--Makefiles are from lesson 'Domes Dedomenwn-Xatzikokolakis'

--HP Compile and run:
    -cd programs/HP
    -use make run
    -use make clean to delete files
    or
    -use
        gcc -c main_HP.c -Wall -g -I../../include
        gcc -o main_HP main_HP.o ../../modules/hp.c ../../lib/BF_64.a -I../../include -lm -no-pie
        ./main_HP

--HT Compile and run:
    -cd programs/HT
    -use make run
    -use make clean to delete files
    or
    -use
        gcc -c main_HT.c -Wall -g -I../../include
        gcc -o main_HT main_HT.o ../../modules/ht.c ../../lib/BF_64.a -I../../include -lm -no-pie
        ./main_HT


SECTION 2 : BLOCK STRUCTURE

--HEAP BLOCK STRUCTURE
    block 0:
        <['HEAP'][attrType][attrName][attrLength].................................[1st block]>
    block 1...N:
        <[record1][record2][record3][record4][record5]..............[num_records][next_block]>

--HASH TABLE BLOCK STRUCTURE
    block 0:
        <['HASH'][attrType][attrName][attrLength][buckets]...................[1st hash block]>
    blocks for mapping:
        <[bucket-index1,last_ovfl_blk1]++...................................[next_hash_block]>
    buckets - overflow blocks
        <[record1][record2][record3][record4][record5]..............[num_records][next_block]>


SECTION 3 : SUMMARY - THINGS TO NOTICE

    --HEAP

        -Insert panta sto teleytaio block. Delete me replacing ths epithymhths eggrafhs me thn teleytaia 
        eggrafh tou teleytaiou block - meiwnontas twn arithmo twn eggrafwn kai ara apokryptontas thn teleytaia. 
        Me value 'all' sthn HP_GetAllEntries ektypwnontai oles tis eggrafes tou arxeiou.
        -Xrhsimopoiwntas thn find_entry 1) kanw elegxo sthn insert an yparxei h eggrafh (elegxos gia diplotypa),
        enw 2) sthn delete kai thn get_All psaxnw prwta thn eggrafh kai an yparxei sto arxeio th diagrafw 'h thn
        ektypwnw antistoixa.


    --HASH TABLE

        -Insert - Delete - HT_GetAllEntries doulevoun plhrws san to heap apo panw me th diafora oti ena heap edw
        isodynamei me ena bucket.

        -Apothikevw tis times tou hash se blocks (ta hash_blocks) ta opoia apothikevw ston disko.
        -Xrhsimopoiw opws fainetai kai sto structure 'last overflow block pointer' gia kathe bucket.

        -Kanonika to 'buckets_per_block' (ennowntas ta buckets pou xwroun se ena hash_block) vgainei 63 afou
        se ena block xwroun 126 integers opws ekshgw kai ston kwdika.
        Etsi gia paradeigma an exoume 103 buckets,  
        --to 1o hash_block tha exei 63 buckets plus 63 'last overflow block pointers'
        --to 2o hash_block tha exei 40 buckets plus 40 'last overflow block pointers'
        
        -Paradeigmata:
        --1k eggrafes + 'plhthos buckets' = 701 + buckets_per_block = 20
        --1k eggrafes + 'plhthos buckets' = 31 + buckets_per_block = 1
        --5k eggrafes + 'plhthos buckets' = 4821 + buckets_per_block = 20
        --5k eggrafes + 'plhthos buckets' = 301 + buckets_per_block = 1
        --10k eggrafes + 'plhthos buckets' = 6001 + buckets_per_block = 20
        --myrandrecords (64 eggrafes) + 'plhthos buckets' = 301 + buckets_per_block = 10
        --myrandrecords (64 eggrafes) + 'plhthos buckets' = 2 + buckets_per_block = 63
