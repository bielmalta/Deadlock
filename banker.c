#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int **criar_matriz(int linhas, int colunas) {
    int **matriz = calloc((size_t)linhas, sizeof(*matriz));
    if (matriz == NULL) return NULL;

    for (int i = 0; i < linhas; i++) {
        matriz[i] = calloc((size_t)colunas, sizeof(**matriz));
        if (matriz[i] == NULL) {
            for (int j = 0; j < i; j++) free(matriz[j]);
            free(matriz);
            return NULL;
        }
    }
    return matriz;
}

static void liberar_matriz(int **matriz, int linhas) {
    if (matriz == NULL) return;
    for (int i = 0; i < linhas; i++) free(matriz[i]);
    free(matriz);
}

static int numero_inteiro(const char **p, int *valor) {
    if (**p < '0' || **p > '9') return 0;
    char *fim;
    errno = 0;
    long numero = strtol(*p, &fim, 10);
    if (errno == ERANGE || numero > INT_MAX) return 0;
    *valor = (int)numero;
    *p = fim;
    return 1;
}

static int fim_linha(const char *p) {
    if (*p == '\r') p++;
    if (*p == '\n') p++;
    return *p == '\0';
}

static int ler_cliente(const char *linha, int recursos, int *maximum) {
    const char *p = linha;
    for (int j = 0; j < recursos; j++) {
        if (!numero_inteiro(&p, &maximum[j])) return 0;
        if (j + 1 < recursos) {
            if (*p != ',') return 0;
            p++;
        }
    }
    return fim_linha(p);
}

static int ler_comando(const char *linha, int clientes, int recursos, char *tipo, int *cliente, int *valores) {
    if (linha[0] == '*') {
        if (!fim_linha(linha + 1)) return 0;
        *tipo = '*';
        return 1;
    }

    if (linha[0] != 'R' || (linha[1] != 'Q' && linha[1] != 'L')) return 0;
    *tipo = linha[1];
    const char *p = linha + 2;

    for (int j = -1; j < recursos; j++) {
        if (*p != ' ' && *p != '\t') return 0;
        while (*p == ' ' || *p == '\t') p++;
        int valor;
        if (!numero_inteiro(&p, &valor)) return 0;
        if (j == -1) {
            if (valor >= clientes) return 0;
            *cliente = valor;
        } else {
            valores[j] = valor;
        }
    }
    while (*p == ' ' || *p == '\t') p++;
    return fim_linha(p);
}

