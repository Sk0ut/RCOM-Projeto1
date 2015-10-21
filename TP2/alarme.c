#include <unistd.h>
#include <signal.h>
#include <stdio.h>

int counter = 0;
int flag = 1;
int foo = 0;

#define ATTEMPTS 10
#define TIMEOUT 3

inline void desativa_alarme(){
	alarm(0);
}

inline void ativa_alarme(){
	alarm(TIMEOUT);
}

void atende(){                   // atende alarme
	printf("Attempt #%d...\n", counter);
	flag=1;
	counter++;
}

int main() {
	(void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao
	while(counter < ATTEMPTS){
		if(flag){
      		alarm(3);                 // activa alarme de 3s
      		flag=0;
  		}
	}
	printf("Exceeded number of attempts. Ending... \n");
	return 0;
}