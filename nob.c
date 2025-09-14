#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    Cmd cmd = {0};
    cmd_append(&cmd, "cc", "-fdiagnostics-color=always", "-Wall", "-Wextra");
    cmd_append(&cmd, "-g");
    cmd_append(&cmd, "-o", "main", "main.c");
    cmd_append(&cmd, "-I./libs/raylib-5.5_linux_amd64/include/");
    cmd_append(&cmd, "-L./libs/raylib-5.5_linux_amd64/lib/");
    cmd_append(&cmd, "-l:libraylib.a");
    cmd_append(&cmd, "-lm");
    if (!cmd_run(&cmd))
    {
        return 1;
    }

    return 0;
}
