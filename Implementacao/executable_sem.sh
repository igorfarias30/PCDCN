#!/bin/bash
#set -x
cd ~/Dropbox/rcoelhos/orientacao/TCC/Igor/Monografia

#comando atualiza o compilador, para que c++11 esteja ativado
#sudo apt-get install gcc-4.7 g++-4.7

# ------ comentario---
# time + .\main + nome_instancia + alfa + beta + (estabilidade? 1:nao - 2:sim)

echo "----- 1º Bateria de Testes ---- "
echo " "

#time ./main inst_10_6_50_1.txt 0.75 0.25 1

echo "Fim da estancia inst_10_6_50_1"
echo " "

#time ./main inst_10_6_50_2.txt 0.75 0.25 1

echo "Fim da estancia inst_10_6_50_2"
echo " "

#time ./main inst_10_6_50_3.txt 0.75 0.25 1

echo "Fim da estancia inst_10_6_50_3"
echo " "

#time ./main inst_15_6_50_1.txt 0.75 0.25 1

echo "Fim da estancia inst_15_6_50_1"
echo " "

#time ./main inst_15_6_50_2.txt 0.75 0.25 1

echo "Fim da estancia inst_15_6_50_2"
echo " "

#time ./main inst_15_6_50_3.txt 0.75 0.25 1

echo "Fim da estancia inst_15_6_50_3"
echo " "

#time ./main inst_20_6_50_1.txt 0.75 0.25 1

echo "Fim da estancia inst_20_6_50_1"
echo " "

#time ./main inst_20_6_50_2.txt 0.75 0.25 1

echo "Fim da estancia inst_20_6_50_2"
echo " "

#time ./main inst_20_6_50_3.txt 0.75 0.25 1

echo "Fim da estancia inst_20_6_50_3"
echo " "


echo "----- 2º Bateria de Testes ---- "
echo " "

time ./main inst_10_6_50_1.txt 0.25 0.75 1

echo "Fim da estancia inst_10_6_50_1"
echo " "

time ./main inst_10_6_50_2.txt 0.25 0.75 1

echo "Fim da estancia inst_10_6_50_2"
echo " "

time ./main inst_10_6_50_3.txt 0.25 0.75 1

echo "Fim da estancia inst_10_6_50_3"
echo " "

time ./main inst_15_6_50_1.txt 0.25 0.75 1

echo "Fim da estancia inst_15_6_50_1"
echo " "

time ./main inst_15_6_50_2.txt 0.25 0.75 1

echo "Fim da estancia inst_15_6_50_2"
echo " "

time ./main inst_15_6_50_3.txt 0.25 0.75 1

echo "Fim da estancia inst_15_6_50_3"
echo " "

time ./main inst_20_6_50_1.txt 0.25 0.75 1

echo "Fim da estancia inst_20_6_50_1"
echo " "

time ./main inst_20_6_50_2.txt 0.25 0.75 1

echo "Fim da estancia inst_20_6_50_2"
echo " "

time ./main inst_20_6_50_3.txt 0.25 0.75 1

echo "Fim da estancia inst_20_6_50_3"
echo " "

echo "-------------------------------"
echo " "
