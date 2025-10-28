#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/types.h>

void print_debug_tree(){
    char cmd[80];
    sprintf(cmd, "pstree -lp %d", getpid());
    system(cmd);
}

void __kill(pid_t c, int sig){
    usleep(400);   // pequeño colchón para evitar condiciones de carrera bruscas
    kill(c, sig);
}

void signal_handler(int s){ (void)s; }

int main(int argc, char**argv){

    pid_t root = getpid();

    // =========================
    // CAMBIO: ahora el único argumento es N (repeticiones de la secuencia)
    // =========================
    if(argc < 2){
        printf("Uso: %s N\n", argv[0]);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    if (N <= 0){
        printf("N debe ser > 0\n");
        return EXIT_FAILURE;
    }

    // =========================
    // Fijamos 2 hijos (H1 y H2)
    // =========================
    int n_hijos = 2;
    int idx;
    pid_t child = -1;   // lo usaremos para primer fork interno
    pid_t child2 = -1;  // segundo fork interno
    pid_t child3 = -1;  // bisnieto en la rama H11 -> H112

    // vamos a necesitar conocer sub-pids dentro de cada rama
    pid_t h11 = -1, h12 = -1, h112 = -1;
    pid_t h21 = -1, h22 = -1;

    signal(SIGUSR1, signal_handler);

    pid_t *pids = (pid_t*)malloc(sizeof(pid_t)*n_hijos);

    // =========================
    // Creamos H1 (idx=0) y H2 (idx=1)
    // =========================
    for(idx = 0; idx < n_hijos; idx++){
        pids[idx] = fork();
        if(pids[idx] == 0){
            // Estamos dentro de H1 o H2 según idx

            // -------------------------
            // Rama H1 (idx == 0):
            //   crea H11
            //      H11 crea H112
            //   crea H12
            // -------------------------
            if(idx == 0){

                // fork para H11
                child = fork();
                if(child == 0){
                    // ===== Estoy en H11 =====
                    // H11 crea H112
                    child3 = fork();
                    if(child3 == 0){
                        // === Estoy en H112 ===
                        signal(SIGUSR1, signal_handler);

                        for (int r = 0; r < N; r++){
                            pause(); // señal desde H11
                            printf("Hijo112 [%d]\n", getpid());
                            __kill(getppid(), SIGUSR1); // regreso -> H11
                        }

                        return EXIT_SUCCESS;
                    } else {
                        // seguimos en H11
                        h112 = child3;
                        signal(SIGUSR1, signal_handler);

                        for (int r = 0; r < N; r++){
                            pause(); // señal desde H1
                            printf("Hijo 11 [%d]\n", getpid());
                            __kill(h112, SIGUSR1); // -> H112

                            pause(); // espera vuelta desde H112
                            printf("Hijo 11 [%d]\n", getpid());
                            __kill(getppid(), SIGUSR1); // -> H1
                        }

                        // esperamos a H112 al final
                        wait(NULL);
                        return EXIT_SUCCESS;
                    }

                } else {
                    // sigo en H1 (padre de H11)
                    h11 = child;

                    // fork para H12
                    child2 = fork();
                    if(child2 == 0){
                        // ===== Estoy en H12 =====
                        signal(SIGUSR1, signal_handler);

                        for (int r = 0; r < N; r++){
                            pause(); // señal desde H1
                            printf("Hijo 12 [%d]\n", getpid());
                            __kill(getppid(), SIGUSR1); // regreso -> H1
                        }

                        return EXIT_SUCCESS;

                    } else {
                        // seguimos en H1 (padre de H11 y H12)
                        h12 = child2;
                        signal(SIGUSR1, signal_handler);

                        for (int r = 0; r < N; r++){
                            // H1 recibe señal desde H2 para empezar su parte
                            pause();
                            printf("Hijo 1 [%d]\n", getpid());

                            // -> H12
                            __kill(h12, SIGUSR1);
                            pause(); // espera vuelta de H12
                            printf("Hijo 1 [%d]\n", getpid());

                            // -> H11
                            __kill(h11, SIGUSR1);
                            pause(); // espera vuelta de H11 (que a su vez habló con H112)
                            printf("Hijo 1 [%d]\n", getpid());

                            // Devuelve al Padre
                            __kill(getppid(), SIGUSR1);
                        }

                        // esperar a H11 y H12 antes de salir
                        wait(NULL); // H11
                        wait(NULL); // H12
                        return EXIT_SUCCESS;
                    }
                }

            // -------------------------
            // Rama H2 (idx == n_hijos-1 == 1):
            //   crea H21
            //   crea H22
            // -------------------------
            } else if(idx == (n_hijos - 1)){

                // fork para H21
                child = fork();
                if(child == 0){
                    // ===== Estoy en H21 =====
                    signal(SIGUSR1, signal_handler);

                    for (int r = 0; r < N; r++){
                        pause(); // señal desde H2
                        printf("Hijo 21 [%d]\n", getpid());
                        __kill(getppid(), SIGUSR1); // regreso -> H2
                    }

                    return EXIT_SUCCESS;

                } else {
                    h21 = child;

                    // fork para H22
                    child2 = fork();
                    if(child2 == 0){
                        // ===== Estoy en H22 =====
                        signal(SIGUSR1, signal_handler);

                        for (int r = 0; r < N; r++){
                            pause(); // señal desde H2
                            printf("Hijo 22 [%d]\n", getpid());
                            __kill(getppid(), SIGUSR1); // regreso -> H2
                        }

                        return EXIT_SUCCESS;

                    } else {
                        // ===== Estoy en H2 =====
                        h22 = child2;
                        signal(SIGUSR1, signal_handler);

                        for (int r = 0; r < N; r++){
                            pause(); // señal inicial desde Padre
                            printf("Hijo 2 [%d]\n", getpid());

                            // -> H22
                            __kill(h22, SIGUSR1);
                            pause(); // espera vuelta de H22
                            printf("Hijo 2 [%d]\n", getpid());

                            // -> H21
                            __kill(h21, SIGUSR1);
                            pause(); // espera vuelta de H21
                            printf("Hijo 2 [%d]\n", getpid());

                            // -> H1  (pids[0] es H1; tenemos copia local de pids[])
                            __kill(((pid_t*)pids)[0], SIGUSR1);
                        }

                        // esperar a H21 y H22 al final
                        wait(NULL); // H21
                        wait(NULL); // H22
                        return EXIT_SUCCESS;
                    }
                }

            }

            // No hay otros hijos (no existe "hijo intermedio" en este enunciado)
            break;
        }
    }

    // =========================
    // Proceso PADRE (root)
    // =========================
    if(root == getpid()){
        print_debug_tree();           // opcional: ver arbol pstree
        signal(SIGUSR1, signal_handler);

        for (int r = 0; r < N; r++){
            // inicio de la vuelta
            printf("Padre [%d]\n", getpid());

            // Padre -> H2 (pids[1])
            __kill(pids[1], SIGUSR1);

            // al final de toda la ronda, H1 devuelve señal al padre
            pause();
            printf("Padre [%d]\n", getpid());
        }

        // El padre hace wait de H1 y H2 al salir
        for(int i = 0; i < n_hijos; i++){
            wait(NULL);
        }
    }

    return EXIT_SUCCESS;
}
