
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

#define HEADER_SIZE 10
#define FRAME_HEADER_SIZE 10

static unsigned int synchsafe_to_uint(const unsigned char *data)
{
    return ((unsigned int)(data[0] & 0x7F) << 21) | ((unsigned int)(data[1] & 0x7F) << 14) | ((unsigned int)(data[2] & 0x7F) << 7) | (unsigned int)(data[3] & 0x7F);
}

static void uint_to_synchsafe(unsigned int size, unsigned char *data)
{
    data[0] = (size >> 21) & 0x7F;
    data[1] = (size >> 14) & 0x7F;
    data[2] = (size >> 7) & 0x7F;
    data[3] = size & 0x7F;
}

void LtoB_endian(unsigned int size, unsigned char *data)
{
    data[0] = (size >> 24) & 0xFF;
    data[1] = (size >> 16) & 0xFF;
    data[2] = (size >> 8) & 0xFF;
    data[3] = size & 0xFF;
}

int validate_edit(char *file)
{
    FILE *fp = fopen(file, "rb");
    unsigned char header[HEADER_SIZE];

    if (fp == NULL)
    {
        printf("ERROR: Cannot open %s\n", file);
        return 1;
    }

    if (fread(header, 1, HEADER_SIZE, fp) != HEADER_SIZE || memcmp(header, "ID3", 3) != 0)
    {
        printf("ERROR: Valid ID3 metadata was not found.\n");
        fclose(fp);
        return 1;
    }

    fclose(fp);

    if (strstr(file, ".mp3") == NULL)
    {
        printf("ERROR: Please provide an MP3 file.\n");
        return 1;
    }

    return 0;
}

static const char *get_frame_id(const char *option, unsigned char version)
{
    if (strcmp(option, "-t") == 0)
        return "TIT2";

    if (strcmp(option, "-a") == 0)
        return "TPE1";

    if (strcmp(option, "-A") == 0)
        return "TALB";

    if (strcmp(option, "-y") == 0)
        return version == 3 ? "TYER" : "TDRC";

    if (strcmp(option, "-g") == 0)
        return "TCON";

    if (strcmp(option, "-C") == 0)
        return "TCOM";

    return NULL;
}

static int write_frame(FILE *out, const char *frame_id, const char *value, unsigned char version)
{
    unsigned char frame_header[FRAME_HEADER_SIZE] = {0};
    unsigned char encoding = 0;
    unsigned int text_size = (unsigned int)strlen(value) + 1;

    memcpy(frame_header, frame_id, 4);

    if (version == 4)
        uint_to_synchsafe(text_size, &frame_header[4]);
    else
        LtoB_endian(text_size, &frame_header[4]);

    if (fwrite(frame_header, 1, FRAME_HEADER_SIZE, out) != FRAME_HEADER_SIZE)
        return 1;

    if (fwrite(&encoding, 1, 1, out) != 1)
        return 1;

    if (fwrite(value, 1, text_size - 1, out) != text_size - 1)
        return 1;

    return 0;
}

