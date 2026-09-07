#include "shell.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

    /* Volver al inicio del archivo */
    lseek(editor->fd, 0, SEEK_SET);

    char c;

    /* Caso: p  -> imprimir todo */
    if(argc == 1)
    {
        while(read(editor->fd, &c, 1) > 0)
        {
            write(STDOUT_FILENO, &c, 1);
        }

        return 0;
    }

    /* Caso: p n */
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

int cmd_i(int argc, char *argv[], Editor *editor)
{

}

int cmd_s(int argc, char *argv[], Editor *editor)
{

}
