/** Universidade Federal do Ceará
* @title: Implementação do Modelo da Monografia
* @author: Igor Farias Souza - 352202
* @orientador: Prof. Dr. Ricardo Coelho 
*/

#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <iomanip>
#include <stack>
#include <ilcplex/ilocplex.h>

using namespace std;
ILOSTLBEGIN

typedef struct slot{
	bool ocupado;
	int destino;
	bool remanejado;
}slot;

// Após rodar um certo tempo, armazenar a melhor solução encontrada
ILOMIPINFOCALLBACK5(timeLimitCallback, IloCplex, cplex, IloBool, aborted, IloNum, timeStart, IloNum, timeLimit, IloNum, acceptableGap){
	
	if ( !aborted  &&  hasIncumbent() ) {
		
		IloNum gap = 100.0 * getMIPRelativeGap();		
		IloNum timeUsed = cplex.getCplexTime() - timeStart;
		
		//if ( timeUsed > 1 )
		//	getEnv().out() << timeUsed << endl;
	
		if ( timeUsed > timeLimit || gap < acceptableGap ) {
			
			getEnv().out() << endl
			<< "Good enough solution at "
			<< timeUsed << " sec., gap = "
			<< gap << "%, quitting." << endl;
			aborted = IloTrue;
			abort();
		
		}
	}
}

ILOHEURISTICCALLBACK1(Rounddown, IloIntVarArray, vars) {		
		IntegerFeasibilityArray feas;
		IloNumArray             obj;
		IloNumArray             x;
		try {

			feas = IntegerFeasibilityArray(getEnv());
			obj  = IloNumArray(getEnv());
			x    = IloNumArray(getEnv());
			getFeasibilities(feas, vars);
			getObjCoefs     (obj , vars);
			getValues       (x   , vars);

			IloNum objval = getObjValue();
			IloInt cols   = vars.getSize();
			for (IloInt j = 0; j < cols; j++) {
			// Set the fractional variable to zero and update the objective value
				if ( feas[j] == Infeasible ) {
					objval -= x[j] * obj[j];
					x[j] = 0.0;
				}
			}
			setSolution(vars, x, objval);
		}
		catch (...) {
			feas.end();
			obj.end();
			x.end();
			throw;
		}
		feas.end();
		obj.end();
		x.end();
	}

// Classe para o objeto do tipo problema
class Problema{

private:

	IloEnv env; //variavel do ambiente do cplex
	IloModel modelo, relaxado; // modelo para o problema
	IloCplex cplex, cplex_relaxado; 
	IloObjective fObj; //variével da função Objetivo
	IloIntVarArray X, Y; //Variaveis de decisão
	
	//IloNumVarArray XCM, YCM; // para a estabilidade
	
	IloNumArray valor;
	//IloNumArray _valor;
	
	string instancia; //armazena o nome da instancia
	slot ***slotSolucao;
	
	int N; //numero de portos
	int R; // numero de linhas
	int C; // numero de colunas
	 
	int size1, size2; //tamanho das variaveis X e Y
	int nTriangulo; // numero de elementos de uma matriz triangular superior
	
	int *****x; // tensor para a variavel binária x
	int ***y; // tensor para a variavel binaria y
	int **T; //matrix de transporte
	
	
	// Matrizes Para solucao inicial
	// -------------------
	int *****start_x;
	int ***start_y; //pegando uma solucao inicial para y;
	// -------------------
	
	float **d; //distancia do slot para o cG da baia
	
	int *nConteiner; // numero de conteiners em cada porto
	int estabilidade; //inteiro que diferencia qual tipo de restricao para o problema
	
	// escalares da função Objetivo
	float alfa, beta;
	
	int *nRemanejosPorto; // Guarda o numero de rearranjos nos N - 2 portos. No porto inicial e final são desprezados
	
	//Strings para o nome do arquivo
	string aalfa, bbeta, ooption;
	
public:

	// Para a matriz de transporte T_{ij}
	// função para calcular o número de elementos de uma matriz triangular superior
	int pascal(int i){	
		if(i == 2)
			return 3;
		else
			return pascal(i - 1) + i;
	}
	
	//Problema(string arq, float a, float b, int option){
	
	Problema(string arq, const char* a, const char* b, const char * option){
	
		alfa = atof(a); //peso do somatorio 1
		this->beta = atof(b); //peso do somatorio 2
		
		//Pegando o tipo de restriçao de estebilidade
		this->estabilidade = atoi(option);
		
		//pegando o nome da instancia
		instancia = arq;
		
		//armazenando os valores: alfa, beta e estabilidade na string
		this->aalfa = a;
		this->bbeta = b;
		this->ooption = option;
		
		// Lendo o arquivo
		ifstream entrada;
		entrada.open(arq.c_str());
		
		if(entrada.fail()){
			cerr << "Falha na leitura de arquivo!" << endl;
			exit(-1);
		}
		
		entrada >> N; // portos
		entrada >> R; // linhas
		entrada >> C; //Colunas
		
		// iniciando a matriz de transporte
		T = new int*[N];
		for(int i = 0; i < N; i++){
			T[i] = new int[N];
			//for(int j = 0; j < N; j++)
			//	T[i][j] = 0;
		}
		
		int origem, destino, numeroConteiner;
		
		/* // Caso a instancia nao possuisse a matrix de transporte
		nTriangulo = pascal(N);
		// Alocando a matriz de Transporte
		for(int i = 0; i < nTriangulo; i++){
			entrada >> origem >> destino >> nConteiner;
			T[origem - 1][destino - 1] = nConteiner;
		} */
		
		//lendo a matriz de transporte
		for(int i = 0; i < N; i++)
			for(int j = 0; j < N; j++){
				entrada >> numeroConteiner;
				T[i][j] = numeroConteiner;
			}
		
		// variavel de x_ijv(r, c)
		int indice = 0;
		x = new int ****[N];
		for(int i = 0; i < N; i++){
			x[i] = new int ***[N];
			for(int j = 0; j < N; j++){
				x[i][j] = new int **[N];
				for(int v = 0; v < N; v++){
					x[i][j][v] = new int *[R];
					for(int r = 0; r < R; r++){
						x[i][j][v][r] = new int [C];
						for(int c = 0; c < C; c++){
							x[i][j][v][r][c] = indice;
							indice++;
						}
					}
				}
			}
		}
		
		indice = 0;
		// variavel binaria y_i(r, c)
		y = new int **[N];
		for(int i = 0; i < N; i++){
			y[i] = new int*[R];
			for(int r = 0; r < R; r++){
				y[i][r] = new int [C];
				for(int c = 0; c < C; c++){
					y[i][r][c] = indice;
					indice++;
					//cout << y[i][r][c] << " ";
				}
			}			
		}
		
		//distancia do cg do slot ao cg da baia
		d = new float*[R];
		for(int r = 0; r < R; r++)
			d[r] = new float[C];
		
		//slot ***slotSolucao; // guarda para cada porto i, o destino do contêiner alocado no slot (r, c)
		slotSolucao = new slot **[N];
		for(int i = 0; i < N; i++){
			slotSolucao[i] = new slot *[R];
			for(int r = 0; r < R; r++){
				slotSolucao[i][r] = new slot [C];
				for(int c = 0; c < C; c++){
					slotSolucao[i][r][c] = {false, 0, false};
					//cout << "origem: " << slotSolucao[i][r][c].origem << endl;
					//cout << "destino: " << slotSolucao[i][r][c].destino << endl;
				}
			}
		}
		
	}
	
