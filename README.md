<!-- TODO:  -->
# Aulas da disciplina Laboratório de Introdução à Programação Paralela 
### Ementa: Práticas em laboratório para introdução aos conceitos básicos de programação paralela. Modelos e ambientes para programação paralela. Corretude da execução concorrente em sistemas de memória compartilhada. Técnicas de paralelização. Algoritmos paralelos. Avaliação de desempenho em computação paralela. 

Aula baseada nos slides em https://www.openmp.org/wp-content/uploads/omp-hands-on-SC08.pdf. [O arquivo ](./OpenMP-4.0-C.pdf) contém uma Quick Reference Card de OpenMP 4.0.

A maior parte dos cõdigos entregues na aula anterior tinham um erro que gerava execuçòes incorretas: o valor de Pi era diferente em cada execução. 

```cpp
#pragma omp parallel
	  {
	  	double partial = 0.0;
	  	#pragma omp for
			for (i=1;i<= num_steps; i++){
			  x = (i-0.5)*step;
			  partial += 4.0/(1.0+x*x);
			}
            ...
 ```           
O erro está no uso da variável compartilhada **x** sem proteção por todas as threads dentro da região paralela. Para entender o problema, vamos estudar o ambiente de dados em OpenMP.
No modelo de programação com memória compartilhada de OpenMP as variáveis são **compartilhadas por default**.
* As variáveis compartilhadas são SHARED para todas as threads. Em C: File scope variables, static. Em C e FORTRAN: dynamically allocated memory (ALLOCATE, malloc, new)
* Variáveis da pilha (locais a funções) são privadas: (PRIVATE). Variáveis automaticas são PRIVATE.(Em C, todas as variáveis declaradas dentro de um bloco de código são automáticas por padrão.)

 Data sharing: Exemplos:
 ```cpp
double A[10]; 
int main() { 
	int index[10];

	#pragma omp parallel 
		work(index);
	printf(“%d\n”, index[0]); 
}

extern double A[10]; 
void work(int *index) {
	double temp[10]; 
	static int count; 
	...
}
```
**A, index e count** são compartilhadas por todas as threads. 
**temp** é local a cada thread

Exemplo:
```cpp
void simple(int n, float *a, float *b)
{
int i;
#pragma omp parallel for
    for (i=1; i<n; i++) /* i is private by default */
}

```
Alterando os atributos de armazenamento:
É possível alterar seletivamente atributos de armazenamento para construções usando as seguintes cláusulas:
– SHARED
– PRIVATE
– FIRSTPRIVATE
(Todas as cláusulas aplicam-se à construção do OpenMP, NÃO para toda a região.)
O valor final de um private dentro de um loop paralelo pode ser transmitido para a variável compartilhada fora do loop com:
– LASTPRIVATE
Os atributos padrão podem ser substituídos com:
– DEFAULT (PRIVATE | SHARED | NONE)

private(var) cria uma nova cópia local de var para cada thread. 
– The value is uninitialized
– In OpenMP 2.5 the value of the shared variable is undefined after the region
```cpp
void wrong() { 
  int tmp = 0;
#pragma omp for private(tmp) 
  for (int j = 0; j < 1000; ++j)
	tmp+=j; //tmp não inicializado
	printf(“%d\n”, tmp); //tmp: 0 em OpenMP 3.0, nao especificado em 2.5
}
```

```cpp
int tmp;
void danger() {
	tmp = 0;
#pragma omp parallel private(tmp)
	work(); 
	printf(“%d\n”, tmp);//tmp tem valor indefinido
}
```
```cpp
extern int tmp; 
void work() {
	tmp = 5; //indefinido qual copia de tmp
}
```
Firstprivate : caso especial de private.
– Inicializa cada variável privada com o valor correspondente na thread **master**

```cpp
void useless() { 
	int tmp = 0;
	#pragma omp for firstprivate(tmp) 
	for (int j = 0; j < 1000; ++j) //Cada thread recebe sua propria copia de tmp, valor inicial 0
		tmp += j;
	printf(“%d\n”, tmp);//tmp: 0 em 3.0, indefinido em 2.5
}
```
Lastprivate passa o valor de uma private da última iteração para uma variável global.
```cpp
void closer() { 
	int tmp = 0;
//cada thread recebe sua própria copia de tmp
#pragma omp parallel for firstprivate(tmp) \ 
lastprivate(tmp)
for (int j = 0; j < 1000; ++j)
	tmp += j; 
	printf(“%d\n”, tmp);//tmp is defined as its value at the “last sequential” iteration (i.e., for j=999)
}
```
Consider this example of PRIVATE and FIRSTPRIVATE variables A,B, and C = 1
#pragma omp parallel private(B) firstprivate(C)
 Are A,B,C local to each thread or shared inside the parallel region?
 What are their initial values inside and values after the parallel region?
Inside this parallel region ...
 “A” is shared by all threads; equals 1   “B” and “C” are local to each thread.
– B’s initial value is undefined – C’s initial value equals 1
Outside this parallel region ...
 The values of “B” and “C” are unspecified in OpenMP 2.5, and in OpenMP 3.0 if referenced in the region but outside the construct.
Note that the default storage attribute is DEFAULT(SHARED) (so no need to use it)
 Exception: #pragma omp task
 To change default: DEFAULT(PRIVATE)
 each variable in the construct is made private as if specified in a private clause
 mostly saves typing
 DEFAULT(NONE): no default for variables in static extent. Must list storage attribute for each variable in static extent. Good programming practice!
Only the Fortran API supports default(private). C/C++ only has default(shared) or default(none).

 Data Copying: Copyprivate
Used with a single region to broadcast values of privates from one member of a team to the rest of the team.
```cpp
#include <omp.h>
void input_parameters (int, int); // fetch values of input parameters
void do_work(int, int);

void main()
{
	int Nsize, choice;	
	#pragma omp parallel private (Nsize, choice)
	{
	  #pragma omp single copyprivate (Nsize, choice)
		input_parameters (Nsize, choice);
	  do_work(Nsize, choice);
	}
}
```


[Vamos no slide 55 da Intro to OpenMP](./Intro_To_OpenMP_Mattson.pdf)

## Afinidade / AFFINITY ##
A afinidade de threads (Thread affinity ) restringe a execução de determinadas threads (unidades de execução virtual) a um subconjunto das unidades de processamento físico em um computador com multiprocessador. Dependendo da topologia da máquina, a afinidade de threads pode ter um efeito dramático na velocidade de execução de um programa.
Isso minimiza a migração de threads e o custo da troca de contexto entre os núcleos (cores). Ela também melhora a localidade dos dados e reduz o tráfego de coerência de cache entre os núcleos (ou processadores).
OpenMP tem a capacidade de vincular (bind) threads OpenMP a unidades de processamento físico. A interface é controlada usando uma variável de ambiente cujo nome depende do compilador em uso. Ex. para Intel use KMP_AFFINITY.
https://gcc.gnu.org/onlinedocs/libgomp/GOMP_005fCPU_005fAFFINITY.html#GOMP_005fCPU_005fAFFINITY

Exercício: [Multiplicação de matrizes](./profiling)