static int estado_seguro(int clientes, int recursos, const int *available, int **allocation, int **need) {
    long long *work = malloc((size_t)recursos * sizeof(*work));
    int *finish = calloc((size_t)clientes, sizeof(*finish));
    if (work == NULL || finish == NULL) {
        free(work);
        free(finish);
        return -1;
    }

    for (int j = 0; j < recursos; j++) work[j] = available[j];
    int concluidos = 0;
    int progresso = 1;

    while (concluidos < clientes && progresso) {
        progresso = 0;
        for (int i = 0; i < clientes; i++) {
            if (finish[i]) continue;
            int pode_terminar = 1;
            for (int j = 0; j < recursos; j++) {
                if (need[i][j] > work[j]) {
                    pode_terminar = 0;
                    break;
                }
            }
            if (pode_terminar) {
                for (int j = 0; j < recursos; j++) work[j] += allocation[i][j];
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

static void imprimir_vetor(FILE *saida, const int *vetor, int recursos) {
    for (int j = 0; j < recursos; j++) {
        if (j > 0) fputc(' ', saida);
        fprintf(saida, "%d", vetor[j]);
    }
}

static void imprimir_estado(FILE *saida, int clientes, int recursos, const int *available, int **maximum, int **allocation, int **need) {
    fprintf(saida, "MAXIMUM | ALLOCATION | NEED\n");
    for (int i = 0; i < clientes; i++) {
        imprimir_vetor(saida, maximum[i], recursos);
        fprintf(saida, " | ");
        imprimir_vetor(saida, allocation[i], recursos);
        fprintf(saida, " | ");
        imprimir_vetor(saida, need[i], recursos);
        fputc('\n', saida);
    }
    fprintf(saida, "AVAILABLE ");
    imprimir_vetor(saida, available, recursos);
    fputc('\n', saida);
}

static void erro(const char *mensagem) {
    fprintf(stderr, "Erro: %s\n", mensagem);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        erro("informe a quantidade de instancias de cada recurso.");
        return EXIT_FAILURE;
    }

    int recursos = argc - 1;
    int *available = NULL, *valores = NULL;
    int **maximum = NULL, **allocation = NULL, **need = NULL;
    FILE *clientes_arquivo = NULL, *comandos = NULL, *saida = NULL;
    char *linha = NULL;
    size_t tamanho = 0;
    int clientes = 0;
    int status = EXIT_FAILURE;

    available = malloc((size_t)recursos * sizeof(*available));
    if (available == NULL) {
        erro("falha ao alocar memoria.");
        goto finalizar;
    }

    for (int j = 0; j < recursos; j++) {
        const char *p = argv[j + 1];
        if (!numero_inteiro(&p, &available[j]) || *p != '\0') {
            erro("quantidade de recursos invalida.");
            goto finalizar;
        }
    }

    clientes_arquivo = fopen("customer.txt", "r");
    if (clientes_arquivo == NULL) {
        erro("nao foi possivel abrir customer.txt.");
        goto finalizar;
    }

    int c, ultimo = '\n';
    while ((c = fgetc(clientes_arquivo)) != EOF) {
        if (c == '\n') {
            if (clientes == INT_MAX) {
                erro("quantidade de clientes invalida.");
                goto finalizar;
            }
            clientes++;
        }
        ultimo = c;
    }
    if (ferror(clientes_arquivo)) {
        erro("falha na leitura de customer.txt.");
        goto finalizar;
    }
    if (ultimo != '\n') clientes++;
    if (clientes == 0) {
        erro("customer.txt esta vazio.");
        goto finalizar;
    }
    rewind(clientes_arquivo);

    maximum = criar_matriz(clientes, recursos);
    allocation = criar_matriz(clientes, recursos);
    need = criar_matriz(clientes, recursos);
    valores = malloc((size_t)recursos * sizeof(*valores));
    if (maximum == NULL || allocation == NULL || need == NULL || valores == NULL) {
        erro("falha ao alocar memoria.");
        goto finalizar;
    }

    for (int i = 0; i < clientes; i++) {
        if (getline(&linha, &tamanho, clientes_arquivo) == -1 || !ler_cliente(linha, recursos, maximum[i])) {
            erro("formato invalido em customer.txt.");
            goto finalizar;
        }
        for (int j = 0; j < recursos; j++) {
            need[i][j] = maximum[i][j];
        }
    }
    if (ferror(clientes_arquivo)) {
        erro("falha na leitura de customer.txt.");
        goto finalizar;
    }
    fclose(clientes_arquivo);
    clientes_arquivo = NULL;

    comandos = fopen("commands.txt", "r");
    if (comandos == NULL) {
        erro("nao foi possivel abrir commands.txt.");
        goto finalizar;
    }
    saida = fopen("result.txt", "w");
    if (saida == NULL) {
        erro("nao foi possivel criar result.txt.");
        goto finalizar;
    }

    while (getline(&linha, &tamanho, comandos) != -1) {
        char tipo;
        int cliente = 0;
        if (!ler_comando(linha, clientes, recursos, &tipo, &cliente, valores)) {
            erro("formato invalido em commands.txt.");
            goto finalizar;
        }
        if (tipo == '*') {
            imprimir_estado(saida, clientes, recursos, available, maximum, allocation, need);
            continue;
        }

        if (tipo == 'L') {
            for (int j = 0; j < recursos; j++) {
                if (valores[j] > allocation[cliente][j]) {
                    erro("liberacao excede os recursos alocados.");
                    goto finalizar;
                }
            }
            for (int j = 0; j < recursos; j++) {
                available[j] += valores[j];
                allocation[cliente][j] -= valores[j];
                need[cliente][j] += valores[j];
            }
            fprintf(saida, "Release from customer %d the resources ", cliente);
            imprimir_vetor(saida, valores, recursos);
            fputc('\n', saida);
            continue;
        }

        int excede_maximo = 0, insuficiente = 0;
        for (int j = 0; j < recursos; j++) {
            if (valores[j] > need[cliente][j]) excede_maximo = 1;
            if (valores[j] > available[j]) insuficiente = 1;
        }
        if (excede_maximo) {
            fprintf(saida, "The customer %d request ", cliente);
            imprimir_vetor(saida, valores, recursos);
            fprintf(saida, " was denied because exceed its maximum allocation\n");
        } else if (insuficiente) {
            fprintf(saida, "The resources ");
            imprimir_vetor(saida, available, recursos);
            fprintf(saida, " are not enough to customer %d request ", cliente);
            imprimir_vetor(saida, valores, recursos);
            fputc('\n', saida);
        } else {
            for (int j = 0; j < recursos; j++) {
                available[j] -= valores[j];
                allocation[cliente][j] += valores[j];
                need[cliente][j] -= valores[j];
            }
            int seguro = estado_seguro(clientes, recursos, available, allocation, need);
            if (seguro == -1) {
                erro("falha ao alocar memoria.");
                goto finalizar;
            }
            if (!seguro) {
                for (int j = 0; j < recursos; j++) {
                    available[j] += valores[j];
                    allocation[cliente][j] -= valores[j];
                    need[cliente][j] += valores[j];
                }
                fprintf(saida, "The customer %d request ", cliente);
                imprimir_vetor(saida, valores, recursos);
                fprintf(saida, " was denied because result in an unsafe state\n");
            } else {
                fprintf(saida, "Allocate to customer %d the resources ", cliente);
                imprimir_vetor(saida, valores, recursos);
                fputc('\n', saida);
            }
        }
    }

    if (ferror(comandos)) {
        erro("falha na leitura de commands.txt.");
        goto finalizar;
    }
    if (fclose(saida) != 0) {
        saida = NULL;
        erro("falha na escrita de result.txt.");
        goto finalizar;
    }
    saida = NULL;
    status = EXIT_SUCCESS;

finalizar:
    free(linha);
    if (clientes_arquivo != NULL) fclose(clientes_arquivo);
    if (comandos != NULL) fclose(comandos);
    if (saida != NULL) fclose(saida);
    liberar_matriz(maximum, clientes);
    liberar_matriz(allocation, clientes);
    liberar_matriz(need, clientes);
    free(valores);
    free(available);
    return status;
}
