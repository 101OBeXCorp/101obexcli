//
//  main.c
//  101obexcli
//
//  Created by Rafael Ruiz on 07/11/2022.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <pwd.h>
#include <unistd.h>
#else
#ifndef linux
#include <windows.h>
#include <Shlobj.h>
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif



char* TESTTOKEN = "{'access_token':'ya29.a0AeTM1ifzc1V1mx6nF-4ETdiRHx-S_l6aYaSt0nabcD6M7sHSP091ze5Gu_G4uBhxahpogOX62d84cwP7mMz5dFD0toF0C8zsTQ5QC4QR9tkz7AjV3LMtNIRr91PO8fkhjTqbGTz3IFG4EcOnjx1Uq-1VyghIaCgYKAUwSARESFQHWtWOmPZ_IzAQ6XGY_1QsOpGPV-Q0163','expires_in':3599,'refresh_token':'1//03tFX_HL0S2NKCgYIARAAGAMSNwF-L9IrAogNS-YfK8SLLD0oSuGuZ4oxNKlip28HqZlFLRbkuBS4dA5OaSKui7SMrKggS8RTyVs','scope':'https://www.googleapis.com/auth/cloud-platform','token_type':'Bearer'}";

typedef struct string_buffer_s
{
    char* ptr;
    size_t len;
} string_buffer_t;


static void string_buffer_initialize(string_buffer_t* sb)
{
    sb->len = 0;
    sb->ptr = malloc(sb->len + 1);
    sb->ptr[0] = '\0';
}


static void string_buffer_finish(string_buffer_t* sb)
{
    free(sb->ptr);
    sb->len = 0;
    sb->ptr = NULL;
}


static size_t string_buffer_callback(void* buf, size_t size, size_t nmemb, void* data)
{
    string_buffer_t* sb = data;
    size_t new_len = sb->len + size * nmemb;

    sb->ptr = realloc(sb->ptr, new_len + 1);

    memcpy(sb->ptr + sb->len, buf, size * nmemb);

    sb->ptr[new_len] = '\0';
    sb->len = new_len;

    return size * nmemb;

}

static size_t write_callback(void* buf, size_t size, size_t nmemb, void* data)
{
    return string_buffer_callback(buf, size, nmemb, data);
}

char* str_replace(char* orig, char* rep, char* with) {
    char* result;
    char* ins;
    char* tmp;
    size_t len_rep;
    size_t len_with;
    size_t len_front;
    size_t count;

    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL;
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
    }
    strcpy(tmp, orig);
    return result;
}

char* getKeyFromObjectString(char* data, char* key) {
    char key_indicator[256];
    unsigned long lengt_indicator;
    char* id_token;
    int copiando = 0;
    int indice = 0;

    id_token = malloc(8192);

    key_indicator[0] = '\0';
    strcat(key_indicator, key);
    strcat(key_indicator, "':'");

    lengt_indicator = strlen(key_indicator);

    for (int contador = 0; contador < strlen(data); contador++) {
        if (contador > lengt_indicator) {

            //char subbuff[lengt_indicator+1];
            char* subbuff;
            subbuff = malloc(lengt_indicator + 1);

            memcpy(subbuff, &data[contador - lengt_indicator], lengt_indicator);
            subbuff[lengt_indicator] = '\0';

            if (!strcmp(subbuff, key_indicator)) copiando = 1;

            if (copiando == 1) {
                id_token[indice++] = (char)data[contador];
                if (data[contador] == '\'') copiando = 0;
            }
        }

    }
    id_token[indice - 1] = '\0';
    return id_token;
}

