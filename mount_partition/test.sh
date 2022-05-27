#!/bin/bash

etape1(){

        #Création de répertoire        
        mkdir partition_ouichefs/d1
        mkdir partition_ouichefs/d2

        #Création de fichiers
        echo "ceci est un test" > partition_ouichefs/f1
        echo "ceci est un test" > partition_ouichefs/f2
        echo "ceci est un test" > partition_ouichefs/d1/f3

        touch partition_ouichefs/d2/f4
        touch partition_ouichefs/f5

        #Ajouter des versions 

        echo "ceci est la suite du test" > partition_ouichefs/f2
        echo "ceci est la suite du test" >  partition_ouichefs/d1/f3

        echo "ceci est la suite du test" >> partition_ouichefs/f1
        echo "ceci est la suite du test" >> partition_ouichefs/f2

        echo "ceci est la suite du test" > partition_ouichefs/d2/f4

        # 5 fichiers respectivement f1,f2,d1/f3,d2/f4,f5
        # 2 dossiers d1 et d2 à la racine
        # f1: 2 versions
        # f2: 3 versions
        # f3: 2 versions
        # f4: 1 versions
        # f5: 1 version
}

if [[ $1 = "clean" ]];
then
        rm partition_ouichefs/f1 partition_ouichefs/f2 partition_ouichefs/f5
        rm -r partition_ouichefs/d1 partition_ouichefs/d2
else
        etape1
fi