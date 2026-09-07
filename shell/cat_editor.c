#include "shell.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int cmd_o(int argc, char *argv[], Editor *editor)
{
    if(argc < 2)
        return -1;

    const char *nArchivo = argv[1];

    return open(nArchivo, O_RDWR | O_CREAT, 0644);
}