	// Verifica se a instancia tem solução viável
	bool factivel(void){
		
		nConteiner = new int[N - 1];
		
		for(int p = 0; p < N; p++){
		int soma = 0;	
			for(int i = 0; i <= p; i++)
				for(int j = p + 1; j < N; j++)
					if(T[i][j] <= R * C)
						soma += T[i][j];
					else
						return false;
			
			//cout << "Porto " << p + 1 << " = " << soma << endl;
			
			//o tamanho de nConteiner é N - 1
			if(p < N - 1)
				nConteiner[p] = soma;
		}
		
		return true;
	}
	
	// DEstrutor
	~Problema(){
		
		// desalocando a variavel x
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++){
				for(int v = 0; v < N; v++){
					for(int r = 0; r < R; r++)
						delete [] x[i][j][v][r];
				}
			}
		}
		
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++){
				for(int v = 0; v < N; v++){
					delete [] x[i][j][v];
				}
			}
		}
		
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++){
				delete [] x[i][j];
			}
		}
		
		for(int i = 0; i < N; i++)
			delete [] x[i];
		
		delete [] x;
		
		// desalocando a variavel y	
		for(int i = 0; i < N; i++)
			for(int r = 0; r < R; r++)
				delete [] y[i][r];
		
		for(int i = 0; i < N; i++)
			delete [] y[i];
		
		delete [] y;
		
		// Desalocando a matriz de transporte		
		for(int i = 0; i < N; i++)
			delete[] T[i];
		
		delete [] T;
		
		//desalocando o matriz de distancias
		for(int r = 0; r < R; r++)
			delete [] d[r];
		
		delete [] d;
		
		// desalocando a variavel slotSolucao
		for(int i = 0; i < N; i++)
			for(int r = 0; r < R; r++)
				delete [] slotSolucao[i][r];
		
		for(int i = 0; i < N; i++)
			delete [] slotSolucao[i];
		
		delete [] slotSolucao;
		
		delete [] nConteiner;
		
		//deletando o vetor que armazena o numero de rearranjos em cada portos
		delete [] nRemanejosPorto;
		
		env.end();
	}
	
	
	// Iniciando as variáveis de decisão
	void iniciarVariaveis(void){
		
		size1 = R*C*N*N*N; // Numero de variaveis do tipo X
		size2 = R*C*N; // Numero de variaveis do tipo Y
		
		X = IloIntVarArray(env, size1, 0, 1);
		Y = IloIntVarArray(env, size2, 0, 1);

		cout << "Tamanho de X: " << size1 << endl;
		cout << "Tamanho de Y: " << size2 << endl;
		
		char strnum[30];
		
		for(int i = 0, indice = 0; i < N; i++){
			for(int j = 0; j < N; j++){
				for(int v = 0; v < N; v++){
					for(int r = 0; r < R; r++){
						for(int c = 0; c < C; c++, indice++){
							sprintf(strnum, "x%d%d%d(%d,%d)", i + 1, j + 1, v + 1, r + 1, c + 1);
							X[indice].setName(strnum);
						}
					}
				}
			}
		}
		
		
		for(int i = 0, indice = 0; i < N; i++){
			for(int r = 0; r < R; r++){
				for(int c = 0; c < C; c++, indice++){
					sprintf(strnum, "y%d(%d,%d)", i + 1, r + 1, c + 1);
					Y[indice].setName(strnum);
				}
			}
		}
		
		float base, altura;
		// calculando as distancias
		// variaveis auxiliar para ajudar no calcula da distancia	
		stack <float> d_aux;
		
		/*for(int r = 0; r < R; r++){
			for(int c = 0; c < C; c++){
				
				//caso impar
				if(R % 2){
					//pegando as teto(R/2) linhas
					if(r < R/2 + 1){
						// 1 quadrante: superior esquerdo	
						if(c < C/2){
							base = abs( (float)C/2 - (0.5 + c) );
							altura = abs( (float)R/2 - (0.5 + r) );
							//cout << "base: " << base << " altura: " << altura << endl;	
							d[r][c]	= sqrt(pow(base, 2) + pow(altura, 2));
							d_aux.push(d[r][c]);
						}else{
							d[r][c] = d_aux.top();
							d_aux.pop();
						}	
					}else{
						// Como o numero de linhas é impar, logo so vou preencher as piso(R/2), pois ja 
						//preenchi as teto(R/2)
						for(int rr = 0; rr < R/2; rr++){
							d_aux.push(d[rr][c]);
						}
						d[r][c] = d_aux.top();
						d_aux.pop();
					}
				}else{
					// Caso o numero de linhas seja PAR
					//1 quadrante
					if(r < R/2){
						// lado esquerdo superior
						if(c < C/2){
							base = abs( (float)C/2 - (0.5 + c) );
							altura = abs( (float)R/2 - (0.5 + r) );
							//cout << "base: " << base << " altura: " << altura << endl;	
							d[r][c]	= sqrt(pow(base, 2) + pow(altura, 2));
							d_aux.push(d[r][c]);
						}else{
							d[r][c] = d_aux.top();
							d_aux.pop();
						}
					}else{
						
						for(int rr = 0; rr < R/2; rr++){
							d_aux.push(d[rr][c]);
						}
						d[r][c] = d_aux.top();
						d_aux.pop();
					}
				}
				cout << d[r][c] << " ";
			}
			cout << endl;
		}*/
		
		// variaveis auxiliares
		int rr = 0, index = 1;
		for(int r = 0; r < R; r++){
			
			if(R%2){
				if(r < R/2 + 1){		
					for(int c = 0; c < C/2; c++){
						
						base = abs( (float)C/2 - (0.5 + c) );
						altura = abs( (float)R/2 - (0.5 + r) );
						//cout << "base: " << base << " altura: " << altura << endl;	
						d[r][c]	= sqrt(pow(base, 2) + pow(altura, 2));
						d_aux.push(d[r][c]);
					}for(int c = C/2; c < C; c++){
						d[r][c] = d_aux.top();
						d_aux.pop();
					}	
				
				}else{
					// r >= R/2 + 1
					for(int c = 0; c < C/2; c++){
						
						base = abs( (float)C/2 - (0.5 + c) );
						altura = abs( (float)R/2 - (0.5 + r) );
						//cout << "base: " << base << " altura: " << altura << endl;	
						d[r][c]	= sqrt(pow(base, 2) + pow(altura, 2));
						d_aux.push(d[r][c]);
					}
					for(int c = C/2; c < C; c++){
						d[r][c] = d_aux.top();
						d_aux.pop();
					}	
				}
			}else{
				// caso PAR
				// Caso o numero de linhas seja PAR
					//1 quadrante
					if(r < R/2){
						// lado esquerdo superior
						for(int c = 0; c < C/2; c++){
							base = abs( (float)C/2 - (0.5 + c) );
							altura = abs( (float)R/2 - (0.5 + r) );
							//cout << "base: " << base << " altura: " << altura << endl;	
							d[r][c]	= sqrt(pow(base, 2) + pow(altura, 2));
							d_aux.push(d[r][c]);
						//	cout << d[r][c] << " ";
						}for(int c = C/2; c < C; c++){
							d[r][c] = d_aux.top();
							d_aux.pop();
						//	cout << d[r][c] << " ";
						}
					}else{
						
						//cout << "\trr == " << rr << endl;
						if(rr < R/2){
							
							// lado inferior esquerdo
							for(int c = 0; c < C/2; c++){
								//cout << "soma: " << r - (rr + 1) << endl;
								d[r][c] = d[r - (rr + index)][c];
								d_aux.push(d[r][c]);
			//					cout << d[r][c] << " ";
							}
						
							//lado inferior direito
							for(int c = C/2; c < C; c++){
								d[r][c] = d_aux.top();
								d_aux.pop();
			//					cout << d[r][c] << " ";
							}
							
							rr++; index++;
						
						}
						
					}
					
					//cout << "\tsize = " << d_aux.size() << endl;
			}
			//cout << endl;
		}
	
	}// fim da função iniciarVariaveis
	
	// Criando a Funcao Objetivo
	void funcaoObjetivo(void){
		
		fObj = IloObjective(env);
		IloExpr expr(env);
		
		// variavel de x_ijv(r, c)
		for(int i = 0; i < N - 1; i++){
			for(int j = i + 1; j < N; j++){
				// Colocar como v <= j ou v < j
				
				for(int v = i + 1; v < j; v++){
				//int v = i + 1;
					for(int r = 0; r < R; r++){
						for(int c = 0; c < C; c++){
							expr += X[x[i][j][v][r][c]];
							//cout << i + 1 << j + 1 << v + 1 << " ";
						}
					}
				}
			}
		//	cout << endl;
		}
		
		expr *= alfa; 
		//Estabilidade
		IloExpr somatorio_2(env);
		
		/*
		for(int i = 0; i < N - 1; i++){
			
			for(int r = 0; r < R; r++){
				for(int c = 0; c < C; c++){
					expr2 += Y[y[i][r][c]];
				}
			}
			
			for(int r = 0; r < R; r++){
				for(int c = 0; c < C; c++){
					expr1 += ( Y[y[i][r][c]] * (r + 1 - 0.5));
				}
			}
			
			//expr1 = expr1/expr2;
			
			for(int r = 0; r < R; r++){
				for(int c = 0; c < C; c++){
					expr3 += (Y[y[i][r][c]] * (c + 1 - 0.5)); //XCM_i
				}
			}
			
			expr4 += ( ((expr1)) - (float) R/2 ) *( (expr1) - (float) R/2) + ( (expr3) - (float) C/2) * ( (expr3) - (float) C/2);
			
			expr1.clear();
			expr2.clear();
			expr3.clear();
		}
		
		fObj = IloMinimize(env, expr + expr4 ); 
		
		for(int i = 0; i < R; i++){
			for(int j = 0; j < C; j++){
				cout << d[i][j] << " ";
			}
			cout << endl;
		} */
		
		for(int i = 0; i < N - 1; i++){
			
			for(int r = 0; r < R; r ++){
				for(int c = 0; c < C; c++){
					somatorio_2 += Y[y[i][r][c]] * d[r][c]; 
				}
			}
		}
		
		somatorio_2 *= beta; //peso do somatorio 2 
		
		fObj = IloMinimize(env, expr + somatorio_2);
				
		modelo.add(fObj);
		expr.end();
		somatorio_2.end();
	}
	
	// Criando as Restricoes do problema para o caso 2D
	void restricao(){
		
		IloExpr expr1(env);
		IloExpr expr2(env);
		
		// Restricao 2:  é a restrição de conservação de fluxo, onde T_ij é o elemento da matriz de transporte que representa o número de contêineres que embarcam no porto i com destino ao porto j.
		for(int i = 0; i < N - 1; i++){
			for(int j = i + 1; j < N; j++){
				
				//
				for(int v = i + 1; v < j + 1; v++){
				//	cout << "Entrou: j = " << j << " | v = "<< v << endl;
					for(int r = 0; r < R; r++){
						for(int c = 0; c < C; c++){
							expr1 += X[x[i][j][v][r][c]];
						}
					}
				}
				
				for(int k = 0; k < i; k++){
					for(int r = 0; r < R; r++){
						for(int c = 0; c < C; c++){
							expr1 -= X[x[k][j][i][r][c]];
						}
					}
				}
				
				modelo.add(expr1 == T[i][j]);
				expr1.clear();
			}
		} 
		
		// Restricao 3: 
		expr1.clear();
		for(int i = 0; i < N - 1; i++){	
			for(int r = 0; r < R; r++){
				for(int c = 0; c < C; c++){
					
					for(int k = 0; k < i + 1; k++){
						for(int j = i + 1; j < N; j++){
							for(int v = i + 1; v < j + 1; v++){
								expr1 += X[x[k][j][v][r][c]];
							}
						}
					}
					expr2 = Y[y[i][r][c]];
					modelo.add(expr1 - expr2 == 0);
					expr1.clear();
				}
			}
		}
		
		// Restricao 4: A restrição (4) é necessária para garantir que existem contêineres embaixo do contêiner que ocupa o compartimento (r, c)
		for(int i = 0; i < N - 1; i++){
			for(int r = 0; r < R - 1; r++){
				for(int c = 0; c < C; c++){
					//cout << " Porto: " << i  << " Linha: "<< r << " coluna:  " << c;
					expr1 = Y[y[i][r][c]];
					expr2 = Y[y[i][r + 1][c]];
					modelo.add(expr1 - expr2 >= 0 );
				}
				expr1.clear();
				expr2.clear();
			}
		}
		
		// Restrição 5
		expr1.clear();
		expr2.clear();
		for(int j = 1; j < N; j++){
			for(int r = 0; r < R - 1; r++){
				
				for(int c = 0; c < C; c++){
					
					// primeiro somatorio
					for(int i = 0; i <= j - 1; i++){	
						for(int p = j; p < N; p++){
							expr1 += X[x[i][p][j][r][c]];
						}	
					}
					
					//segundo somatorio
					for(int i = 0; i <= j - 1; i++){	
						for(int p = j + 1; p < N; p++){
							for(int v = j + 1; v <= p; v++){
								expr2 += X[x[i][p][v][r + 1][c]];
							}
						}
					}
					modelo.add(expr1 + expr2 <= 1);
					expr1.clear(); expr2.clear();	
				}
			}
		}
		
		// aplica a restrição que coloca os contêineres distribuidos compactamente
		if(estabilidade == 2){
			// Restricao de Estabilidade somente eh aplicar se o numero de colunas(C): C > 1
			/*if(C > 1){
				expr1.clear();
				//Restricao de Estabilidade Transversal
				for(int i = 0; i < N - 1; i ++){
					//lado esquerdo
					for(int c = 0; c < C/2; c++){
						for(int r = 0; r < R; r++){
							expr1 += Y[y[i][r][c]];
						}
					}
					
					//lado direito
					for(int c = C/2; c < C; c++){
						for(int r = 0; r < R; r++){
							expr1 -= Y[y[i][r][c]];
						}
					}
					modelo.add(expr1 <= C/2);
					modelo.add(expr1 >= -C/2);
				}
			} */
			
			expr1.clear();
			expr2.clear();
			//tentar colocar os conteineres mais ajustados
			for(int i = 0; i < N - 1; i++){
				for(int c = 0; c < C - 1; c++){
					
					for(int r = 0; r < R; r++){
						expr1 += Y[y[i][r][c]];
						//expr2 -= Y[y[i][r][c + 1]];
					}
				
					for(int cc = c + 1; cc < C; cc++){
						for(int r = 0; r < R; r++)
							expr2 -= Y[y[i][r][cc]];
					
						modelo.add(expr1 + expr2 <= 1);
						modelo.add(expr1 + expr2 >= -1);
						expr2.clear();
					}
					expr1.clear();
				}
			}
			
			
		}
		
		expr1.end(); expr2.end();
		modelo.add(X); modelo.add(Y);
		
		// No último porto o slot é vazio
//		for(int r = 0; r < R; r++)
//			for(int c = 0; c < C; c++)
//				Y[y[N - 1][r][c]].setBounds(0, 0);
//		//-----------------------------------
//		for(int i = 0; i < N; i++){
//			for(int j = 0; j < N; j++){
//				for(int v = 0; v < N; v++){
//					for(int r = 0; r < R; r++){
//						for(int c = 0; c < C; c++){
//							if( (i > j && v > j) || (i == j == v) || (i == j && (v > j || v < j)))
//								X[x[i][j][v][r][c]].setBounds(0, 0);
//						}
//					}
//				}
//			}
//		}
		
		//para o problema relaxado
		//relaxado.add(modelo);
		//relaxado.add(IloConversion(env, X, ILOFLOAT));
		//relaxado.add(IloConversion(env, Y, ILOFLOAT));
	}
	
	// Inicializando o modelo para o problema no Cplex
	void iniciarLP(){
		
		try{	
			modelo = IloModel(env); // resolvendo o problema binário
			relaxado = IloModel(env); // resolvendo o problema relaxado
			iniciarVariaveis();
			
			funcaoObjetivo();
			restricao();
			cplex = IloCplex (modelo);
			cplex_relaxado = IloCplex(relaxado);
			
		}catch(IloException &e){
			cerr << "Falha em iniciar, devido " << e.getMessage() << " \n";
		}
	} 
	
	// Criando o arquivo .lp
	void criarPPL(){
		cplex.exportModel("navio.lp");
	}
	
	// Resolvendo o problema
	void solvePPL(){
		cout << "\n ----- Inteiro ------ \n";
		_solvePPL(cplex); // para o problema inteiro
		//cout << "\n ----- Relaxado ------ \n";
		//_solvePPL(cplex_relaxado); // para o problema relaxado
	}
	
	// setando uma solucao incial
	void setSolucaoInicial(IloCplex & _cplex){
		
		ifstream entrada;
		entrada.open("solucaoSlot/solucaoInicial.txt");
		
		if(entrada.fail()){
			cout << "Erro durante leitura de arquivo com solução inicial!" << endl;
			throw(-1);
		}
		
		start_x = new int ****[N];
		for(int i = 0; i < N; i++){
			start_x[i] = new int ***[N];
			for(int j = 0; j < N; j++){
				start_x[i][j] = new int **[N];
				for(int v = 0; v < N; v++){
					start_x[i][j][v] = new int *[R];
					for(int r = 0; r < R; r++){
						start_x[i][j][v][r] = new int [C];
						for(int c = 0; c < C; c++){
							entrada >> start_x[i][j][v][r][c];
						}
					}
				}
			}
		}
		
		start_y = new int **[N];
		for(int i = 0; i < N; i++){
			start_y[i] = new int*[R];
			for(int r = 0; r < R; r++){
				start_y[i][r] = new int [C];
				for(int c = 0; c < C; c++){
					entrada >> start_y[i][r][c];
				}
			}			
		}
		
		
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);
		
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++){
				for(int v = 0; v < N; v++){
					for(int r = 0; r < R; r++){
						for(int c = 0; c < C; c++){		
							startVar.add(X[x[i][j][v][r][c]]);
							startVal.add(start_x[i][j][v][r][c]);
							
						}
					}
				}
			}
		}
		
		/*
    		Level 0 (zero) MIPStartAuto: Automatic, let CPLEX decide.
    		Level 1 (one) MIPStartCheckFeas: CPLEX checks the feasibility of the MIP start.
   		Level 2 MIPStartSolveFixed: CPLEX solves the fixed problem specified by the MIP start.
   		Level 3 MIPStartSolveMIP: CPLEX solves a subMIP.
    		Level 4 MIPStartRepair:CPLEX attempts to repair the MIP start if it is infeasible, according to the parameter that sets the frequency to try to repair infeasible MIP start, CPX_PARAM_REPAIRTRIES. 
    		
		MIPStartAuto = CPX_MIPSTART_AUTO	 
		MIPStartCheckFeas = CPX_MIPSTART_CHECKFEAS	 
		MIPStartSolveFixed = CPX_MIPSTART_SOLVEFIXED	 
		MIPStartSolveMIP = CPX_MIPSTART_SOLVEMIP	 
		MIPStartRepair = CPX_MIPSTART_REPAIR */
		
		//addMIPStart(IloNumVarArray vars=0, IloNumArray values=0, IloCplex::MIPStartEffort effort=MIPStartAuto, const char * name=0)
		
		cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartEffort(  CPX_MIPSTART_REPAIR ), "CPX_MIPSTART_REPAIR");
		
		startVal.clear();
		startVar.clear();
		
		for(int i = 0; i < N; i++){
			for(int r = 0; r < R; r++){
				for(int c = 0; c < C; c++){
		
					startVar.add(Y[y[i][r][c]]);
					startVal.add(start_y[i][r][c]);
							
				}
			}
		}
		
		cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartEffort(  CPX_MIPSTART_REPAIR ), "CPX_MIPSTART_REPAIR");
		startVal.end();
		startVar.end();
		
		// Deleting
		
		// desalocando a variavel start_x
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++){
				for(int v = 0; v < N; v++){
					for(int r = 0; r < R; r++)
						delete [] start_x[i][j][v][r];
				}
			}
		}
		
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++){
				for(int v = 0; v < N; v++){
					delete [] start_x[i][j][v];
				}
			}
		}
		
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++){
				delete [] start_x[i][j];
			}
		}
		
		for(int i = 0; i < N; i++)
			delete [] start_x[i];
		
		delete [] start_x;
		
		
		// desalocando a variavel start_y	
		for(int i = 0; i < N; i++)
			for(int r = 0; r < R; r++)
				delete [] start_y[i][r];
		
		for(int i = 0; i < N; i++)
			delete [] start_y[i];
		
		delete [] start_y;
	}
	
	// auxiliar para solve
	void _solvePPL(IloCplex &_cplex){
		
		_cplex.setParam(IloCplex::Threads, 16);
		
		//desativando os cortes
		//_cplex.setParam(IloCplex::Param::MIP::Cuts::Cliques, -1);
		//_cplex.setParam(IloCplex::Param::MIP::Pool::RelGap, 100);
		// armazena todas as solucções encontradas
		//_cplex.populate();
		//_cplex.use(Rounddown(env, Y));
		
		_cplex.setParam(IloCplex::Param::MIP::Limits::TreeMemory, 2048);
		
		setSolucaoInicial(_cplex);
		
		// Estipulando um limite de GAP e tempo para resolver o problema
		_cplex.use(timeLimitCallback(env, _cplex, IloFalse, _cplex.getCplexTime(), 7200, 1e-4));
	
		if(!_cplex.solve()){
			_cplex.out() << "erro ao otimizar !\n";
			throw(-1);
		}
	}
	
	// Mostra as soluções do problema relaxado
	void solucao(){
	
		cout << "\nSolução Inteira: \n";
		_solucao(cplex);
		
		//cout << "\nSolução Relaxada: \n";
		//_solucao(cplex_relaxado);
	}
	
	// Mostrando as solucoes
	void _solucao(IloCplex &_cplex){
		
		//GoalI g(_cplex);
		
		_cplex.out() << "Status: " << _cplex.getStatus() << endl;
		_cplex.out() << "Numero de Nos: " << _cplex.getNnodes() << endl;
		//_cplex.out() << "Numero de Cortes: " << _cplex.getNcuts(IloCplex::GoalI::getNcliques()) << endl;
		_cplex.out() << "Numero de Iteracoes: " << _cplex.getNiterations() << endl;
		_cplex.out() << "Funcao Objetivo (Re-handles number): " << _cplex.getObjValue() << endl;
		//_cplex.out() << "Tempo: " << (double) _cplex.getTime()<< endl;
		
		// ---------------------------------------------------------------
		// numero de soluções encontradas
		//int numsol = _cplex.getSolnPoolNsolns();
		/*env.out() << "Encontradas " << numsol << " soluções." << endl;
		// algumas soluções são delatadas devido
		
		int numsolreplaced = _cplex.getSolnPoolNreplaced();
		env.out() << numsolreplaced << " solutions were removed due to the "
		"solution pool relative gap parameter." << endl;
		
		// Get the average objective value of solutions in the solution pool
		env.out() << "O número médio do valor da função objetivo: " << _cplex.getSolnPoolMeanObjValue() << "." << endl << endl;
		
		for(int i = 0; i < numsol; i ++)
			cout << "F.O. da solucao " << i + 1 << " = " << _cplex.getObjValue(i) << endl;
		*///----------------------------------------------------------------
		
		cout << "Solucao: \n";
		valor = IloNumArray(env, size1);
		IloNumArray _valor(env, size2);
		
		_cplex.getValues(valor, X);
		_cplex.getValues(_valor, Y);
		
		//int cont = 0;
		
		//cout << "Contador: " << cont << endl;
		
		nRemanejosPorto = new int[N];
		int nRemanejos = 0, remanejoEmCadaPorto;
		
		for(int v = 0; v < N; v++){
			
			remanejoEmCadaPorto = 0;
			for(int i = 0; i < N; i++){
				for(int j = 0; j < N; j++){
					
					for(int r = 0; r < R; r++){
						for(int c = 0; c < C; c++){
						
							if(valor[x[i][j][v][r][c]]){
								
								if(i < j && v < j && v > i){
									cout << "Porto: " << i + 1 << "-" << j + 1 << "-" << v + 1 << "(" << r + 1 << ", " << c + 1 << ")" << endl;
									remanejoEmCadaPorto++; //N de remanejo no porto v
									nRemanejos++; // N de remanejos globais
									
									slotSolucao[v][r][c].remanejado = true;
									//slotSolucao[v][r][c].destino = j;
								}
							}	
							
						}
					}
				}
			}
			nRemanejosPorto[v] = remanejoEmCadaPorto;
			//cout << endl;
		}
		
		cout << "Rearranjos: " << nRemanejos << "\t| Movimentos: " << nRemanejos * 2 << endl;
		//cout << valor[x[0][2][1][0][20]] << endl;
		
		/*for(int v = 0; v < N; v++)
			if(nRemanejosPorto[v]){
				cout << "Porto " << v + 1 << " : " << nRemanejosPorto[v] << endl;
				
			}
		*/
		
		int movimentos = 0;
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++){
				for(int v = 0; v < N; v++){
					for(int r = 0; r < R; r++){
						for(int c = 0; c < C; c++){
							
							if(valor[x[i][j][v][r][c]] == 1){
								//cout << "x_" << i + 1 << j + 1 << v + 1 << "(" << r + 1 << "," << c + 1 << ") = " << 1 << "\t";
								if(!slotSolucao[i][r][c].ocupado){
									if(_valor[y[i][r][c]]){
										//slot esta preenchido
										slotSolucao[i][r][c].ocupado = true;
										//analisando o destino do contêiner que ocupa esse slot
										if(i < j && v == j)	
											slotSolucao[i][r][c].destino = v; // destino do conteiner
										//se foi movido em 'v', há rearranjo
										if(i < j && v < j){
											slotSolucao[i][r][c].remanejado = true;
											slotSolucao[i][r][c].destino = j;
				//							cout << "Porto: " << i + 1 << "-" << j + 1 << "-" << v + 1 << "(" << r + 1 << ", " << c + 1 << ")" << endl;
										}
									}
								}
								movimentos ++; //numero de movimentos realizados: colocar + retirar conteiner
							}
						}
					}
				}
			}
			//cout << endl;
		}
		
		// Devemos multiplicar por 2, para incluir o carregamento e o descarregamento
		cout << "Total de Movimentos: " << movimentos * 2 << endl;
		
		//imprimindo a configuração dos contêineres em cada porto
		imprimirSlot(_cplex, _valor, movimentos);
		
		//imprimindo solução em arquivo
		//guardarSolucao(slotSolucao);
		//_cplex.writeSolutions("opa.sol");
		
		//Mostrndo cada um das solucoes encontradas
		//for(int i = 0; i < numsol; i++){
			//_cplex.getValues(_valor, Y, numsol - 1);
			//imprime configuração dos slots ocupados
			
			//		X, 	Y
			imprimirSolucao(valor, _valor);
		//}
		
		
		
	}
	
	// imprime a instância e a matriz de Transporte
	void imprimir(void){
		
		cout << "Numero de portos: " << N << endl;
		cout << "Numero de linhas: " << R << endl;
		cout << "Numero de colunas: " << C << endl;
		
		/*cout << "Matriz de Transporte: \n";
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++)
				cout << T[i][j] << " ";
			cout << endl;
		} */
	}
	
	//imprime a solução em modo de visualização gráfica no Scilab
	//void guardarSolucao(IloNumArray _valor){
	void guardarSolucao(slot ***slotSolucao){
	
		//apagando o "txt" para acrescentar o "sce". Em 'txt', há três caracteres!
		for(int i = 0; i < 3 ; i++){
			instancia.erase(instancia.size() - 1);
		}
		//cout << "\nNome da instancia eh " << instancia << endl;
		
		// salvando o problema no arquivo de scilab
		if(estabilidade == 1)
			ooption = "SEM_ESTABILIDADE";
		else
			ooption = "COM_ESTABILIDADE";
			
		string sol = "solucaoScilab/result_" + aalfa + "_" + bbeta + "_" + ooption + "_" + instancia + "sce" ;
		ofstream outFile;
		outFile.open(sol.c_str());
		
		//--- primeiro exemplo
		outFile << "clf();\n";
		outFile << "ax=gca();\n";//obtendo o manipulador dos eixos correntes
		//int R3 = R * 3, C3 = C * 3; 
		outFile << "ax.data_bounds=[0,0;" << C  << ", " << R << "]; //set the data_bounds" << endl;
		outFile << "ax.box='on'; //desenha uma caixa" << endl;
		outFile << "a = 5  * ones(11,11); a(2:10,2:10)=34; a(6.5:7,6.5:7)=5;" << endl;
		outFile << "b = 1 * ones(11,11); b(2:10,2:10)=1; b(6.5:7,6.5:7)=1;" << endl;
		//outFile << "// primeira matriz no retângulo [1,1,3,3]\n";
		//outFile << "Matplot1(a,[0,0,3,3]) //conteiner no canto esquerdo inferior\n";
		//outFile << "Matplot1(a, [0,3,3,6]) //conteiner acima do que está no canto inferior esquerdo\n";
		//outFile << "Matplot1(a, [3,0,6,3]) //conteiner à direita do que está no canto inferior esquerdo\n";
		//outFile << "Matplot1(a, [3,3,6,6]) // (x1, y1, x2, y2);\n";
		
		
		//outFile << "porto = input('Entre com o porto')\n";
		outFile << "L = list();\n";
		for(int i = 0; i < N - 1; i++){
			
		//	outFile << "\nif(porto == " << i + 1 << ")" << endl;
			outFile << "//Porto " << i + 1 << endl;
		//		outFile << "	clf();\n";
			outFile << "S = [";
			
			//for(int r = R - 1; r >= 0  ; r--){
			for(int r = 0; r < R; r++){
				for(int c = 0; c < C; c++){
					//cout << _valor[y[i][r][c]];
					if(slotSolucao[i][r][c].ocupado){
						if(slotSolucao[i][r][c].remanejado)
							outFile << "100";
						else
							outFile << (slotSolucao[i][r][c].destino + 1);
					}
					else
						outFile << 0;
					//	cout << "y_" << i + 1 << "("<< r + 1 << ", " << c + 1 << ") = "<< 1 << endl;
					
					if(c != C - 1)
						outFile << " ";
					else
						outFile << ";";
				}
				//outFile << endl;
			}
			outFile << "];" << endl;
			//cout << endl;
			
			//salvando as matrizes na lista
			outFile << "L(" << i + 1 <<") = S;\n" << endl;
		}
		
		/*outFile << "for k = 1:" << N - 1 << endl;
		outFile << "	Matplot(L(k));" << endl;
		outFile << "	sleep(2000);" << endl;
		outFile << "	clf();" << endl;
		outFile << "end" << endl; */
		
		outFile << "for k = 1:" << N - 1 << endl;
		outFile << "	gcf();" << endl;
		outFile << "	for i = " << R - 1 << ": -1:0" << endl;
		outFile << "		for j = " << C - 1 << ":-1:0" << endl;
		outFile << "			if(L(k)(i+1,j+1) <> 0)" << endl;
		outFile << "				a(6.5:7,6.5:7) = L(k)(i + 1, j + 1);" << endl;
		//outFile << "				A = [j, i, j + 1, i + 1];" << endl;
		outFile << "				Matplot1(a, [j, i, j + 1, i + 1]);\n"; // (x1, y1, x2, y2);" << endl;
		outFile << "			else" << endl;
		//outFile << "				b(6.5:7,6.5:7) = 50;"  << endl;
		outFile << "				Matplot1(b, [j, i, j + 1, i + 1]);" << endl;
		outFile << "			end" << endl;
		outFile << "		end" << endl;
		outFile << "	end" << endl;
		outFile << "	sleep(2000);\n";
		//outFile << "	titlepage(k)\n;"
		outFile << "end" << endl;
		outFile << "clf(gcf(), 'reset')\n";
	
	}
	
	// Imprime os slot preenchidos e não preenchidos de uma solução encontrada, para cada porto
	void imprimirSolucao(IloNumArray & valor, IloNumArray &_valor){
		
		ofstream solucaoInicial;
		
		//Para armazenar uma solucao
		string arq = "solucaoSlot/solucaoInicial.txt";
		solucaoInicial.open(arq.c_str());
		
		for(int i = 0; i < N; i++){
			for(int j = 0; j < N; j++){	
				for(int v = 0; v < N; v++){
					for(int r = 0; r < R; r++){
						for(int c = 0; c < C; c++){
							if(valor[x[i][j][v][r][c]]){
								
								if(c < C - 1)
									solucaoInicial << "1 ";
								else
									solucaoInicial << "1";
							}else{
								if(c < C - 1)
									solucaoInicial << "0 ";
								else
									solucaoInicial << "0";
							}
								 
						}
						solucaoInicial << endl;
					}
				}
			}
		}
		
		for(int i = 0; i < N; i++){
			cout << "Porto: " << i + 1 << endl;
			
			if(i < N - 1){
				cout << "Número de slots ocupados: " << nConteiner[i] << endl;
				if(i > 0) // no porto inicial nao há rearranjos
					cout << "Número de rearranjos: " << nRemanejosPorto[i] << endl;
			}else
				cout << "Número de slots ocupados: " << 0 << endl;
				
			for(int r = R - 1; r >= 0; r--){
				for(int c = 0; c < C; c++){
					//cout << _valor[y[i][r][c]];
					if(_valor[y[i][r][c]]){
						printElemento("*", 1);//cout << "*";
						
						if(c < C - 1)
							solucaoInicial << "1 ";
						else
							solucaoInicial << "1";
					}else{
						printElemento("-", 1);//cout << "-";
						
						if(c < C - 1)
							solucaoInicial << "0 ";
						else
							solucaoInicial << "0";
					}
					//	cout << "y_" << i + 1 << "("<< r + 1 << ", " << c + 1 << ") = "<< 1 << endl;
				}
				cout << endl;
				solucaoInicial << endl;
			}
			//cout << endl;
		}
		
	}
	
	// Imprime a configuração do slot de uma solução encontrada, para cada porto
	void imprimirSlot(IloCplex &_cplex, IloNumArray & _valor, int movimentos){
		
		// salvando o problema no arquivo de scilab
		if(estabilidade == 1)
			ooption = "SEM_ESTABILIDADE";
		else
			ooption = "COM_ESTABILIDADE";
		
		string arq = "solucaoSlot/result_" + aalfa + "_" + bbeta + "_" + ooption + "_" + instancia;
		
		ofstream out;
		out.open(arq.c_str());
		
		// --- Informações da Instância ---- //
		out << "Informações: " << endl;
		out << "Valor de alfa: " << alfa << endl;
		out << "Valor de beta: " << beta << endl;

		if(estabilidade == 1)
			out << "Sem REGRA de ESTABILIDADE " << endl; 
		else
			out << "Com REGRA de ESTABILIDADE " << endl;
		
		// --------- Salvando -------------///
		for(int i = 0; i < N; i++){
		//	cout << "Porto: " << i + 1 << endl;
			out << "\nPorto: " << i + 1 << endl;
			
			if(i < N - 1){
				out << "Número de slots ocupados: " << nConteiner[i] << endl;
				if(i > 0){ // no porto inicial nao há rearranjos
		//			cout << "Número de rearranjos: " << nRemanejosPorto[i] << endl;
					out << "Número de rearranjos: " << nRemanejosPorto[i] << endl;
				}	
			}else
				out << "Número de slots ocupados: " << 0 << endl;
				
			for(int r = R - 1; r >= 0; r--){
				for(int c = 0; c < C; c++){
					//Verificando se o slot foi ocupado
					if(slotSolucao[i][r][c].ocupado){
						//se o destino dele for para o porto seguinte ... ok!
						if(!slotSolucao[i][r][c].remanejado){
							if(slotSolucao[i][r][c].destino == i + 1){
		//						printElemento(slotSolucao[i][r][c].destino + 1, 2);
								out << slotSolucao[i][r][c].destino + 1;
							}
							else{
								//caso o destino não seja o porto seguinte, devemos salvar a posição para os portos seguintes até que chego no seu destino. Devemos fazer isso, pois perdemos essa informação
							
								//armazenado o porto do destino
								int porto = slotSolucao[i][r][c].destino;
								/* Devemos preencher o slot, pois do porto corrente até o porto antes do destino (no porto do destino, ele é descarregado, logo seu valor é zero), essa posição é preenchida pelo mesmo conteiner, caso não haja rearranjo
								*/
								for(int ii = i; ii < porto; ii++)
									slotSolucao[ii][r][c] = {true, porto, false};
							
		//						printElemento(slotSolucao[i][r][c].destino + 1, 2);
								out << slotSolucao[i][r][c].destino + 1;
							}
						}
						else{
		//					printElemento(slotSolucao[i][r][c].destino + 1, 2);
							out << slotSolucao[i][r][c].destino + 1;	
						}
					}
					else{
		//				printElemento("-", 2);
						out << "0";
					//	cout << "y_" << i + 1 << "("<< r + 1 << ", " << c + 1 << ") = "<< 1 << endl;
					}
				}
		//		cout << endl;
				out << endl;
			}
			//cout << endl;
		}
		
		
		//Conta o numero de rearranjos
		int nRemanejos;
		out << endl;
		for(int v = 0; v < N; v++){
			
			for(int i = 0; i < N; i++){
				for(int j = 0; j < N; j++){
					
					for(int r = 0; r < R; r++){
						for(int c = 0; c < C; c++){
						
							if(valor[x[i][j][v][r][c]]){
								
								if(i < j && v < j && v > i){
									out << "Porto: " << i + 1 << " -> " << j + 1 << "\t| Rearranjo: " << v + 1 << "\t| Slot: (" << r + 1 << ", " << c + 1 << ")" << endl;
									nRemanejos++; // N de remanejos globais
								}
							}	
							
						}
					}
				}
			}
			//cout << endl;
		}
		
		out << "Rearranjos: " << nRemanejos << "\t| Movimentos: " << nRemanejos * 2 << endl;
		
		// Imprimindo informações do CPLEX
		out << "\n-------------------------------" << endl;
		out << "Status: " << _cplex.getStatus() << endl;
		out << "Numero de Nos: " << _cplex.getNnodes() << endl;
		out << "Numero de Iteracoes: " << _cplex.getNiterations() << endl;
		out << "Funcao Objetivo: " << _cplex.getObjValue() << endl;
		out << "Numeto Total de Movimentos Realizados: " << movimentos * 2 << endl;
		out << "Gap: " << _cplex.getMIPRelativeGap() * 100 << "%" << endl;
		out << "Tempo: " << _cplex.getTime() << " s"<< endl;
		
		// Calculo da F.O. Estabilidade
		float est = 0.0;
		for(int i = 0; i < N; i++){
			for(int r = 0; r < R; r++){
				for(int c = 0; c < C; c++){
					if(_valor[y[i][r][c]])
						est += d[r][c];
				}
			}
		} 
		
	//	cout << "Estabilidade: " << est << endl;
		out << "Estabilidade: " << est << endl;
	}
	
	template <typename T>
	void printElemento(T t, const int& width){
		cout << left << setw(width) << setfill(' ') << t;
	}
	
};

