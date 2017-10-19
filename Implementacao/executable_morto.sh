#!/bin/bash
#set -x

# Compilar, antes de executa-los

cd ~/Dropbox/rcoelhos/orientacao/TCC/Igor/Monografia

echo "------ Arquivos Para serem executados novamente -----"
echo " "

time ./main inst_10_6_50_3.txt 1 0 1

echo "Fim da estancia inst_20_6_50_3"
echo " "

time ./main inst_15_6_50_3.txt 1 0 1

echo "Fim da estancia inst_20_6_50_3"
echo " "

echo "--------- Arquivos Mortos --------"

time ./main inst_20_6_50_3.txt 0 1 2

echo "Fim da estancia inst_10_6_50_3"
echo " "

time ./main inst_10_6_50_3.txt 0.5 0.5 2

echo "Fim da estancia inst_10_6_50_3"
echo " "

time ./main inst_15_6_50_3.txt 0.5 0.5 2

echo "Fim da estancia inst_15_6_50_3"
echo " "

time ./main inst_20_6_50_3.txt 0.5 0.5 2

echo "Fim da estancia inst_20_6_50_3"
echo " "
