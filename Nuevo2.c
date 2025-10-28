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
    // Handler vacío, solo para despertar de pause()
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

    int n_hijos = 2;  // H1 y H2
    int idx;
    pid_t child1, child2;  // Para los nietos de cada hijo
    pid_t grandchild;      // Para el bisnieto H112

    signal(SIGUSR1, signal_handler);

    pid_t *pids = (pid_t*)malloc(sizeof(pid_t) * n_hijos);

    // ========== CREACIÓN DE LA JERARQUÍA DE PROCESOS ==========
    for(idx = 0; idx < n_hijos; idx++){
        pids[idx] = fork();
        if(pids[idx] == 0){
            // Estoy en H1 o H2
            // Cada hijo crea 2 nietos
            child1 = fork();
            if(child1 == 0){
                // Estoy en primer nieto (H11 o H21)
                if(idx == 0){
                    // H11 crea su propio hijo H112
                    grandchild = fork();
                    if(grandchild == 0){
                        // Soy H112 (bisnieto)
                        break;
                    }
                }
                break;
            }
            
            child2 = fork();
            if(child2 == 0){
                // Estoy en segundo nieto (H12 o H22)
                break;
            }
            // Si llegó aquí, soy H1 o H2 (padre de los nietos)
            break;
        }
    }

    // ========== ESQUEMA DE COMUNICACIÓN CON SEÑALES ==========
    if(root == getpid()){
        // ===== PROCESO PADRE =====
        print_debug_tree();
        printf("Padre [%d]\n", getpid());
        
        for (int r = 0; r < N; r++){
            // Padre inicia enviando a H2
            __kill(pids[1], SIGUSR1);
            
            // Espera señal de regreso desde H1
            pause();
            printf("Padre [%d]\n", getpid());
        }

        // Esperar a todos los hijos directos
        for(int i = 0; i < n_hijos; i++){
            wait(NULL);
        }
        
    } else if(idx == 1 && child1 > 0 && child2 > 0){
        // ===== PROCESO H2 =====
        for (int r = 0; r < N; r++){
            pause();  // Espera señal del Padre
            printf("Hijo2 [%d]\n", getpid());
            
            __kill(child2, SIGUSR1);  // Envía a H22 primero
            pause();  // Espera regreso de H22
            
            printf("Hijo2 [%d]\n", getpid());
            __kill(child1, SIGUSR1);  // Envía a H21
            pause();  // Espera regreso de H21
            
            printf("Hijo2 [%d]\n", getpid());
            __kill(pids[0], SIGUSR1);  // Envía a H1
        }
        wait(NULL); wait(NULL);  // Espera a H21 y H22
        
    } else if(idx == 1 && child1 == 0 && child2 > 0){
        // ===== PROCESO H21 =====
        for (int r = 0; r < N; r++){
            pause();  // Espera señal de H2
            printf("Hijo21 [%d]\n", getpid());
            __kill(getppid(), SIGUSR1);  // Regresa a H2
        }
        
    } else if(idx == 1 && child2 == 0){
        // ===== PROCESO H22 =====
        for (int r = 0; r < N; r++){
            pause();  // Espera señal de H2
            printf("Hijo22 [%d]\n", getpid());
            __kill(getppid(), SIGUSR1);  // Regresa a H2
        }
        
    } else if(idx == 0 && child1 > 0 && child2 > 0){
        // ===== PROCESO H1 =====
        for (int r = 0; r < N; r++){
            pause();  // Espera señal de H2
            printf("Hijo1 [%d]\n", getpid());
            
            __kill(child2, SIGUSR1);  // Envía a H12 primero
            pause();  // Espera regreso de H12
            
            printf("Hijo1 [%d]\n", getpid());
            __kill(child1, SIGUSR1);  // Envía a H11
            pause();  // Espera regreso de H11
            
            printf("Hijo1 [%d]\n", getpid());
            __kill(getppid(), SIGUSR1);  // Regresa a Padre
        }
        wait(NULL); wait(NULL);  // Espera a H11 y H12
        
    } else if(idx == 0 && child1 == 0 && grandchild > 0){
        // ===== PROCESO H11 =====
        for (int r = 0; r < N; r++){
            pause();  // Espera señal de H1
            printf("Hijo11 [%d]\n", getpid());
            
            __kill(grandchild, SIGUSR1);  // Envía a H112
            pause();  // Espera regreso de H112
            
            printf("Hijo11 [%d]\n", getpid());
            __kill(getppid(), SIGUSR1);  // Regresa a H1
        }
        wait(NULL);  // Espera a H112
        
    } else if(idx == 0 && child1 == 0 && grandchild == 0){
        // ===== PROCESO H112 =====
        for (int r = 0; r < N; r++){
            pause();  // Espera señal de H11
            printf("Hijo112 [%d]\n", getpid());
            __kill(getppid(), SIGUSR1);  // Regresa a H11
        }
        
    } else if(idx == 0 && child2 == 0){
        // ===== PROCESO H12 =====
        for (int r = 0; r < N; r++){
            pause();  // Espera señal de H1
            printf("Hijo12 [%d]\n", getpid());
            __kill(getppid(), SIGUSR1);  // Regresa a H1
        }
    }

    free(pids);
    return EXIT_SUCCESS;
}