// Funcao principal
int main(int argn, const char** argc){
	
	// Testando o numero de argumentos passados
	if(argn < 2){
		cout << "Poucos argumentos passados !\n";
		cout << "Tente: " << argc[0] << " instance-file \n";
		return EXIT_SUCCESS;
	}
	
	// se alfa e beta forem diferente
	if( (atof(argc[2]) + atof(argc[3])) != 1 ){
		
		cout << "alfa + beta != 1" << endl;
		cout << "Tente novamente! " << endl;
		return EXIT_SUCCESS;
	}
	
	try{
		int option;
		float alfa, beta;
		
		/*while(true){
			
			//system("clear");
			cout << "Resolver problema " << argc[1] << " :" << endl;
			
			cout << "Escalares da Função Objetivo: " << endl;
			cout << "alfa + beta <= 1" << endl;
			cout << "alfa: ";
			// cin >> alfa;
			cout <<  "beta: ";
			// cin >> beta;
			
			cout << "[1] - Sem regras de Estabilidade" << endl;
			cout << "[2] - Com regras de Estabilidade" << endl;
			cout << "Entre com o valor entre colchetes: ";
			//cin >> option;
			//alfa = 0.5; beta = 0.5; option = 1;
*/
			alfa = atof( argc[2] );
			cout << "Valor de alfa: " << alfa << endl;
			cout << "Valor de beta: " << argc[3] << endl;
			option = atoi( argc[4] );
			if(option == 1)
				cout << "Sem Regras de Estabilidade" << endl;
			else
				cout << "Com Regras de Estabilidade" << endl;
			
			/*if((option == 1 || option == 2) && ( 0 <= alfa <= 1 ) && (0 <= beta <= 1) && (alfa + beta <= 1))
				break;
			else{
				if(alfa + beta > 1){
					cout << "Soma dos escalares é maior que 1! Tente novamente ..." << endl;
				}
			}
		}*/
		
		//Instanciando o problema
		//Problema *conteiner = new Problema(argc[1], alfa, beta, option);
		
		//os argumentos sao: nome_instancia, alfa, beta, com ou sem regra de estabilidade
		Problema *conteiner = new Problema(argc[1], argc[2], argc[3], argc[4]);
		
		// Verificando se a instancia tem solucao viavel
		if(!conteiner->factivel())
			return EXIT_SUCCESS;
		
		//if(option == 1) {
		//	ofstream LogFile("result_without_stability.txt");
		//	conteiner->cplex.setOut(LogFile);
		//} else {
		//	ofstream LogFile("result_with_stability.txt");
		//	conteiner->cplex.setOut(LogFile);
		//}

		conteiner->imprimir(); //imprimindo a instancia
		conteiner->iniciarLP(); // imprimindo o lp do problema
		
		conteiner->criarPPL();
		cout << "ppl escrito em navio.lp" << endl;
		
		conteiner->solvePPL(); //solving the problem
		conteiner->solucao();
		
		delete conteiner;

	}catch(IloException &e){
		cout << "Erro do tipo:" << e.getMessage() << " .\n";
	}
	
	return EXIT_SUCCESS;
}
