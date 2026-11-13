#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>

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

    int numero_clientes = 0;
    int caractere;
    int ultimo_caractere = '\n';

    while ((caractere = fgetc(arquivo)) != EOF) {
        if (caractere == '\n') {
            numero_clientes++;
        }

        ultimo_caractere = caractere;
    }

    if (ultimo_caractere != '\n') {
        numero_clientes++;
    }

    if (numero_clientes == 0) {
        fprintf(stderr, "Erro: customer.txt esta vazio.\n");
        fclose(arquivo);
        free(disponiveis);
        return EXIT_FAILURE;
    }

    rewind(arquivo);

    int **maximum = malloc(numero_clientes * sizeof(int *));

    if (maximum == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria.\n");
        fclose(arquivo);
        free(disponiveis);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < numero_clientes; i++) {
        maximum[i] = malloc(numero_recursos * sizeof(int));

        if (maximum[i] == NULL) {
            fprintf(stderr, "Erro: falha ao alocar memoria.\n");

            for (int j = 0; j < i; j++) {
                free(maximum[j]);
            }

            free(maximum);
            fclose(arquivo);
            free(disponiveis);
            return EXIT_FAILURE;
        }
    }

    
    char *linha = NULL;
    size_t tamanho = 0;
    int formato_valido = 1;

    for (int i = 0; i < numero_clientes && formato_valido; i++) {
        if (getline(&linha, &tamanho, arquivo) == -1) {
            formato_valido = 0;
            break;
        }

        char *p = linha;

        for (int j = 0; j < numero_recursos; j++) {
            if (*p < '0' || *p > '9') {
                formato_valido = 0;
                break;
            }

            char *fim;
            errno = 0;
            long valor = strtol(p, &fim, 10);

            if (errno == ERANGE || valor > INT_MAX) {
                formato_valido = 0;
                break;
            }

            maximum[i][j] = (int)valor;
            p = fim;

            if (j < numero_recursos - 1) {
                if (*p != ',') {
                    formato_valido = 0;
                    break;
                }

                p++;
            }
        }

        if (formato_valido && *p == '\r') {
            p++;
        }

        if (formato_valido && *p == '\n') {
            p++;
        }

        if (formato_valido && *p != '\0') {
            formato_valido = 0;
        }
    }

    free(linha);

    if (!formato_valido || ferror(arquivo)) {
        fprintf(stderr, "Erro: formato invalido em customer.txt.\n");

        for (int i = 0; i < numero_clientes; i++) {
            free(maximum[i]);
        }

        free(maximum);
        fclose(arquivo);
        free(disponiveis);
        return EXIT_FAILURE;
    }

    fclose(arquivo);

    for (int i = 0; i < numero_clientes; i++) {
        free(maximum[i]);
    }

    free(maximum);
    free(disponiveis);

    return EXIT_SUCCESS;
}