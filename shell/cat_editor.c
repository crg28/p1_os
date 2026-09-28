#include "shell.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Vuelca el archivo completo apuntado por 'fd' a un buffer dinámico (malloc)
static char *read_all_fd(int fd, off_t *out_size) {
    LOG_SYSCALL("lseek", "%d, 0, SEEK_END", fd);
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return NULL;
    }
    LOG_SYSCALL_RESULT(size);

    LOG_SYSCALL("lseek", "%d, 0, SEEK_SET", fd);
    off_t rc = lseek(fd, 0, SEEK_SET);
    if (rc == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return NULL;
    }
    LOG_SYSCALL_RESULT(rc);

    char *buffer = malloc((size_t)size + 1);
    if (buffer == NULL) {
        fprintf(stderr, COLOR_ERROR "malloc: sin memoria disponible.\n" COLOR_RESET);
        return NULL;
    }

    off_t total = 0;
    while (total < size) {
        LOG_SYSCALL("read", "%d, buffer+%ld, %ld", fd, (long)total, (long)(size - total));
        ssize_t leidos = read(fd, buffer + total, (size_t)(size - total));
        if (leidos < 0) {
            LOG_SYSCALL_ERROR(strerror(errno));
            free(buffer);
            return NULL;
        }
        LOG_SYSCALL_RESULT(leidos);
        if (leidos == 0) break; // EOF inesperado
        total += leidos;
    }

    *out_size = total;
    return buffer;
}

// Abrir archivo -> o -nombreArchivo
int cmd_o(int argc, char *argv[], Editor *editor)
{
    if(argc < 2){
        fprintf(stderr, COLOR_ERROR "Uso: o [nombre_archivo.ext] \n" COLOR_RESET);
        return -1;
    }

    const char *nArchivo = argv[1];

    editor->fd = open(nArchivo, O_RDWR | O_CREAT, 0644);

    if(editor->fd == -1)
    {
        perror("open");
        return -1;
    }
}

// Imprime la linea n -> p [n] | o todo -> p
int cmd_p(int argc, char *argv[], Editor *editor)
{

    if(editor->fd == -1)
    {
        fprintf(stderr, COLOR_ERROR "No hay un archivo abierto.\n" COLOR_RESET);
        return -1;
    }

    lseek(editor->fd, 0, SEEK_SET);

    char c;

    if(argc == 1)
    {
        while(read(editor->fd, &c, 1) > 0)
        {
            write(STDOUT_FILENO, &c, 1);
        }

        return 0;
    }

    int lineaObjetivo = atoi(argv[1]);

    if(lineaObjetivo <= 0)
    {
        fprintf(stderr, COLOR_ERROR "Número de línea inválido.\n" COLOR_RESET);
        return -1;
    }

    int lineaActual = 1;

    while(read(editor->fd, &c, 1) > 0)
    {
        if(lineaActual == lineaObjetivo)
            write(STDOUT_FILENO, &c, 1);

        if(c == '\n')
        {
            if(lineaActual == lineaObjetivo)
                break;

            lineaActual++;
        }
    }
    return 0;
}


int cmd_a(int argc, char *argv[], Editor *editor)
{

}

int cmd_d(int argc, char *argv[], Editor *editor)
{

}

int cmd_q(int argc, char *argv[], Editor *editor)
{

}