int manage_token(int u) {

    CURL* obex_curl;
    CURLcode obex_res;
    string_buffer_t obex_strbuf;
    struct curl_slist* obex_headers = NULL;
    FILE* fptr;
    char datos[8192];
    char configfile[2048];

#ifdef __APPLE__
    struct passwd* pw = getpwuid(getuid());
    const char* homedir = pw->pw_dir;
#else
#ifndef linux
    const char* homedir = getenv("USERPROFILE");
#else
    const char* homedir = getenv("HOME");
#endif
#endif
    
    configfile[0] = '\0';
    strcat(configfile, homedir);
    strcat(configfile, "/.101oblex/config.json");

    fptr = fopen(configfile, "r");

    if (!fptr) {
        fprintf(stderr, "You must register Token first\nUse 101obexcli init\n");
        return 0;
    }

    fscanf(fptr, "%s", datos);

    fclose(fptr);

    char* datos_a_mostrar = getKeyFromObjectString(datos, "id_token");

    string_buffer_initialize(&obex_strbuf);

    obex_curl = curl_easy_init();

    if (!obex_curl)
    {
        fprintf(stderr, "Fatal: curl_easy_init() error.\n");
        string_buffer_finish(&obex_strbuf);
        return EXIT_FAILURE;
    }

    char obex_str[8192];
    obex_str[0] = '\0';

    if (u == 0) {
        strcat(obex_str, "https://api.101obex.com:3001/info_token?developer_token=");
        curl_easy_setopt(obex_curl, CURLOPT_CUSTOMREQUEST, "GET");
    }
    if (u == 1) {
        strcat(obex_str, "https://api.101obex.com:3001/reset_token?developer_token=");
        curl_easy_setopt(obex_curl, CURLOPT_CUSTOMREQUEST, "POST");
    }

    strcat(obex_str, datos_a_mostrar);

    curl_slist_append(obex_headers, "Accept: application/json");
    curl_slist_append(obex_headers, "Content-Type: application/json");
    curl_slist_append(obex_headers, "charset: utf-8");
    curl_easy_setopt(obex_curl, CURLOPT_HTTPHEADER, obex_headers);
    curl_easy_setopt(obex_curl, CURLOPT_USERAGENT, "libcrp/0.1");

    curl_easy_setopt(obex_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(obex_curl, CURLOPT_WRITEDATA, &obex_strbuf);


    curl_easy_setopt(obex_curl, CURLOPT_URL, obex_str);

    obex_res = curl_easy_perform(obex_curl);

    if (obex_res != CURLE_OK)
    {
        fprintf(stderr, "Request failed: curl_easy_perform(): %s\n", curl_easy_strerror(obex_res));

        curl_easy_cleanup(obex_curl);
        string_buffer_finish(&obex_strbuf);

        return EXIT_FAILURE;
    }

    printf("\n%s\n", obex_strbuf.ptr);

    if (u == 1) remove(configfile);

    return EXIT_SUCCESS;

}

int reset_token(int u) {

    return manage_token(1);
}

int get_info(int u) {

    return manage_token(0);
}



int register_token(int u)
{
    char string[512];

    CURL* curl;
    CURLcode res;
    string_buffer_t strbuf;
    CURL* obex_curl;
    CURLcode obex_res;
    string_buffer_t obex_strbuf;
    struct curl_slist* headers = NULL;
    struct curl_slist* obex_headers = NULL;
    char* respuesta_conv;

    if (u == 0) {


        string_buffer_initialize(&strbuf);

        curl = curl_easy_init();

        if (!curl)
        {
            fprintf(stderr, "Fatal: curl_easy_init() error.\n");
            string_buffer_finish(&strbuf);
            return EXIT_FAILURE;
        }

        printf("Please identify in your browser\n");
#ifdef __APPLE__
        system("open 'https://accounts.google.com/o/oauth2/v2/auth?client_id=578114581231-jaa6ncsp7bv06dra7g59vpfvb6736sea.apps.googleusercontent.com&response_type=code&scope=https://www.googleapis.com/auth/cloud-platform https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile&access_type=offline&redirect_uri=urn:ietf:wg:oauth:2.0:oob'");
#else
#ifndef linux
        system("cmd /c start \"\" \"https://accounts.google.com/o/oauth2/v2/auth?client_id=578114581231-jaa6ncsp7bv06dra7g59vpfvb6736sea.apps.googleusercontent.com&response_type=code&scope=https://www.googleapis.com/auth/cloud-platform https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile&access_type=offline&redirect_uri=urn:ietf:wg:oauth:2.0:oob\"");
#else
        system("open 'https://accounts.google.com/o/oauth2/v2/auth?client_id=578114581231-jaa6ncsp7bv06dra7g59vpfvb6736sea.apps.googleusercontent.com&response_type=code&scope=https://www.googleapis.com/auth/cloud-platform https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile&access_type=offline&redirect_uri=urn:ietf:wg:oauth:2.0:oob' >> /dev/null 2>&1");
#endif
#endif
        
        printf("And paste the code you received here: ");

        scanf("%s", string);

        char google_oauth_string[] = "https://www.googleapis.com/oauth2/v4/token?client_id=578114581231-jaa6ncsp7bv06dra7g59vpfvb6736sea.apps.googleusercontent.com&client_secret=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&grant_type=authorization_code&redirect_uri=urn:ietf:wg:oauth:2.0:oob&code=";

        char str[500];
        str[0] = '\0';
        strcat(str, google_oauth_string);
        strcat(str, string);

        curl_slist_append(headers, "Accept: application/json");
        curl_slist_append(headers, "Content-Type: application/json");
        curl_slist_append(headers, "charset: utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcrp/0.1");
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strbuf);
        curl_easy_setopt(curl, CURLOPT_URL, str);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "Request failed: curl_easy_perform(): %s\n", curl_easy_strerror(res));

            curl_easy_cleanup(curl);
            string_buffer_finish(&strbuf);

            return EXIT_FAILURE;
        }

        char respuesta[8192];
        strcpy(respuesta, strbuf.ptr);

        respuesta_conv = str_replace(respuesta, "\n", "");
        respuesta_conv = str_replace(respuesta_conv, "\"", "\'");
        respuesta_conv = str_replace(respuesta_conv, " ", "");

        FILE* fptr;

        char configfile[2048];

