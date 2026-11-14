
#include <stdio.h>
#include <string.h>
#include "main.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("ERROR: Insufficient arguments.\n");
        printf("Usage:\n");
        printf("View: ./mp3reader -v song.mp3\n");
        printf("Edit: ./mp3reader -e -t \"New Title\" song.mp3\n");
        printf("Help: ./mp3reader --help\n");
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0)
    {
        printf("MP3 Tag Reader and Editor\n\n");
        printf("View MP3 tags:\n");
        printf("./mp3reader -v song.mp3\n\n");

        printf("Edit MP3 tags:\n");
        printf("./mp3reader -e -t \"New Title\" song.mp3\n");
        printf("./mp3reader -e -a \"New Artist\" song.mp3\n");
        printf("./mp3reader -e -A \"New Album\" song.mp3\n");
        printf("./mp3reader -e -y \"2026\" song.mp3\n");
        printf("./mp3reader -e -g \"Soundtrack\" song.mp3\n");
        printf("./mp3reader -e -C \"New Composer\" song.mp3\n");

        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "-v") == 0)
    {
        if (validate_view(argv[2]) == 0)
        {
            return view_data(argv[2]);
        }
        return 1;
    }

    if (argc == 5 && strcmp(argv[1], "-e") == 0)
    {
        if (validate_edit(argv[4]) == 0)
        {
            return edit_data(argv[2], argv[3], argv[4]);
        }
        return 1;
    }

    printf("ERROR: Invalid arguments. Use ./mp3reader --help\n");
    return 1;
}

