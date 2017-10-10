#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "blowfish.h"
long int fsize(FILE *f) {
    long int original, size;
    original = ftell(f);
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, original, SEEK_SET);
    return size;
}

struct file_struct {
    size_t size;
    BYTE *data;
};

struct file_struct *create_file_struct(size_t size, BYTE *data);
struct file_struct *read_file(char *name);
struct file_struct *truncate_padding(struct file_struct *data);
void load_data(BYTE *data, size_t infilesize, FILE *infile);
void decrypt(struct file_struct *data, char *userkey);
void write_file(FILE *outfile, struct file_struct *data);
void save_output_file(char *name, struct file_struct *f);
BYTE *allocate_data(size_t size);
FILE *get_file(char *infilename);
FILE *get_output_file(char *name);
size_t get_file_size(FILE *infile);
size_t get_model_data_offset(BYTE *data, size_t size);

int main(int argc, char **argv) {
    char cubepro_key[] = "221BBakerMycroft";
    char bfb[] = ".bfb";
    char *infilename;
    char *outfilename;
    if (argc < 2) {
        return -1;
    }
    infilename = argv[1]; //assign argument
    struct file_struct *f = truncate_padding(read_file(infilename)); //read input file and remove prefix
    decrypt(f, cubepro_key); //run data through blowfish
	outfilename = strcat(infilename, bfb); //make outfile name
    save_output_file(outfilename, f); //save output file
    return 0;
}

FILE *get_file(char *infilename) {
    FILE *infile = fopen(infilename, "rb");
    if (infile == NULL) {
        perror("Unable to open input file");
    }
    return infile;
}

size_t get_file_size(FILE *infile) {
    size_t infilesize = fsize(infile);
    if (infilesize < 0) {
        perror("Unable to determine size of input file");
        return 3;
    }                            
    return infilesize;
}

BYTE *allocate_data(size_t size) {
    BYTE *data = malloc(size);                
    if (data == NULL) {
        perror("Unable to allocate memory for input file");
    }
    return data;
}

void load_data(BYTE *data, size_t infilesize, FILE *infile) {
    size_t readcount = fread(data, 1, infilesize, infile);
    if (readcount != infilesize) {
        printf("Unable to read the whole input file: %zd != %zd\n", readcount, infilesize);
    }
}

struct file_struct *create_file_struct(size_t size, BYTE *data) {
    struct file_struct *f = malloc(sizeof(struct file_struct));
    f->size = size;
    f->data = data;
    return f;
}

struct file_struct *read_file(char *name) {
    FILE *infile = get_file(name);
    size_t size = get_file_size(infile);
    BYTE *data = allocate_data(size);
    load_data(data, size, infile);
    fclose(infile);
    return create_file_struct(size, data);
}

size_t get_model_data_offset(BYTE *data, size_t size) {
	int i;
    for (i=0; i < size; i++) {
        if (data[i] == 0xc8) {
            return i;
        }
    }
}

struct file_struct *truncate_padding(struct file_struct *data) {
    size_t offset = get_model_data_offset(data->data, data->size);
    size_t truncated_size = data->size - offset;
    BYTE *truncated_data = malloc(truncated_size);
    memcpy(truncated_data, data->data+offset, truncated_size );
    return create_file_struct(truncated_size, truncated_data);
}

void decrypt(struct file_struct *data, char *userkey) {
    BLOWFISH_KEY key;
    blowfish_key_setup((BYTE*) userkey, &key, strlen(userkey));
    int i;
    for (i = 0; i < data->size; i += 8) {
        blowfish_decrypt(&data->data[i], &data->data[i], &key, 1);
    }
}

FILE *get_output_file(char *name) {
    FILE *outfile = fopen(name, "wb");
    if (outfile == NULL) {
        perror("Unable to open output file");
    }
    return outfile;
}

void write_file(FILE *outfile, struct file_struct *data) {
    size_t writecount = fwrite(data->data, 1, data->size, outfile);
    if (writecount != data->size) {
        printf("Unable to write the whole output file: %zd != %zd\n", writecount, data->size);
    }
}

void save_output_file(char *name, struct file_struct *f) {
    FILE *outfile = get_output_file(name);
    write_file(outfile, f);
    fclose(outfile);
}