#ifdef __APPLE__
        struct passwd* pw = getpwuid(getuid());
        const char* homedir = pw->pw_dir;
#else
#ifndef linux
        const char* homedir = getenv("USERPROFILE");
#else
        const char* homedir = getenv("HOME");
#endif
#endif

        configfile[0] = '\0';
        strcat(configfile, homedir);
        strcat(configfile, "/.101oblex");

        struct stat st = { 0 };

        if (stat(configfile, &st) == -1) {
            mkdir(configfile, 0700);
        }
        strcat(configfile, "/config.json");

        fptr = fopen(configfile, "w");

        fprintf(fptr, "%s", respuesta_conv);

        fclose(fptr);

        curl_easy_cleanup(curl);
        string_buffer_finish(&strbuf);

    }
    else {
        respuesta_conv = TESTTOKEN;
    }
    string_buffer_initialize(&obex_strbuf);

    obex_curl = curl_easy_init();

    if (!obex_curl)
    {
        fprintf(stderr, "Fatal: curl_easy_init() error.\n");
        string_buffer_finish(&obex_strbuf);
        return EXIT_FAILURE;
    }

    char obex_str[8192];
    obex_str[0] = '\0';

    strcat(obex_str, "https://api.101obex.com:3001/register_token?google_response=");
    strcat(obex_str, respuesta_conv);


    curl_slist_append(obex_headers, "Accept: application/json");
    curl_slist_append(obex_headers, "Content-Type: application/json");
    curl_slist_append(obex_headers, "charset: utf-8");
    curl_easy_setopt(obex_curl, CURLOPT_HTTPHEADER, obex_headers);
    curl_easy_setopt(obex_curl, CURLOPT_USERAGENT, "libcrp/0.1");
    curl_easy_setopt(obex_curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(obex_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(obex_curl, CURLOPT_WRITEDATA, &obex_strbuf);


    curl_easy_setopt(obex_curl, CURLOPT_URL, obex_str);

    obex_res = curl_easy_perform(obex_curl);

    if (obex_res != CURLE_OK)
    {
        fprintf(stderr, "Request failed: curl_easy_perform(): %s\n", curl_easy_strerror(obex_res));

        curl_easy_cleanup(obex_curl);
        string_buffer_finish(&obex_strbuf);

        return EXIT_FAILURE;
    }

    printf("\n%s\n", obex_strbuf.ptr);

    return EXIT_SUCCESS;

}


int main(int argc, const char* argv[]) {

    if (argc > 1) {
        if (!strcmp(argv[1], "init")) return register_token(0);
        if (!strcmp(argv[1], "info")) return get_info(0);
        if (!strcmp(argv[1], "reset")) return reset_token(0);
        else fprintf(stderr, "Command not recognized.\nUse info, init, reset.\n");
    }
    else fprintf(stderr,"You must indicate a parameter.\nUse info, init, reset.\n");
}
