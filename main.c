#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/types.h>

void print_debug_tree(){
    char cmd[50];
    sprintf(cmd, "pstree -lp %d", getpid());
    system(cmd);
}

void __kill(pid_t c, int sig){
    usleep(400);
    kill(c, sig);
}

void signal_handler(int s) {

}

int main(int argc, char**argv){
    pid_t root = getpid();

    if(argc < 2){
        printf("Uso: %s N\n", argv[0]);
        return EXIT_FAILURE;
    }
    int N = atoi(argv[1]);
    if (N <= 0){
        printf("N debe ser > 0\n");
        return EXIT_FAILURE;
    }

    int n_hijos = 2;
    int idx;
    pid_t child;

    signal(SIGUSR1, signal_handler);

    pid_t *pids = (pid_t*)malloc(sizeof(pid_t)*n_hijos);

    // Creación de H1 y H2; en idx==0 y idx==n_hijos-1 se crea nieto
    for(idx = 0; idx < n_hijos; idx++){
        pids[idx] = fork();
        if(pids[idx] == 0){
            if(idx == 0 || idx == (n_hijos - 1)){
                child = fork();   // child>0 soy padre del nieto; child==0 soy el nieto
                break;
            }else{
                break;
            }
        }
    }

    if(root == getpid()){
        print_debug_tree();

        // ciclo de N repeticiones
        printf("Padre [%d]\n", getpid());
        for (int r = 0; r < N; r++){
            // padre arranca y pasa a H2
            __kill(pids[n_hijos - 1], SIGUSR1);

            // Espera de regreso la señal desde H1
            pause();
            printf("Padre [%d]\n", getpid());
        }

        for(int i = 0; i < n_hijos; i++){
            wait(NULL); // espera a H1 y H2
        }
    } else {
        // cada proceso repite N veces
        for (int r = 0; r < N; r++){
            pause();

            if(idx == (n_hijos - 1)){             // ***** H2 y H21 *****
                if(child > 0){                    // H2
                    printf("Hijo2 [%d]\n", getpid());
                    __kill(child, SIGUSR1);       // -> H21
                    pause();                      // espera regreso de H21
                    printf("Hijo2 [%d]\n", getpid());
                    __kill(pids[idx-1], SIGUSR1); // -> H1
                } else {                           // H21
                    printf("Hijo21 [%d]\n", getpid());
                    __kill(getppid(), SIGUSR1);   // -> H2
                }
            } else if(idx == 0){                   // ***** H1 y H11 *****
                if(child > 0){                    // H1
                    printf("Hijo1 [%d]\n", getpid());
                    __kill(child, SIGUSR1);       // -> H11
                    pause();                      // espera regreso de H11
                    printf("Hijo1 [%d]\n", getpid());
                    __kill(getppid(), SIGUSR1);   // -> Padre
                } else {                           // H11
                    printf("Hijo11 [%d]\n", getpid());
                    __kill(getppid(), SIGUSR1);   // -> H1
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