int edit_data(char *option, char *new_data, char *file)
{
    FILE *fp = NULL;
    FILE *frames = NULL;
    FILE *out = NULL;
    unsigned char header[HEADER_SIZE];
    unsigned char *tag = NULL;
    char *temp_name = NULL;
    const char *target;
    unsigned int old_tag_size;
    unsigned int new_tag_size = 0;
    unsigned int offset = 0;
    int failed = 0;
    int found = 0;
    int valid_frames = 1;

    fp = fopen(file, "rb");
    if (fp == NULL)
    {
        printf("ERROR: Cannot open MP3 file.\n");
        return 1;
    }

    if (fread(header, 1, HEADER_SIZE, fp) != HEADER_SIZE ||
        memcmp(header, "ID3", 3) != 0)
    {
        printf("ERROR: Invalid ID3 header.\n");
        fclose(fp);
        return 1;
    }

    if (header[3] != 3 && header[3] != 4)
    {
        printf("ERROR: Only ID3v2.3 and ID3v2.4 are supported.\n");
        fclose(fp);
        return 1;
    }

    target = get_frame_id(option, header[3]);
    if (target == NULL)
    {
        printf("ERROR: Invalid option. Use -t, -a, -A, -y, -g or -C.\n");
        fclose(fp);
        return 1;
    }

    old_tag_size = synchsafe_to_uint(&header[6]);

    tag = malloc(old_tag_size ? old_tag_size : 1);
    if (tag == NULL)
    {
        printf("ERROR: Memory allocation failed.\n");
        fclose(fp);
        return 1;
    }

    if (fread(tag, 1, old_tag_size, fp) != old_tag_size)
    {
        printf("ERROR: Cannot read ID3 tag.\n");
        free(tag);
        fclose(fp);
        return 1;
    }

    /*
     * Validate the existing frames. If a frame is malformed,
     * discard the old tag contents and rebuild a clean tag.
     */
    while (offset + FRAME_HEADER_SIZE <= old_tag_size)
    {
        unsigned char *frame_header = tag + offset;
        unsigned int frame_size;

        if (frame_header[0] == 0)
            break;

        if (header[3] == 4)
            frame_size = synchsafe_to_uint(&frame_header[4]);
        else
            frame_size = ((unsigned int)frame_header[4] << 24) | ((unsigned int)frame_header[5] << 16) |((unsigned int)frame_header[6] << 8) | (unsigned int)frame_header[7];

        if (frame_size > old_tag_size - offset - FRAME_HEADER_SIZE)
        {
            valid_frames = 0;
            break;
        }

        offset += FRAME_HEADER_SIZE + frame_size;
    }

    frames = tmpfile();
    if (frames == NULL)
    {
        printf("ERROR: Cannot create temporary storage.\n");
        free(tag);
        fclose(fp);
        return 1;
    }

    if (valid_frames)
    {
        offset = 0;

        while (offset + FRAME_HEADER_SIZE <= old_tag_size)
        {
            unsigned char *frame_header = tag + offset;
            unsigned int frame_size;
            unsigned int total;

            if (frame_header[0] == 0)
                break;

            if (header[3] == 4)
                frame_size = synchsafe_to_uint(&frame_header[4]);
            else
                frame_size = ((unsigned int)frame_header[4] << 24) | ((unsigned int)frame_header[5] << 16) | ((unsigned int)frame_header[6] << 8) | (unsigned int)frame_header[7];

            total = FRAME_HEADER_SIZE + frame_size;

            if (memcmp(frame_header, target, 4) == 0)
            {
                if (!found)
                {
                    if (write_frame(frames, target, new_data, header[3]) != 0)
                        failed = 1;

                    found = 1;
                    new_tag_size += FRAME_HEADER_SIZE + (unsigned int)strlen(new_data) + 1;
                }
                /* Skip duplicate copies of the target frame. */
            }
            else
            {
                if (fwrite(frame_header, 1, total, frames) != total)
                {
                    failed = 1;
                    break;
                }

                new_tag_size += total;
            }

            offset += total;
        }
    }

    /* If the old tag was malformed, create a clean tag with the new frame. */
    if (!valid_frames || !found)
    {
        if (valid_frames)
        {
            /* Existing valid frames were already written; append the new one. */
            if (write_frame(frames, target, new_data, header[3]) != 0)
                failed = 1;

            new_tag_size += FRAME_HEADER_SIZE + (unsigned int)strlen(new_data) + 1;
        }
        else
        {
            rewind(frames);
            new_tag_size = 0;

            if (write_frame(frames, target, new_data, header[3]) != 0)
                failed = 1;

            new_tag_size = FRAME_HEADER_SIZE + (unsigned int)strlen(new_data) + 1;
        }
    }

    if (failed || new_tag_size > 0x0FFFFFFF)
    {
        printf("ERROR: Failed to prepare updated metadata.\n");
        fclose(frames);
        free(tag);
        fclose(fp);
        return 1;
    }

    temp_name = malloc(strlen(file) + 5);
    if (temp_name == NULL)
    {
        fclose(frames);
        free(tag);
        fclose(fp);
        return 1;
    }

    sprintf(temp_name, "%s.tmp", file);

    out = fopen(temp_name, "wb");
    if (out == NULL)
    {
        printf("ERROR: Cannot create output file.\n");
        free(temp_name);
        fclose(frames);
        free(tag);
        fclose(fp);
        return 1;
    }

    uint_to_synchsafe(new_tag_size, &header[6]);

    if (fwrite(header, 1, HEADER_SIZE, out) != HEADER_SIZE || fseek(frames, 0, SEEK_SET) != 0)
        failed = 1;

    {
        unsigned char buffer[4096];
        size_t count;

        while (!failed && (count = fread(buffer, 1, sizeof(buffer), frames)) > 0)
        {
            if (fwrite(buffer, 1, count, out) != count)
                failed = 1;
        }

        while (!failed && (count = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        {
            if (fwrite(buffer, 1, count, out) != count)
                failed = 1;
        }

        if (ferror(fp) || ferror(frames))
            failed = 1;
    }

    if (fclose(out) != 0)
        failed = 1;

    fclose(frames);
    fclose(fp);
    free(tag);

    if (failed)
    {
        printf("ERROR: Failed to write updated MP3.\n");
        remove(temp_name);
        free(temp_name);
        return 1;
    }

    if (rename(temp_name, file) != 0)
    {
        printf("ERROR: Could not replace original file.\n");
        printf("Temporary file: %s\n", temp_name);
        free(temp_name);
        return 1;
    }

    free(temp_name);
    printf("SUCCESS: %s updated successfully.\n", target);
    return 0;
}
