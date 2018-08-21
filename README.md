<!-- TODO:  -->
# Aulas da disciplina Laboratório de Introdução à Programação Paralela 
### Ementa: Práticas em laboratório para introdução aos conceitos básicos de programação paralela. Modelos e ambientes para programação paralela. Corretude da execução concorrente em sistemas de memória compartilhada. Técnicas de paralelização. Algoritmos paralelos. Avaliação de desempenho em computação paralela. 

Aula baseada nos slides em https://www.openmp.org/wp-content/uploads/omp-hands-on-SC08.pdf. [O arquivo ](./OpenMP-4.0-C.pdf) contém uma Quick Reference Card de OpenMP 4.0.

A maior parte dos códigos entregues na aula anterior tinham um erro que gerava execuçòes incorretas: o valor de Pi era diferente em cada execução. 

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

private(var) cria uma nova cópia local de var para cada thread. Mas preste atenção a que:
– **O valor NÃO está inicializado**
– Em OpenMP 2.5 o valor da variável compartilhada está indefinido após a região.
Boa prática! Declare todos seus dados privador por default!
```cpp
#pragma omp parallel for default(shared) private(c,eps)
```
Ainda temos problemas. Como inicializarmos variáveis privadas?
```cpp
void wrong() { 
  int tmp = 0;
  #pragma omp for private(tmp) 
  for (int j = 0; j < 1000; ++j)
	tmp+=j; //tmp não foi inicializado!
  printf(“%d\n”, tmp); //O valor da variável na thread master tmp: 0 em OpenMP 3.0, nao especificado em 2.5
}
```
Outro exemplo:

```cpp
extern int tmp; 
void work() {
  tmp = 5; //indefinido qual copia de tmp
}

int tmp;
void danger() {
  tmp = 0;
  #pragma omp parallel private(tmp)
    work(); 
    printf(“%d\n”, tmp);//tmp tem valor indefinido
}
```
Firstprivate : caso especial de private.
– Inicializa cada variável privada com o valor correspondente na thread **master**

Exemplo: cada cópia da variável incr é inicializada com 0.
```cpp
incr = 0;
#pragma omp parallel for firstprivate(incr)
for (i = 0; i <= MAX; i++) {
  if ((i%2)==0) incr++;
    A[i] = incr;
}
```
E como copiar de volta um valor de uma variável privada para a variável da thread master?
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
#pragma omp parallel for firstprivate(tmp) lastprivate(tmp)
for (int j = 0; j < 1000; ++j)
	tmp += j; 
	printf(“%d\n”, tmp);//tmp is defined as its value at the “last sequential” iteration (i.e., for j=999)
}
```


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
Exercício: modifique o programa da aula anterior para evitar condições de corrida


[Vamos no slide 55 da Intro to OpenMP](./Intro_To_OpenMP_Mattson.pdf)

Nota: para determinar o tamanho da linha de cache digite:
```
cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_l
ine_size
```
