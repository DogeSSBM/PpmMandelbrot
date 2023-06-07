#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <complex.h>
#include <stdarg.h>
#include <tgmath.h>
#include <pthread.h>
#include "Panic.h"

typedef FILE File;
typedef uint8_t u8;
typedef unsigned int uint;
typedef union{
    u8 arr[3];
    struct{
        u8 r;
        u8 g;
        u8 b;
    };
}Color;

#define THREADS 4
#define WX 5000
#define WY 4000
#define SECY ((WY/2)/THREADS)
#define BAIL 256
#define STEP (2.5 / (double)WX)

typedef struct{
    uint yoff;
    u8** v;
}Section;

Section secNew(const uint yoff)
{
    Section sec = {
        .yoff = yoff,
        .v = calloc(WX, sizeof(u8*))
    };
    for(uint x = 0; x < WX; x++)
        sec.v[x] = calloc(SECY, sizeof(u8));
    return sec;
}

void secFree(Section sec)
{
    if(!sec.v)
        return;
    for(uint x = 0; x < WX; x++)
        free(sec.v[x]);
    free(sec.v);
}

void* secCalc(void* arg)
{
    Section *sec = arg;
    printf("yoff: %i\n", sec->yoff);
    for(uint x = 0; x < WX; x++){
        const char buf[2] = {'0'+x/10000, '\0'};
        if(!(x%10000))
            puts(buf);
        for(uint y = 0; y < SECY; y++){
            u8 v = 0;
            const double xr = -2.0 + x*STEP;
            const double yi = -1.0 + (y+sec->yoff)*STEP;
            const double q = (xr-0.25)*(xr-0.25)+yi*yi;
            if(q*(q+(xr-0.25))<=0.25*(yi*yi)){
                v = 0;
                goto done;
            }
            double complex z = 0;
            for(uint i = 0; i < BAIL; i++){
                z = (z*z)+(xr + yi*I);
                if(creal(z) * creal(z) + cimag(z) * cimag(z) > 4.0){
                    v = (u8)(((double)i/BAIL)*255.0);
                    goto done;
                }
            }
            done:
            sec->v[x][y] = v;
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    File *f = NULL;
    if(argc != 2)
        panic("Usage: %s <FileName.pgm>", argv[0]);
    if((f = fopen(argv[1], "r"))){
        fclose(f);
        panic("File \"%s\" already exists!", argv[1]);
    }
    if(!(f = fopen(argv[1], "wb")))
        panic("Unable to create file \"%s\"", argv[1]);
    Section secs[THREADS] = {0};
    pthread_t thread[THREADS] = {0};
    for(int t = 0; t < THREADS; t++){
        secs[t] = secNew(t*SECY);
        if(pthread_create(&thread[t], NULL, secCalc, &secs[t]))
            panic("Could not create thread[%i]", t);
    }

    for(int t = 0; t < THREADS; t++)
        pthread_join(thread[t], NULL);
    printf("Done calculating, writing to file\n");

    Color pal[256] = {0};
    int o = 1;
    float c = 0.0f;
    for(int i = 0; i < 256; i++){
        if(c > 42.0f){
            c = 0;
            o++;
        }
        pal[i].arr[o%3] = (u8)((c/42)*255.0f);
        pal[i].arr[(o+1)%3] = 128-pal[i].arr[o%3];
        c++;
    }

    fprintf(f, "P6\n%i %i\n255\n", WX, WY);
    for(uint y = 0; y < WY/2; y++){
        const char buf[2] = {'0'+2+(y/10000), '\0'};
        if(!(y%10000))
            puts(buf);
        for(uint x = 0; x < WX; x++){
            const u8 d = secs[y/SECY].v[x][y%SECY];
            fwrite(pal[d].arr, 1, 3, f);
        }
    }
    for(uint y = WY/2; y > 0; y--){
        const char buf[2] = {'0'+y/10000, '\0'};
        if(!(y%10000))
            puts(buf);
        for(uint x = 0; x < WX; x++){
            const u8 d = secs[(y-1)/SECY].v[x][(y-1)%SECY];
            fwrite(pal[d].arr, 1, 3, f);
        }
    }
    fclose(f);
    printf("Done writing file\n");

    for(int t = 0; t < THREADS; t++)
        secFree(secs[t]);
    printf("Done!\n");

    return 0;
}
