
echo "Instâncias em falta, com restrição"

echo "alfa 0 | beta 1"
echo " "

time ./main inst_10_6_50_2.txt 0 1 2

echo "Fim da estancia inst_10_6_50_2"
echo " "

time ./main inst_10_6_50_3.txt 0 1 2

echo "Fim da estancia inst_10_6_50_3"
echo " "

time ./main inst_15_6_50_1.txt 0 1 2

echo "Fim da estancia inst_15_6_50_1"
echo " "

time ./main inst_15_6_50_2.txt 0 1 2

echo "Fim da estancia inst_15_6_50_2"
echo " "

echo "alfa 1 | beta 0"
echo " "

time ./main inst_20_6_50_2.txt 1 0 2

echo "Fim da estancia inst_20_6_50_2"
echo " "
