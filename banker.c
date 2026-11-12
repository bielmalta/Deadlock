#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Erro: informe a quantidade de instancias de cada recurso.\n");
        return EXIT_FAILURE;
    }

    int numero_recursos = argc - 1;

    int *disponiveis = malloc(numero_recursos * sizeof(int));

    if (disponiveis == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria.\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < numero_recursos; i++) {
        char *fim;
        errno = 0;
        long valor = strtol(argv[i + 1], &fim, 10);

        if (errno == ERANGE || fim == argv[i + 1] ||
            *fim != '\0' || valor < 0 || valor > INT_MAX) {
            fprintf(stderr, "Erro: quantidade de recursos invalida.\n");
            free(disponiveis);
            return EXIT_FAILURE;
        }

        disponiveis[i] = (int)valor;
    }

    FILE *arquivo = fopen("customer.txt", "r");

    if (arquivo == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir customer.txt.\n");
        free(disponiveis);
        return EXIT_FAILURE;
    }

    fclose(arquivo);

    free(disponiveis);

    return EXIT_SUCCESS;
}