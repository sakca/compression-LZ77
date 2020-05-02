#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#define OFFSETBITS 5
#define LENGTHBITS (8 - OFFSETBITS)

#define OFFSETMASK ((1 << OFFSETBITS) - 1)
#define LENGTHMASK ((1 << LENGTHBITS) - 1)

#define GETOFFSET(x) (x >> LENGTHBITS)
#define GETLENGTH(x) (x & LENGTHMASK)
#define COMBINE(x,y) (x << LENGTHBITS | y)

using namespace std;

struct token {
    uint8_t offset_len;
    char c;
};


token* encode(char* text, int limit, int* numTokens);
char *decode(token* tokens, int numTokens, int* numDecoded);

char* readfile(char* filename, int& length);
token* readfile_enc(char* filename, int& length);
void writefile(char* filename, token* encoded, int n);
void writefile(char* filename, char* str, int n);

int prefix_matches(char *s1, char *s2, int limit);
void arrcopy(char *src, char *tar, int size);


int main()
{
    int len;

    int original_size;
    int number_tokens;
    int decoded_size;

    char* source_text;
    token* encoded_text;
    char* decoded_text;

    source_text = readfile("source.txt", len);
    original_size = len;
    encoded_text = encode(source_text, len, &number_tokens);
    writefile("encoded.txt", encoded_text, number_tokens);

    token* encoded_content = readfile_enc("encoded.txt", len);
    decoded_text = decode(encoded_content, number_tokens, &decoded_size);
    writefile("decoded.txt", decoded_text, decoded_size);

    cout << "Original size: " << original_size
         << ", Encoded size: " << number_tokens * sizeof(token)
         << ", Decoded size: " << decoded_size;

    return 0;
}


/** Returns token array. numTokens set to number of tokens used. */
token* encode(char* text, int limit, int* numTokens)
{
    int capacity = 1 << 3; // kapasite: 2^3
    int _numTokens = 0;

    struct token t; // şu anki token

    // lookahead ve search tamponlarının başlangıç pozisyonları
    char* lookahead;
    char* search;

    // tokenler için yer ayır
    struct token* encoded = (token*) malloc(capacity * sizeof(struct token));

    // token oluşturma döngüsü
    for(lookahead = text; lookahead < text + limit; lookahead++)
    {
        // Put search buffer 31 chars behind lookahead buffer
        search = lookahead - OFFSETMASK;

        // Out-of-bond check
        if (search < text)
            search = text;

        // Length of the longest matching sequence in search buffer
        int max_len = 0;

        // Position of the longest matching sequence in search buffer
        char* max_match = lookahead;

        // search tamponunda arama yap.
        while(search < lookahead)
        {
            int len = prefix_matches(search, lookahead, LENGTHMASK);

            if (len > max_len)
            {
                max_len = len;
                max_match = search;
            }

            search++;
        }

        /* ÖNEMLİ
        * Eğer eşleşmenin içine metnin son karakteri de dahil olmuşsa,
        * tokenin içine bir karakter koyabilmek için, eşleşmeyi kısaltmamız
        * gerekiyor. */
        // Fix for the case when the last char matches
        if (lookahead + max_len >= text + limit)
            max_len = text + limit - lookahead - 1;


        // Create a token for the match
        t.offset_len = COMBINE(lookahead - max_match, max_len);
        lookahead += max_len;
        t.c = *lookahead;

        // Expand the capacity if necessary
        if (_numTokens + 1 > capacity)
        {
            capacity = capacity << 1;
            encoded = (token*) realloc(encoded, capacity * sizeof(struct token));
        }

        // oluşturulan tokeni, diziye kaydet
        encoded[_numTokens++] = t;
    }

    // token sayısı istendiyse verilen değişkene kaydet
    if (numTokens)
        *numTokens = _numTokens;

    return encoded;
}

/** Converts token array to char array. */
char* decode(struct token *tokens, int numTokens, int* numDecoded)
{
    int capacity = 1 << 3; // kapasite
    char* decoded = (char*) malloc(capacity); // yer ayır

    // Number of bytes used
    *numDecoded = 0;

    for(int i = 0; i < numTokens; i++)
    {
        // Read offset, length and char values from token
        int offset = GETOFFSET(tokens[i].offset_len);
        int len = GETLENGTH(tokens[i].offset_len);
        char c = tokens[i].c;

        // gerekirse kapasite artır.
        if(*numDecoded + len + 1 > capacity)
        {
            capacity = capacity << 1;
            decoded = (char*) realloc(decoded, capacity);
        }

        // If length is not 0, copy the pointed char
        if(len > 0)
            arrcopy(&decoded[*numDecoded], &decoded[*numDecoded - offset], len);

        // Skip forward the amount of chars copied
        *numDecoded += len;

        // Append char inside of the token and forward the pointer
        decoded[*numDecoded] = c;
        *numDecoded = *numDecoded + 1;
    }

    return decoded;
}

//
// -- File Utilities //

/** Read whole file as chars (binary mode). Additionally provides file length. */
char* readfile(char* filename, int& length)
{
    char* text = NULL;
    ifstream infile(filename, ios::in|ios::binary|ios::ate);

    if (infile.is_open())
    {
        length = infile.tellg(); // boyutu öğren
        text = new char[length]; // yer ayır

        infile.seekg (0, ios::beg); // başa sar
        infile.read(text, length); // hepsini oku

        infile.close();
    }

    return text;
}

/** Read whole file as encoded tokens (binary mode). Additionally provides file length. */
token* readfile_enc(char* filename, int& length)
{
    token* tokens = NULL;
    char* text = readfile(filename, length);
    if(text)
    {
        tokens = new token[length/2]; // bir token 2 bayt yer tutuyor

        // offset_len ve c değerlerini metinden ayıkla
        for(int i = 0; i < length/2; i++)
            tokens[i] = token{text[i*2], text[i*2 + 1]};
    }


    return tokens;
}

/** Write tokens into a file (binary) */
void writefile(char* filename, token* encoded, int n)
{
    FILE* f;
    if (f = fopen(filename, "wb"))
    {
        fwrite(encoded, sizeof(token), n, f);
        fclose(f);
    }
}

/** Write chars into a file (binary) */
void writefile(char* filename, char* str, int n)
{
    FILE* f;
    if (f = fopen(filename, "wb"))
    {
        fwrite(str, 1, n, f);
        fclose(f);
    }
}

//
// -- AUX -- //

/** Number of characters that are identical at the beginning */
int prefix_matches(char* s1, char* s2, int limit)
{
    int len = 0;

    while(*s1++ == *s2++ && len < limit)
        len++;

    return len;
}

/** Bytewise array copy */
void arrcopy(char* src, char* tar, int size)
{
    while(size--)
        *src++ = *tar++;
}
