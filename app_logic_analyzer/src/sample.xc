#include <xs1.h>

#pragma unsafe arrays

static inline void sample(buffered in port:8 a, buffered in port:8 b, buffered in port:8 c, buffered in port:8 d, const int lookuplow[256], const int lookuphigh[256], int &h, int &l) {
    int tmp;
    a :> tmp;
    l = lookuplow[tmp];
    h = lookuphigh[tmp];
    l <<= 1;
    h <<= 1;
    b :> tmp;
    l |= lookuplow[tmp];
    h |= lookuphigh[tmp];
    l <<= 1;
    h <<= 1;
    c :> tmp;
    l |= lookuplow[tmp];
    h |= lookuphigh[tmp];
    l <<= 1;
    h <<= 1;
    d :> tmp;
    l |= lookuplow[tmp];
    h |= lookuphigh[tmp];
}


#pragma unsafe arrays
void sampleHigh(buffered in port:8 a, buffered in port:8 b, buffered in port:8 c, buffered in port:8 d, const int lookuplow[256], const int lookuphigh[256], streaming chanend o) {
    while(1) {
        int h, l;
        sample(a, b, c, d, lookuplow, lookuphigh, h, l);
        l <<= 4;
        h <<= 4;
        o <: h;
        o <: l;
    }
}


#pragma unsafe arrays
void sampleLow(buffered in port:8 a, buffered in port:8 b, buffered in port:8 c, buffered in port:8 d, const int lookuplow[256], const int lookuphigh[256], streaming chanend i, streaming chanend o) {
    while(1) {
        int h, l, tmp;
        sample(a, b, c, d, lookuplow, lookuphigh, h, l);
        i :> tmp;
        o <: h | tmp;
        i :> tmp;
        o <: l | tmp;
    }
}

void sampler(buffered in port:8 a, buffered in port:8 b, buffered in port:8 c, buffered in port:8 d, buffered in port:8 e, buffered in port:8 f, buffered in port:8 g, buffered in port:8 h, const int lookuplow[256], const int lookuphigh[256], streaming chanend k) {
    streaming chan samples;
    par {
        sampleHigh(a,b,c,d,lookuplow, lookuphigh, samples);
        sampleLow(e,f,g,h,lookuplow, lookuphigh, samples, k);
    }
}

