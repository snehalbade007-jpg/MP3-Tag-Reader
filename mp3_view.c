
#include "main.h"
#include <stdint.h>

#define ID3_HEADER_SIZE 10
#define FRAME_HEADER_SIZE 10

unsigned int BtoL_endian(unsigned char *data)
{
    return ((unsigned int)data[0] << 24) | ((unsigned int)data[1] << 16) | ((unsigned int)data[2] << 8) | (unsigned int)data[3];
}

static unsigned int synchsafe_to_uint(unsigned char *data)
{
    return ((unsigned int)(data[0] & 0x7F) << 21) | ((unsigned int)(data[1] & 0x7F) << 14) | ((unsigned int)(data[2] & 0x7F) << 7) | (unsigned int)(data[3] & 0x7F);
}

static void display_frame(const char *id, const unsigned char *data, unsigned int size)
{
    const char *label = NULL;

    if (memcmp(id, "TIT2", 4) == 0)
        label = "Title";
    else if (memcmp(id, "TPE1", 4) == 0)
        label = "Artist";
    else if (memcmp(id, "TALB", 4) == 0)
        label = "Album";
    else if (memcmp(id, "TYER", 4) == 0 ||
             memcmp(id, "TDRC", 4) == 0)
        label = "Year";
    else if (memcmp(id, "TCON", 4) == 0)
        label = "Genre";
    else if (memcmp(id, "TCOM", 4) == 0)
        label = "Composer";

    if (label == NULL || size < 1)
        return;

    /* Skip the text encoding byte. */
    data++;
    size--;

    printf("%s - %-8s: ", id, label);

    for (unsigned int i = 0; i < size && data[i] != '\0'; i++)
    {
        if (data[i] >= 32 && data[i] <= 126) putchar(data[i]);
    }

    putchar('\n');
}

int validate_view(char *file)
{
    FILE *fp = fopen(file, "rb");

    if (fp == NULL)
    {
        printf("ERROR: Cannot open file %s\n", file);
        return 1;
    }

    unsigned char header[ID3_HEADER_SIZE];

    if (fread(header, 1, ID3_HEADER_SIZE, fp) != ID3_HEADER_SIZE ||
        memcmp(header, "ID3", 3) != 0)
    {
        printf("ERROR: Invalid or missing ID3 header.\n");
        fclose(fp);
        return 1;
    }

    if (header[3] != 3 && header[3] != 4)
    {
        printf("ERROR: Only ID3v2.3 and ID3v2.4 are supported.\n");
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}

int view_data(char *file)
{
    FILE *fp = fopen(file, "rb");

    if (fp == NULL)
    {
        printf("ERROR: Cannot open file %s\n", file);
        return 1;
    }

    unsigned char header[ID3_HEADER_SIZE];

    if (fread(header, 1, ID3_HEADER_SIZE, fp) != ID3_HEADER_SIZE || memcmp(header, "ID3", 3) != 0)
    {
        printf("ERROR: Invalid or missing ID3 header.\n");
        fclose(fp);
        return 1;
    }

    if (header[3] != 3 && header[3] != 4)
    {
        printf("ERROR: Only ID3v2.3 and ID3v2.4 are supported.\n");
        fclose(fp);
        return 1;
    }

    unsigned int tag_size = synchsafe_to_uint(&header[6]);
    unsigned char *tag = malloc(tag_size ? tag_size : 1);

    if (tag == NULL)
    {
        printf("ERROR: Memory allocation failed.\n");
        fclose(fp);
        return 1;
    }

    if (fread(tag, 1, tag_size, fp) != tag_size)
    {
        printf("ERROR: Cannot read ID3 tag.\n");
        free(tag);
        fclose(fp);
        return 1;
    }

    printf("\n   ========== MP3 TAG DETAILS ==========\n");
    printf("             ID3 Version: 2.%d.%d\n", header[3], header[4]);
    putchar('\n');

    unsigned int offset = 0;

    while (offset + FRAME_HEADER_SIZE <= tag_size)
    {
        unsigned char *frame = tag + offset;

        if (frame[0] == 0)
            break;

        unsigned int frame_size;

        if (header[3] == 4)
            frame_size = synchsafe_to_uint(&frame[4]);
        else
            frame_size = BtoL_endian(&frame[4]);

        if (frame_size > tag_size - offset - FRAME_HEADER_SIZE)
        {
            printf("WARNING: Invalid frame size; stopping tag parsing.\n");
            break;
        }

        display_frame((const char *)frame, frame + FRAME_HEADER_SIZE,
                      frame_size);

        offset += FRAME_HEADER_SIZE + frame_size;
    }

    printf("========================================\n");

    free(tag);
    fclose(fp);
    return 0;
}

