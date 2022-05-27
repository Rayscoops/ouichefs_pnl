#!/bin/bash

etape1(){

        #Création de fichiers
        echo "ceci est un test" > f1
        echo "ceci est un test" > f2
        echo "ceci est un test" > f3

        touch f4
        touch f5

        #Ajouter des versions 

        echo "ceci est la suite du test" > f2
        echo "ceci est la suite du test" > f3

        echo "ceci est la suite du test" >> f1
        echo "ceci est la suite du test" >> f2

        echo "ceci est la suite du test" > f4

        # à la fin, on a 
        # 4 fichiers respectivement f1,f2,f3,f4
        # f1: 2 versions
        # f2: 3 versions
        # f3: 3 versions
        # f4: 2 versions
        # f5: 1 version
}
echo $1
# if [ $1 == "clean" ];
# then
#         echo yo
#         rm f1 f2 f3 f4 f5
# fi

# etape1