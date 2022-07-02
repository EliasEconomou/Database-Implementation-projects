Ilias Oikonomou - 1115201200133
Ylopoihsh Systhmatwn Vasewn Dedomenwn
Project 2

SECTION 1 : COMPILATION

--Implemented in vscode using WSL:Ubuntu-18.04.
--Makefiles are from lesson 'Domes Dedomenwn-Xatzikokolakis'

--SHT Compile and run:
    -cd programs/SHT
    -use make run
    -use make clean to delete files
    or
    -use
        gcc -c main_SHT.c -Wall -g -I../../include
        gcc -o main_SHT main_SHT.o ../../modules/sht.c ../../modules/ht.c ../../lib/BF_64.a -I../../include -lm -no-pie
        ./main_SHT


SECTION 2 : SUMMARY - THINGS TO NOTICE
    -Block-structure opws sto project1
    -Xrhsimopoiw to myrandrecords.txt: to opoio metaksy allwn periexei surnames opws moustoksydis,aleksandrou,evaggelou
    pros anazhthsh me perissoteres apo mia emfaniseis.
    -An yparxoun duplicates apo surname-blockId den ta apothikevw sto secondary hash table, afou tha anazhthsw olo to 
    block sto opoio tha odhghthw sto primary hash table (xrhsh ths find_duplicate).
    -H Hash_Statistics mporei na xrhsimopoihthei eite apo primary eite apo secondary hash table. Gia na petyxw auto
    ekana mikres prosthikes sto HT.h perilamvanontas aparaithtes synarthseis tou ht.c gia na xrhsimopoihthoun apo thn
    Hash_Statistics sto sht.c.
    -Insert - SHT_SecondaryGetAllEntries doulevoun plhrws san to primary hash table: Apothikevw tis times tou hash se 
    blocks (ta hash_blocks) ta opoia apothikevw ston disko. Xrhsimopoiw opws fainetai kai sto structure 'last overflow 
    block pointer' gia kathe bucket.
    