// Inserta "texto" como nueva línea antes de la línea n, reescribiendo el archivo
int cmd_i(int argc, char *argv[], Editor *editor)
{
    if (editor->fd == -1) {
        fprintf(stderr, COLOR_ERROR "No hay un archivo abierto. Usa 'o <archivo>' primero.\n" COLOR_RESET);
        return -1;
    }

    if (argc < 3) {
        fprintf(stderr, COLOR_ERROR "Uso: i [n] [texto]\n" COLOR_RESET);
        return -1;
    }

    int lineaObjetivo = atoi(argv[1]);
    if (lineaObjetivo <= 0) {
        fprintf(stderr, COLOR_ERROR "Número de línea inválido.\n" COLOR_RESET);
        return -1;
    }

    // Reconstruir el texto uniendo los argumentos restantes con espacios
    size_t textoLen = 0;
    for (int i = 2; i < argc; i++) {
        textoLen += strlen(argv[i]) + 1; /* +1 por el espacio o el nulo final */
    }
    char *texto = malloc(textoLen);
    if (texto == NULL) {
        fprintf(stderr, COLOR_ERROR "malloc: sin memoria disponible.\n" COLOR_RESET);
        return -1;
    }
    texto[0] = '\0';
    for (int i = 2; i < argc; i++) {
        strcat(texto, argv[i]);
        if (i != argc - 1) strcat(texto, " ");
    }
    textoLen = strlen(texto);

    off_t fileSize = 0;
    char *contenido = read_all_fd(editor->fd, &fileSize);
    if (contenido == NULL) {
        free(texto);
        return -1;
    }

    off_t offset = 0;
    int lineaActual = 1;
    while (offset < fileSize && lineaActual < lineaObjetivo) {
        if (contenido[offset] == '\n') lineaActual++;
        offset++;
    }

    // Nuevo buffer: [0, offset) + texto + '\n' + [offset, fileSize)
    off_t nuevoSize = fileSize + (off_t)textoLen + 1;
    char *nuevoContenido = malloc((size_t)nuevoSize);
    if (nuevoContenido == NULL) {
        fprintf(stderr, COLOR_ERROR "malloc: sin memoria disponible.\n" COLOR_RESET);
        free(texto);
        free(contenido);
        return -1;
    }
    memcpy(nuevoContenido, contenido, (size_t)offset);
    memcpy(nuevoContenido + offset, texto, textoLen);
    nuevoContenido[offset + (off_t)textoLen] = '\n';
    memcpy(nuevoContenido + offset + (off_t)textoLen + 1, contenido + offset, (size_t)(fileSize - offset));

    LOG_SYSCALL("lseek", "%d, 0, SEEK_SET", editor->fd);
    off_t rc = lseek(editor->fd, 0, SEEK_SET);
    if (rc == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        goto cleanup_error;
    }
    LOG_SYSCALL_RESULT(rc);

    LOG_SYSCALL("write", "%d, nuevoContenido, %ld", editor->fd, (long)nuevoSize);
    ssize_t escritos = write(editor->fd, nuevoContenido, (size_t)nuevoSize);
    if (escritos == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        goto cleanup_error;
    }
    LOG_SYSCALL_RESULT(escritos);

    LOG_SYSCALL("ftruncate", "%d, %ld", editor->fd, (long)nuevoSize);
    int trc = ftruncate(editor->fd, nuevoSize);
    if (trc == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        goto cleanup_error;
    }
    LOG_SYSCALL_RESULT(trc);

    printf(COLOR_RESULT "Línea insertada en la posición %d (%zu bytes añadidos).\n" COLOR_RESET,
           lineaObjetivo, textoLen + 1);

    free(texto);
    free(contenido);
    free(nuevoContenido);
    return 0;

cleanup_error:
    free(texto);
    free(contenido);
    free(nuevoContenido);
    return -1;
}

// Busca todas las apariciones de "palabra" e informa línea y columna
int cmd_s(int argc, char *argv[], Editor *editor)
{
    if (editor->fd == -1) {
        fprintf(stderr, COLOR_ERROR "No hay un archivo abierto. Usa 'o <archivo>' primero.\n" COLOR_RESET);
        return -1;
    }

    if (argc != 2) {
        fprintf(stderr, COLOR_ERROR "Uso: s [palabra]\n" COLOR_RESET);
        return -1;
    }

    const char *palabra = argv[1];
    size_t wlen = strlen(palabra);
    if (wlen == 0) {
        fprintf(stderr, COLOR_ERROR "La palabra a buscar no puede estar vacía.\n" COLOR_RESET);
        return -1;
    }

    off_t fileSize = 0;
    char *contenido = read_all_fd(editor->fd, &fileSize);
    if (contenido == NULL) {
        return -1;
    }

    printf(COLOR_TITLE "--- Resultados de búsqueda de \"%s\" ---\n" COLOR_RESET, palabra);

    int lineaActual = 1;
    off_t columnaActual = 1;
    off_t i = 0;
    int coincidencias = 0;

    while (i < fileSize) {
        if (contenido[i] == '\n') {
            lineaActual++;
            columnaActual = 1;
            i++;
            continue;
        }

        if ((size_t)(fileSize - i) >= wlen && strncmp(contenido + i, palabra, wlen) == 0) {
            printf("  " COLOR_RESULT "Coincidencia" COLOR_RESET " en línea " COLOR_PARAM "%d" COLOR_RESET
                   ", columna " COLOR_PARAM "%ld" COLOR_RESET "\n", lineaActual, (long)columnaActual);
            coincidencias++;
            i += wlen;
            columnaActual += wlen;
            continue;
        }

        i++;
        columnaActual++;
    }

    if (coincidencias == 0) {
        printf(COLOR_INFO "No se encontraron coincidencias.\n" COLOR_RESET);
    } else {
        printf(COLOR_TITLE "Total de coincidencias: " COLOR_RESULT "%d\n" COLOR_RESET, coincidencias);
    }

    free(contenido);
    return 0;
}
