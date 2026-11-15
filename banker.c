
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

int **criar_matriz(int linhas, int colunas) {
    int **matriz = calloc(linhas, sizeof(int *));

    if (matriz == NULL) {
        return NULL;
    }

    for (int i = 0; i < linhas; i++) {
        matriz[i] = calloc(colunas, sizeof(int));

        if (matriz[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(matriz[j]);
            }

            free(matriz);
            return NULL;
        }
    }

    return matriz;
}

void liberar_matriz(int **matriz, int linhas) {
    if (matriz == NULL) {
        return;
    }

    for (int i = 0; i < linhas; i++) {
        free(matriz[i]);
    }

    free(matriz);
}

int estado_seguro(int clientes, int recursos, int *disponiveis, int **allocation, int **need) {
    long long *work = malloc(recursos * sizeof(long long));
    int *finish = calloc(clientes, sizeof(int));

    if (work == NULL || finish == NULL) {
        free(work);
        free(finish);
        return -1;
    }

    for (int j = 0; j < recursos; j++) {
        work[j] = disponiveis[j];
    }

    int concluidos = 0;
    int progresso = 1;

    while (concluidos < clientes && progresso) {
        progresso = 0;

        for (int i = 0; i < clientes; i++) {
            if (finish[i]) {
                continue;
            }

            int pode_terminar = 1;

            for (int j = 0; j < recursos; j++) {
                if (need[i][j] > work[j]) {
                    pode_terminar = 0;
                    break;
                }
            }

            if (pode_terminar) {
                for (int j = 0; j < recursos; j++) {
                    work[j] += allocation[i][j];
                }

                finish[i] = 1;
                concluidos++;
                progresso = 1;
            }
        }
    }

    free(work);
    free(finish);

    return concluidos == clientes;
}

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

        if (errno == ERANGE || fim == argv[i + 1] || *fim != '\0' || valor < 0 || valor > INT_MAX) {
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

    if (ferror(arquivo)) {
        fprintf(stderr, "Erro: falha na leitura de customer.txt.\n");
        fclose(arquivo);
        free(disponiveis);
        return EXIT_FAILURE;
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

    int **maximum = criar_matriz(numero_clientes, numero_recursos);
    int **allocation = criar_matriz(numero_clientes, numero_recursos);
    int **need = criar_matriz(numero_clientes, numero_recursos);

    if (maximum == NULL || allocation == NULL || need == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria.\n");

        liberar_matriz(maximum, numero_clientes);
        liberar_matriz(allocation, numero_clientes);
        liberar_matriz(need, numero_clientes);
        fclose(arquivo);
        free(disponiveis);
        return EXIT_FAILURE;
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
            need[i][j] = maximum[i][j];
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
        liberar_matriz(maximum, numero_clientes);
        liberar_matriz(allocation, numero_clientes);
        liberar_matriz(need, numero_clientes);
        fclose(arquivo);
        free(disponiveis);
        return EXIT_FAILURE;
    }

    fclose(arquivo);

    FILE *comandos = fopen("commands.txt", "r");

    if (comandos == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir commands.txt.\n");
        liberar_matriz(maximum, numero_clientes);
        liberar_matriz(allocation, numero_clientes);
        liberar_matriz(need, numero_clientes);
        free(disponiveis);
        return EXIT_FAILURE;
    }

    char *linha_comando = NULL;
    size_t tamanho_comando = 0;
    int comandos_validos = 1;

    while (getline(&linha_comando, &tamanho_comando, comandos) != -1) {
        if (strcmp(linha_comando, "*\n") == 0 || strcmp(linha_comando, "*\r\n") == 0 || strcmp(linha_comando, "*") == 0) {
            continue;
        }

        if (strncmp(linha_comando, "RQ ", 3) != 0 && strncmp(linha_comando, "RL ", 3) != 0) {
            comandos_validos = 0;
            break;
        }

        char *p = linha_comando + 2;

        for (int i = 0; i <= numero_recursos; i++) {
            if (*p != ' ' && *p != '\t') {
                comandos_validos = 0;
                break;
            }

            while (*p == ' ' || *p == '\t') {
                p++;
            }

            if (*p < '0' || *p > '9') {
                comandos_validos = 0;
                break;
            }

            char *fim;
            errno = 0;
            long valor = strtol(p, &fim, 10);

            if (errno == ERANGE || valor > INT_MAX) {
                comandos_validos = 0;
                break;
            }

            if (i == 0 && valor >= numero_clientes) {
                comandos_validos = 0;
                break;
            }

            p = fim;
        }

        if (!comandos_validos) {
            break;
        }

        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (*p == '\r') {
            p++;
        }

        if (*p == '\n') {
            p++;
        }

        if (*p != '\0') {
            comandos_validos = 0;
            break;
        }
    }

    if (ferror(comandos)) {
        comandos_validos = 0;
    }

    free(linha_comando);
    fclose(comandos);

    if (!comandos_validos) {
        fprintf(stderr, "Erro: formato invalido em commands.txt.\n");
    }

    if (comandos_validos) {
        int seguro = estado_seguro(
            numero_clientes, numero_recursos,
            disponiveis, allocation, need
        );

        if (seguro == -1) {
            fprintf(stderr, "Erro: falha ao alocar memoria.\n");
            comandos_validos = 0;
        }
    }

    liberar_matriz(maximum, numero_clientes);
    liberar_matriz(allocation, numero_clientes);
    liberar_matriz(need, numero_clientes);
    free(disponiveis);

    return comandos_validos ? EXIT_SUCCESS : EXIT_FAILURE;
}
