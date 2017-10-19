#Igor Farias Souza
CXX	=	g++	-std=c++11
FLAGS_1	=	-O3	-ansi	-DIL_STD#	-Wall	-pedantic
DEBUG	=	-pg	-g	-ansi	-DIL_STD3	-Wall	-pedantic		

CPX_INCDIR_1	= /opt/ibm/ILOG/CPLEX_Studio1261/cplex/include
CPX_INCDIR_2	= /opt/ibm/ILOG/CPLEX_Studio1261/concert/include
CPX_LDPATH_1	= /opt/ibm/ILOG/CPLEX_Studio1261/cplex/lib/x86-64_linux/static_pic
CPX_LDPATH_2	= /opt/ibm/ILOG/CPLEX_Studio1261/concert/lib/x86-64_linux/static_pic
CPX_LDADD_1	=	-lilocplex	-lcplex
CPX_LDADD_2	=	-lconcert	-lm -m64	-lpthread

CPLEX_FLAGS	=	-I$(CPX_INCDIR_1)	-I$(CPX_INCDIR_2)	-L$(CPX_LDPATH_1)	$(CPX_LDADD_1)	-L$(CPX_LDPATH_2)	$(CPX_LDADD_2)

OBJ	=	navio.o

main:	$(OBJ)
	$(CXX) 	$(FLAGS_1)	-o	main	$(OBJ)	$(CPLEX_FLAGS)

navio.o:	navio.cpp	
	$(CXX)	$(FLAGS_1)	$(CPLEX_FLAGS)	-c	navio.cpp	#$<	-o	$@
	
clean:
	rm	-rf	*.o	main
